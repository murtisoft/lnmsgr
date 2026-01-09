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


#include "trace.h"

bool lmTrace::traceMode;
QString lmTrace::fileName;

lmTrace::lmTrace(void) {
}

lmTrace::~lmTrace(void) {
}

void lmTrace::init(QString fileName, bool traceMode) {
    lmTrace::traceMode = traceMode;
    lmTrace::fileName = fileName;

    QDir dir(DefinitionsDir::logDir());
    QStringList logs = dir.entryList({"*.log"}, QDir::Files, QDir::Time);

    while (logs.count() >= 3) {
        dir.remove(logs.takeLast());  //Delete all logs except last 3.
    }

    write("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n"\
          "         " IDA_TITLE " " IDA_VERSION " application log\n"\
          "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-");
}

void lmTrace::write(const QString& string, bool verbose) {
    if(!traceMode || !verbose)
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

void lmTrace::stop(const QString& string) {
    if (!traceMode) return;
    write(string);
    write("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-");
    lmTrace::traceMode = false;
}

bool lmTrace::check(){
    return lmTrace::traceMode;
}
