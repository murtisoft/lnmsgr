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


#include "shareduifunctions.h"
#include <QPainter.h>
#include <qapplication.h>
#include <qicon.h>
#include <qstylehints.h>
#ifdef Q_OS_WIN
#include <Windows.h>
#include <lmcons.h>
#else
#include <stdlib.h>
#endif
#include <QStringList>
#include <QFile>

int Helper::indexOf(const QString array[], int size, const QString& value) {
	for(int index = 0; index < size; index++) {
		if(value == array[index])
			return index;
	}

	return -1;
}

int Helper::statusIndexFromCode(QString status) {
	for(int index = 0; index < ST_COUNT; index++)
		if(statusCode[index].compare(status) == 0)
			return index;
	return -1;
}

QString Helper::formatSize(qint64 size) {
	qint64 gb = 1073741824;
	qint64 mb = 1048576;
	qint64 kb = 1024;

	if(size > gb)
		return QString("%1 GB").arg((double)size / gb, 0, 'f', 2);
	else if(size > mb)
		return QString("%1 MB").arg((double)size / mb, 0, 'f', 2);
	else if(size > kb)
		return QString("%1 KB").arg((double)size / kb, 0, 'f', 2);
	else
		return QString("%1 bytes").arg(size);
}

QString Helper::getUuid(void) {
	QString Uuid = QUuid::createUuid().toString();
	Uuid = Uuid.remove("{").remove("}").remove("-");
	return Uuid;
}

QString Helper::getLogonName(void) {
#if defined Q_OS_WIN	//	if platform is Windows
	TCHAR szUserName[UNLEN + 1];
	DWORD nSize = sizeof(szUserName);
    GetUserName(szUserName, &nSize);
	return QString::fromStdWString(szUserName);
#else	// this code should work for MAC and Linux
	char* szUserName;
	szUserName = getenv("USER");
	if(szUserName)
        return QString::fromLatin1(szUserName);
#endif

	return QString();
}

QString Helper::getHostName(void) {
	return QHostInfo::localHostName();
}

QString Helper::getOSName()
{
    return QSysInfo::prettyProductName();
}

QString Helper::escapeDelimiter(QString *lpszData) {
	return lpszData->replace(DELIMITER, DELIMITER_ESC, Qt::CaseSensitive);
}

QString Helper::unescapeDelimiter(QString* lpszData) {
	return lpszData->replace(DELIMITER_ESC, DELIMITER, Qt::CaseSensitive);
}

//	Returns:
//	<0 if version 1 is older
//	>0 if version 1 is newer
//	0 if both versions are same
int Helper::compareVersions(const QString& version1, const QString& version2) {
	QStringList v1 = version1.split(".", Qt::SkipEmptyParts);
	QStringList v2 = version2.split(".", Qt::SkipEmptyParts);

	//	Assuming that the version is in x.x.x format, we only need to iterate 3 times
	for(int index = 0; index < 3; index++) {
		int comp = v1[index].toInt() - v2[index].toInt();
		if(comp != 0)
			return comp;
	}

	return 0;
}

QString Helper::boolToString(bool value) {
    return value ? LM_TRUE : LM_FALSE;
}

bool Helper::stringToBool(const QString& value) {
    return value.compare(LM_TRUE) == 0 ? true : false;
}

//	Function that copies content of source to destination
//	Destination file will be overwritten
//	Supports only small files
bool Helper::copyFile(const QString& source, const QString& destination) {
	QFile srcFile(source);
	if(!srcFile.open(QIODevice::ReadOnly))
		return false;

	QByteArray data = srcFile.readAll();
	srcFile.close();

	QFile destFile(destination);
	if(!destFile.open(QIODevice::WriteOnly))
		return false;

	destFile.write(data);
	destFile.close();

	return true;
}

void Helper::changeColorScheme(int index){
    switch (index) {
    case 0:
        qApp->styleHints()->setColorScheme(Qt::ColorScheme::Unknown);
        break;
    case 1:
        qApp->styleHints()->setColorScheme(Qt::ColorScheme::Light);
        break;
    case 2:
        qApp->styleHints()->setColorScheme(Qt::ColorScheme::Dark);
        break;
    default:
        break;
    }
    QEvent event(QEvent::ThemeChange);
    QCoreApplication::sendEvent(qApp, &event);
}

//Renders emojis to a pixmap so they can be used as an icon. Unicode, so every OS has its own flavor.
//Using colorful emojis, i am deliberately avoiding garbage monochrome brutalist iconpacks of the last decade and a half.
//Man, i could write a fucking manifesto about this...
QIcon Helper::renderEmoji(const QString& emoji, int size, bool strikethrough) {
    QPixmap temp(size * 2, size * 2);
    temp.fill(Qt::transparent);
    QPainter p(&temp);
    QFont font = p.font();
    font.setPointSize(qRound(size * 0.7));
    p.setFont(font);
    p.drawText(temp.rect(), Qt::AlignCenter, emoji);
    p.end();                                                                        //Render to a double size canvas

    QImage img = temp.toImage();
    int top = img.height(), bottom = 0, left = img.width(), right = 0;
    for (int y = 0; y < img.height(); y++)
        for (int x = 0; x < img.width(); x++)
            if (qAlpha(img.pixel(x, y)) > 0) {
                top = qMin(top, y); bottom = qMax(bottom, y);
                left = qMin(left, x); right = qMax(right, x);
            }

    QPixmap cropped = temp.copy(left, top, right - left + 1, bottom - top + 1);

    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    int ox = (size - cropped.width()) / 2;
    int oy = (size - cropped.height()) / 2;
    painter.drawPixmap(ox, oy, cropped);                                            //Center and crop to actual canvas.
                                                                                    //Because centering vector emojis is goddamn impossible.
    if (strikethrough) {
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(Qt::red, qMax(2, size / 12), Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(pixmap.rect().topLeft(), pixmap.rect().bottomRight());
    }

    return QIcon(pixmap);
}
