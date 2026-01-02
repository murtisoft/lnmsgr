/****************************************************************************
**
** This file is part of LAN Messenger.
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

#include <QDir>
#include <QFileInfo>
#include <QTranslator>
#include <QApplication>
#include "translations.h"

#define IDS_LANGUAGE_VAL "en_US"
Translators Application::translators;
Translators Application::sysTranslators;
QTranslator* Application::current = nullptr;
QTranslator* Application::sysCurrent = nullptr;

Application::Application(const QString& id, int& argc, char** argv)
    : QApplication(argc, argv) {
    Application::current = nullptr;
    Application::sysCurrent = nullptr;
}

Application::~Application(void) {
}

void Application::loadTranslations(const QString& dir) {
    loadTranslations(QDir(dir));
}

void Application::loadTranslations(const QDir& dir) {
    QString filter = "*_*.qm";
    QDir::Filters filters = QDir::Files | QDir::Readable;
    QDir::SortFlags sort = QDir::Name;
    QFileInfoList entries = dir.entryInfoList(QStringList() << filter, filters, sort);
    foreach (QFileInfo file, entries) {
        QStringList parts = file.baseName().split("_");
        QString language = parts.at(parts.count() - 2).toLower();
        QString country = parts.at(parts.count() - 1).toUpper();

        QTranslator* translator = new QTranslator(instance());
        if (translator->load(file.absoluteFilePath())) {
            QString locale = language + "_" + country;
            Application::translators.insert(locale, translator);
        }
    }
    Application::translators.insert(IDS_LANGUAGE_VAL, nullptr);

    QDir sysDir(dir.absolutePath() + "/system");
    if(!sysDir.exists())
        return;

    entries = sysDir.entryInfoList(QStringList() << filter, filters, sort);
    foreach (QFileInfo file, entries) {
        QStringList parts = file.baseName().split("_");
        QString language = parts.at(parts.count() - 2).toLower();
        QString country = parts.at(parts.count() - 1).toUpper();

        QTranslator* translator = new QTranslator(instance());
        if (translator->load(file.absoluteFilePath())) {
            QString locale = language + "_" + country;
            Application::sysTranslators.insert(locale, translator);
        }
    }
}

const QStringList Application::availableLanguages() {
    return QStringList(Application::translators.keys());
}

void Application::setLanguage(const QString& locale) {
    if(Application::current)
        removeTranslator(Application::current);
    if(Application::sysCurrent)
        removeTranslator(Application::sysCurrent);

    Application::sysCurrent = Application::sysTranslators.value(locale, nullptr);
    if(Application::sysCurrent)
        installTranslator(Application::sysCurrent);

    Application::current = Application::translators.value(locale, nullptr);
    if(Application::current)
        installTranslator(Application::current);
}
