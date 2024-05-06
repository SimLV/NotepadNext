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

#include "StreamManager.h"

#include <QThread>

#include "EditorManager.h"
#include "LuaStream.h"
#include "NotepadNextApplication.h"
#include "ScintillaNext.h"

StreamManager::StreamManager(NotepadNextApplication* app):
    QObject(app),
    app(app),
    uiNum(0)
{
}

void StreamManager::stopAll()
{
    qInfo(Q_FUNC_INFO);
}

ScintillaNext* StreamManager::startStream(ScintillaNext *editor, enum Type type)
{
    qInfo(Q_FUNC_INFO);

    QThread *thread = new QThread(this);

    switch (type) {
    case Lua:
        {
            if (!editor)
            {
                qWarning("Unable to start stream without editor");
                return nullptr;
            }

            LuaStream *stream = new LuaStream(thread);
            stream->loadBuffer(editor->getText(editor->textLength()));
            stream->moveToThread(thread);

            ScintillaNext *newEditor = app->getEditorManager()->createEditor(tr("Lua(%1)").arg(uiNum++));
            connect(thread, &QThread::started, stream, &LuaStream::process);
            connect(thread, &QThread::finished, thread, &QThread::deleteLater);
            connect(stream, &LuaStream::onLineReady, newEditor, &ScintillaNext::appendBytes);
            connect(stream, &LuaStream::onError, this, [=] (const QString &error){
                    newEditor->appendError(error);
                    qWarning("Stream error: %s", error.toUtf8().constData());
                });

            thread->start();
            return newEditor;
        }
    default:
        return nullptr;
    }
}
