/****************************************************************************
**
** This file is part of LAN Messenger.
**
** Copyright (c) LAN Messenger Contributors.
** 
** LAN Messenger is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** LAN Messenger is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with LAN Messenger.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/


#include "theme.h"

const QString defTheme = DefinitionsDir::resThemeDir();

/*
    themeData.inMsg         = "/Content.html";
    themeData.inNextMsg     = "/NextContent.html";
    themeData.pubMsg        = "/Broadcast.html";
    themeData.sysMsg        = "/Status.html";
    themeData.stateMsg      = "/Status.html";
    themeData.sysNextMsg    = "/NextStatus.html";
    themeData.reqMsg        = "/Request.html";
*/


const ThemeData lmTheme::loadTheme(const QString &path) {
    QFile file;
    ThemeData themeData;

    themeData.themePath = path;

    file.setFileName(path + "/old_content.html");
    if(!file.open(QIODevice::ReadOnly))
        return loadTheme(defTheme);
    themeData.inMsg = QString(file.readAll().constData());
    file.close();

    file.setFileName(path + "/old_nextcontent.html");
    if(!file.open(QIODevice::ReadOnly))
        return loadTheme(defTheme);
    themeData.inNextMsg = QString(file.readAll().constData());
    file.close();

    file.setFileName(path + "/old_broadcast.html");
    if(!file.open(QIODevice::ReadOnly))
        return loadTheme(defTheme);
    themeData.pubMsg = QString(file.readAll().constData());
    file.close();

    file.setFileName(path + "/old_status.html");
    if(!file.open(QIODevice::ReadOnly))
        return loadTheme(defTheme);
    QString sys = QString(file.readAll().constData());
    themeData.sysMsg = sys;
    themeData.stateMsg = sys;
    file.close();

    file.setFileName(path + "/old_nextstatus.html");
    if(!file.open(QIODevice::ReadOnly))
        return loadTheme(defTheme);
    themeData.sysNextMsg = QString(file.readAll().constData());
    file.close();

    file.setFileName(path + "/old_request.html");
    if(!file.open(QIODevice::ReadOnly))
        return loadTheme(defTheme);
    themeData.reqMsg = QString(file.readAll().constData());
    file.close();

    return themeData;
}
