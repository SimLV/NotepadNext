/*
 * This file is part of Notepad Next.
 * Copyright 2022 Justin Dailey
 *
 * Notepad Next is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Notepad Next is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Notepad Next.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "lua.hpp"

#include <QThread>

#include "LuaBridge.h"
#include "LuaStream.h"

LuaStream::LuaStream(QThread *thread) :
    thread(thread)
{
    L = luaL_newstate();

    luaL_openlibs(L);
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, &lua_Print, 1);
    lua_setglobal(L, "print");
}

int LuaStream::lua_Print(lua_State *L)
{
    LuaStream *This = reinterpret_cast<LuaStream *>(lua_touserdata(L, lua_upvalueindex(1)));
    int top = lua_gettop(L);
    QByteArray array;
    size_t sz;
    lua_getglobal(L, "tostring");

    for (int i = 1; i <= top; i++) {
        const char *ptr;
        lua_pushvalue(L, -1);
        lua_pushvalue(L, i);  // Stack: ...args, tostring, tostring, args[i]
        if (0 != lua_pcall(L, 1, 1, 0)) {
            ptr = lua_tolstring(L, -1, &sz);
            emit This->onError(QString::fromUtf8(ptr, sz));
            return 0;
        }
        if (i > 1) {
                array.append("\t");
        }
        ptr = lua_tolstring(L, -1, &sz);
        array.append(ptr, sz);
        lua_settop(L, top + 1);
    }
    if (top > 0) {
        array.append("\n");
        emit This->onLineReady(array);
    }
    return 0;
}

void LuaStream::loadBuffer(QByteArray const &data)
{
    luaL_loadstring(L, data.constData());
}

void LuaStream::process()
{
    const char *ptr;
    size_t sz;
    if (lua_isstring(L, -1)) {
        emit onError(QString(lua_tostring(L, -1)));
        lua_close(L);
        thread->quit();
        return;
    }
    if (LUA_OK != lua_pcall(L, 0, 1, 0)) {
        emit onError(QString(lua_tostring(L, -1)));
        lua_close(L);
        thread->quit();
        return;
    }

    lua_State *th = lua_tothread(L, -1); //  Loop the thread and print everything else
    if (th == NULL) {
        lua_getglobal(L, "tostring");
        lua_pushvalue(L, -2);
        lua_pcall(L, 1, 1, 0);

        ptr = lua_tolstring(L, -1, &sz);
        emit onLineReady(QByteArray(ptr, sz));
        lua_close(L);
        thread->quit();
        return;
    }

    int ret;
    do {
        lua_pushvalue(th, 1);
        ret = lua_resume(th, L, 0);
        if (((ret == LUA_OK) || (ret == LUA_YIELD)) && (lua_gettop(th) >= 1)) {
            ptr = lua_tolstring(th, 1, &sz);
            emit onLineReady(QByteArray(ptr, sz));
        }
        lua_settop(th, 1);
    } while (ret == LUA_YIELD);

    if (ret != LUA_OK) {
        emit onError(QString(lua_tostring(th, -1)));
    }

    lua_close(L);
    thread->quit();
}
