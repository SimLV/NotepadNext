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

#include "LuaStream.h"

LuaStream::LuaStream(QObject *parent) :
    QThread(parent)
{
    L = luaL_newstate();

    luaL_openlibs(L);
}

void LuaStream::loadBuffer(QByteArray const &data)
{
    luaL_loadstring(L, data.constData());
}

void LuaStream::process()
{
    size_t sz;
    if (lua_isstring(L, -1))
    {
        emit onError(QString(lua_tostring(L, -1)));
        lua_close(L);
        quit();
        return;
    }
    if (LUA_OK != lua_pcall(L, 0, 1, 0))
    {
        emit onError(QString(lua_tostring(L, -1)));
        lua_close(L);
        quit();
        return;
    }
    if (lua_isstring(L, -1))
    {
        emit onLineReady(QByteArray(lua_tolstring(L, -1, &sz), sz));
    }
    else if (lua_type(L, -1) != LUA_TTHREAD)
    {
        emit onLineReady(QString("%1").arg(lua_typename(L, lua_type(L, -1))).toUtf8());
        return;
    }

    emit onLineReady(QString("top=%1\n").arg(lua_gettop(L)).toUtf8());

    lua_State *th = lua_tothread(L, -1);
    int ret;
    do {
        ret = lua_resume(th, L, 0);
        if (lua_gettop(th) >= 2)
        {
            emit onLineReady(QByteArray("\n"));
            emit onLineReady(QByteArray(lua_tolstring(L, 2, &sz), sz));
        }
        lua_settop(th, 1);
    } while (ret == LUA_YIELD);

    emit onLineReady(QString("finished\n").toUtf8());

    if (ret != LUA_OK)
    {
        emit onError(QString(lua_tostring(th, -1)));
    }

    lua_close(L);
    quit();
}
