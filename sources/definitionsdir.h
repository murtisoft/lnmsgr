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


#ifndef DEFINITIONSDIR_H
#define DEFINITIONSDIR_H

#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QDesktopServices>
#include <QDateTime>
#include "settings.h"

#define TRANSFERHISTORY		"transfers.lst"
#define CACHEDIR			"cache"
#define LANGDIR				"translations"
#define THEMEDIR			"templates"
#define GROUPFILE			"group.cfg"
#define AVATARFILE			"avt_local.png"
#define LOGDIR				"logs"
#define TEMPCONFIG			"lmtmpconf.ini"

class DefinitionsDir {
public:
	static QString transferHistory(void) {
        return QDir::toNativeSeparators(QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation) + "/" TRANSFERHISTORY);
	}

	static QString fileStorageDir(void) {
		lmSettings settings;

        QString path = QDir::toNativeSeparators(QStandardPaths::writableLocation(
            QStandardPaths::DownloadLocation));
		path = settings.value(IDS_FILESTORAGEPATH, path).toString();
		return path;
	}

	static QString cacheDir(void) {
        return QDir::toNativeSeparators(QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation) + "/" CACHEDIR);
	}

	static QString libDir(void) {
		return QDir::toNativeSeparators(QDir::currentPath());
	}

	static QString resLangDir(void) {
        return ":/" LANGDIR;
	}

	static QString sysLangDir(void) {
        return QDir::toNativeSeparators(QDir::currentPath() + "/" LANGDIR);
	}

	static QString userLangDir(void) {
        return QDir::toNativeSeparators(QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation) + "/" LANGDIR);
	}

	static QString resThemeDir(void) {
        return ":/" THEMEDIR;
	}

	static QString sysThemeDir(void) {
        return QDir::toNativeSeparators(QDir::currentPath() + "/" THEMEDIR);
	}

	static QString userThemeDir(void) {
        return QDir::toNativeSeparators(QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation) + "/" THEMEDIR);
	}

	static QString groupFile(void) {
        return QDir::toNativeSeparators(QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation) + "/" GROUPFILE);
	}

	static QString avatarFile(void) {
        return QDir::toNativeSeparators(QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation) + "/" AVATARFILE);
	}

	static QString logDir(void) {
        return QDir::toNativeSeparators(QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation) + "/" LOGDIR);
	}

	static QString freeLogFile(void) {
        QString fileName = QString(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss")) + ".log";
		return QDir::toNativeSeparators(logDir() + "/" + fileName);
	}

	static QString tempConfigFile(void) {
        return QDir::toNativeSeparators(QStandardPaths::writableLocation(
            QStandardPaths::TempLocation) + "/" TEMPCONFIG);
	}
};

#endif // DEFINITIONSDIR_H
