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


#ifndef LUASTREAM_H
#define LUASTREAM_H

#include <QObject>

struct lua_State;
class QThread;

class LuaStream : public QObject
{
    Q_OBJECT

public:
    explicit LuaStream(QThread *parent);

    void loadBuffer(QByteArray const &data);

public slots:
    void process();

signals:
    void onLineReady(QByteArray newLine);
    void onError(QString error);

private:
    static int lua_Print(lua_State *L);

private:
    lua_State *L;
    QThread *thread;
};

#endif
