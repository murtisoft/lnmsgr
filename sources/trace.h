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


#ifndef TRACE_H
#define TRACE_H

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include "definitionsdir.h"

class lmTrace {
public:
	lmTrace(void);
	~lmTrace(void);

    static void init(QString fileName, bool traceMode);
    static void write(const QString& string, bool verbose = true);
    static void stop(const QString& string);
    static bool check();

private:
	static bool traceMode;
	static QString fileName;
};

#endif // TRACE_H
