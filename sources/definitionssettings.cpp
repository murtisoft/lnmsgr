/****************************************************************************
**
** This file is part of LAN Messenger.
**
** Copyright (c) LAN Messenger Authors.
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


#include "definitionssettings.h"
#include "definitionsdir.h"
#include "shareduifunctions.h"
#include "definitionsui.h"
#include <QVersionNumber>
#include <QList>
#include <QPair>
#include <climits>
#include <QSet>
#include <functional>

//	Keys that existed in older versions and are no longer used. Add a key here when it is retired.
//	Nothing else needs touching when a new setting is added, unknown keys are kept as they are.
static const QStringList obsoleteKeys = {
	IDS_BROADCAST_OLD,
};

//	Value sanity checks. A value that fails is removed, so the application falls back to the default.
//	Optional, a key without an entry here is kept as is.
static const QList<QPair<QString, std::function<bool(const QVariant&)>>>& valueChecks() {
	auto inRange = [](int min, int max) {
		return [min, max](const QVariant& v) { bool ok; int n = v.toInt(&ok); return ok && n >= min && n <= max; };
	};
	static const QList<QPair<QString, std::function<bool(const QVariant&)>>> checks = {
		{IDS_UDPPORT, inRange(1, 65535)},
		{IDS_TCPPORT, inRange(1, 65535)},
		{IDS_TIMEOUT, inRange(1, INT_MAX)},
		{IDS_MAXRETRIES, inRange(0, INT_MAX)},
		{IDS_REFRESHTIME, inRange(1, INT_MAX)},
		{IDS_IDLETIME, inRange(0, INT_MAX)},
		{IDS_FONTSIZE, inRange(0, FS_COUNT - 1)},
		{IDS_COLORSCHEME, inRange(0, 2)},
		{IDS_USERLISTVIEW, inRange(0, ULV_Max - 1)},
		{IDS_MICACTIVATION, inRange(0, MA_Max - 1)},
	};
	return checks;
}

//	Keys and groups that describe this machine rather than the user's preferences. They are not
//	carried over when importing a config file from somewhere else.
static bool isMachineLocal(const QString& key) {
	static const QStringList prefixes = {"Window/", "Splitter/", "Application/", "GroupExp/"};
	static const QSet<QString> keys = {IDS_HISTORYPATH, IDS_FILEHISTORYPATH, IDS_FILESTORAGEPATH};
	for(const QString& prefix : prefixes)
		if(key.startsWith(prefix))
			return true;
	return keys.contains(key);
}

//	Array style settings ("Name/size", "Name/1/Key"...). Importing replaces the whole array, not merges it.
static QString topLevelGroup(const QString& key) {
	int slash = key.indexOf('/');
	return slash < 0 ? QString() : key.left(slash);
}

lmSettingsBase::lmSettingsBase(void) : QSettings() {
}

lmSettingsBase::lmSettingsBase(const QString& fileName, Format format) :
	QSettings(fileName, format) {
}

lmSettingsBase::lmSettingsBase(Format format, Scope scope, const QString& organization, const QString& application) :
	QSettings(format, scope, organization, application) {
}

lmSettingsBase::~lmSettingsBase(void) {
}

void lmSettingsBase::setValue(const QString& key, const QVariant& value, const QVariant& defaultValue) {
    if(value != defaultValue)
        QSettings::setValue(key, value);
    else
        remove(key);
}

//	settings are stored in <AppDataLocation>/Settings.ini
lmSettings::lmSettings(void) : lmSettingsBase(DefinitionsDir::settingsFile(), QSettings::IniFormat) {
}

//	migrate settings from older versions to new format
//	Returns false if existing settings cannot be migrated, else true
bool lmSettings::migrateSettings(void) {
	//	Make sure any pending write operation is completed
	sync();
	//	If settings file does not exist, return true indicating no error
	if(!QFile::exists(fileName()))
		return true;

	//	Migrate the settings
	bool migrated = migrateSettings(fileName());
	//	Sync the settings with the settings file
	sync();
	return migrated;
}

//	Load settings from the specified config file and overwrite corresponding
//	application settings
bool lmSettings::loadFromConfig(const QString& configFile) {
	if(!QFile::exists(configFile))
		return false;

	if(!Helper::copyFile(configFile, DefinitionsDir::tempConfigFile()))
		return false;

	if(!migrateSettings(DefinitionsDir::tempConfigFile()))
		return false;

	QSettings extSettings(DefinitionsDir::tempConfigFile(), QSettings::IniFormat);

	//	Import everything that survived migration, except what is specific to the other machine.
	//	New settings are picked up automatically.
	const QStringList keys = extSettings.allKeys();

	//	An imported array replaces the existing one, otherwise leftover entries would be mixed in
	QSet<QString> replaced;
	for(const QString& key : keys) {
		if(key == IDS_VERSION || isMachineLocal(key))
			continue;
		QString group = topLevelGroup(key);
		if(!group.isEmpty() && key.count('/') > 1 && !replaced.contains(group)) {
			replaced.insert(group);
			remove(group);
		}
	}
	for(const QString& key : keys) {
		if(key == IDS_VERSION || isMachineLocal(key))
			continue;
		QSettings::setValue(key, extSettings.value(key));
	}

	setValue(IDS_VERSION, IDA_VERSION);
	sync();

	QFile::remove(DefinitionsDir::tempConfigFile());

	return true;
}

void lmSettings::setAutoStart(bool on) {
#ifdef Q_OS_WIN
	QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
		QSettings::NativeFormat);
	if(on)
		settings.setValue(IDA_TITLE, QDir::toNativeSeparators(QApplication::applicationFilePath()));
	else
		settings.remove(IDA_TITLE);
#endif

#ifdef Q_OS_MAC
    Q_UNUSED(on);
#endif

#ifdef Q_OS_X11
	//  get the path of .desktop file
	QString autoStartDir;
	char* buffer = getenv("XDG_CONFIG_HOME");
	if(buffer) {
		autoStartDir = QString(buffer);
		autoStartDir.append("/autostart");
	} else {
		buffer = getenv("HOME");
		autoStartDir = QString(buffer);
		autoStartDir.append("/.config/autostart");
	}
	QDir dir(autoStartDir);
	QString fileName = dir.absoluteFilePath("lm.desktop");
	//	delete the file if autostart is set to false
	if(!on) {
		QFile::remove(fileName);
		return;
	}

	if(!dir.exists())
		dir.mkpath(dir.absolutePath());
	QFile file(fileName);
	if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return;
	QTextStream stream(&file);
	stream.setCodec("UTF-8");
	stream.setGenerateByteOrderMark(false);
	stream << "[Desktop Entry]\n";
	stream << "Encoding=UTF-8\n";
	stream << "Type=Application\n";
	stream << "Name=" << IDA_TITLE << "\n";
	stream << "Comment=Send and receive instant messages\n";
	stream << "Icon=lm\n";
	stream << "Exec=sh " << qApp->applicationDirPath() << "/lm.sh\n";
	stream << "Terminal=false\n";
	file.close();
#endif
}

//	The function expects the config file to exist. Validation must be done
//	prior to calling the function.
bool lmSettings::migrateSettings(const QString& configFile) {
    lmSettingsBase settings(configFile, QSettings::IniFormat);

    //	A missing or unreadable version is treated as an old file, not as an error
    QVersionNumber fileVersion = QVersionNumber::fromString(settings.value(IDS_VERSION).toString());
    QVersionNumber appVersion = QVersionNumber::fromString(IDA_VERSION);

    //	Settings from a later version are deemed non migratable, they are reset
    if(!fileVersion.isNull() && !appVersion.isNull() && fileVersion > appVersion) {
        QFile::remove(configFile);
        return false;
    }

    //	Remove keys that are no longer used
    for(const QString& key : obsoleteKeys)
        settings.remove(key);

    //	Remove values that do not make sense, defaults take over. Everything else is kept.
    for(const auto& check : valueChecks()) {
        if(settings.contains(check.first) && !check.second(settings.value(check.first)))
            settings.remove(check.first);
    }

    settings.setValue(IDS_VERSION, IDA_VERSION);
    settings.sync();
    return true;
}
