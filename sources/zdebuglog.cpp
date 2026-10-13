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


#include "zdebuglog.h"
#include "definitions.h"
#include "definitionsdir.h"

std::atomic<bool> lmDebugLog::enabled{false};
QString lmDebugLog::fileName;

lmDebugLog::lmDebugLog(void) {
}

lmDebugLog::~lmDebugLog(void) {
}

static const char* const LOG_SEPARATOR = "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-";

void lmDebugLog::init(QString fileName, bool trace) {
    lmDebugLog::fileName = fileName;

    if (trace)      //Command line only sets the starting state of this session.
        setEnabled(true);
}

void lmDebugLog::setEnabled(bool enable) {
    if (enable == enabled)
        return;

    if (enable) {
        QDir dir(DefinitionsDir::logDir());
        QStringList logs = dir.entryList({"*.log"}, QDir::Files, QDir::Time);
        while (logs.count() >= 3)
            dir.remove(logs.takeLast());  //Delete all logs except last 3.

        enabled = true;
        write(QString(LOG_SEPARATOR) + "\n"
              "         " IDA_TITLE " " IDA_VERSION " application log\n" + LOG_SEPARATOR);
    } else {
        write("Debug logging stopped");
        write(LOG_SEPARATOR);
        enabled = false;
    }
}

bool lmDebugLog::isEnabled() {
    return enabled;
}

void lmDebugLog::write(const QString& string, bool verbose) {
    if(!enabled || !verbose)
        return;

    QDir logDir(DefinitionsDir::logDir());
    if(!logDir.exists())
        logDir.mkdir(logDir.absolutePath());
    QFile file(fileName);
    if(!file.open(QIODevice::Text | QIODevice::Append))
        return;

    QTextStream stream(&file);

    QString timeStamp = "[" + QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss") + "] ";
    QStringList stringList = string.split("\n", Qt::SkipEmptyParts);
    for(int index = 0; index < stringList.count(); index++)
        stream << timeStamp << stringList[index] << "\n";

    file.close();
}
