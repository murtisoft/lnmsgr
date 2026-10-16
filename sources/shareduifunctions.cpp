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
#include <QPointer>
#include <QScreen>
#include <QGuiApplication>
#include <QWidget>
#include <QHash>
#include <QImage>
#include <QPixmap>
#include <QPen>
#include <QtMath>

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

//	Group names are case insensitive and always shown with the first letter of every word capitalized.
//	So "it dept", "IT DEPT" and "It  Dept " all become "It Dept". An empty name becomes the default group.
//	QString::toLower/toUpper are used on purpose, they do not depend on the system locale, so every peer
//	comes up with the same name for the same input.
QString Helper::normalizeGroupName(const QString& name) {
	QStringList words = name.simplified().toLower().split(' ', Qt::SkipEmptyParts);
	for(QString& word : words)
		word[0] = word.at(0).toUpper();

	QString result = words.join(' ');
	return result.isEmpty() ? QString(GRP_DEFAULT) : result;
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

//Renders one emoji at a given device pixel scale to an image of size*scale pixels square.
static QImage renderEmojiImage(const QString& emoji, int size, int scale, bool strikethrough) {
    const int target = size * scale;
    //Pixel size of the font. A pixel size does not depend on the screen dpi, unlike a point size.
    //0.933 is what a 0.7 * size point font comes to at 96 dpi, so the icons keep the look they had.
    const int fontPixels = qMax(1, qRound(target * 0.933));

    QImage canvas(target * 2, target * 2, QImage::Format_ARGB32_Premultiplied);
    canvas.fill(Qt::transparent);
    QPainter p(&canvas);
    QFont font = p.font();
    font.setPixelSize(fontPixels);
    p.setFont(font);
    p.drawText(canvas.rect(), Qt::AlignCenter, emoji);
    p.end();                                                                        //Render to a double size canvas

    QImage result(target, target, QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);

    //Find the bounds of what was actually drawn
    int top = canvas.height(), bottom = -1, left = canvas.width(), right = -1;
    for (int y = 0; y < canvas.height(); y++) {
        const QRgb* line = reinterpret_cast<const QRgb*>(canvas.constScanLine(y));
        for (int x = 0; x < canvas.width(); x++) {
            if (qAlpha(line[x]) > 0) {
                top = qMin(top, y); bottom = qMax(bottom, y);
                left = qMin(left, x); right = qMax(right, x);
            }
        }
    }

    if (bottom >= 0) {                                                              //Nothing drawn (empty text, missing glyph) leaves a blank icon.
        QImage cropped = canvas.copy(left, top, right - left + 1, bottom - top + 1);
        if (cropped.width() > target || cropped.height() > target)                  //A glyph larger than the icon is scaled down, not clipped.
            cropped = cropped.scaled(target, target, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QPainter painter(&result);
        painter.drawImage((target - cropped.width()) / 2, (target - cropped.height()) / 2, cropped);
        painter.end();                                                              //Center and crop to actual canvas.
                                                                                    //Because centering vector emojis is goddamn impossible.
    }

    if (strikethrough) {
        QPainter painter(&result);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(Qt::red, qMax(2, size / 12) * scale, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPoint(0, 0), QPoint(target - 1, target - 1));
    }

    return result;
}

//Renders emojis to a pixmap so they can be used as an icon. Unicode, so every OS has its own flavor.
//Using colorful emojis, i am deliberately avoiding garbage monochrome brutalist iconpacks of the last decade and a half.
//Man, i could write a fucking manifesto about this...
QIcon Helper::renderEmoji(const QString& emoji, int size, bool strikethrough) {
    if (size <= 0)
        return QIcon();

    //The highest pixel ratio of any screen. The icon gets a pixmap for every whole ratio up to it, so it stays sharp
    //on high dpi screens, and when a window is dragged between screens of different ratios.
    int maxScale = 1;
    for (const QScreen* pScreen : QGuiApplication::screens())
        maxScale = qMax(maxScale, qCeil(pScreen->devicePixelRatio()));

    //Icons are asked for again and again (avatars on every user list refresh, the mute buttons...). Rendering is
    //not free, so render once. Like all pixmap work this belongs to the GUI thread, so no locking is needed.
    static QHash<QString, QIcon> cache;
    const QString key = emoji + QChar(0x1F) + QString::number(size) + QChar(0x1F)
                      + QString::number(strikethrough ? 1 : 0) + QChar(0x1F) + QString::number(maxScale);
    auto it = cache.constFind(key);
    if (it != cache.constEnd())
        return it.value();

    QIcon icon;
    for (int scale = 1; scale <= maxScale; scale++) {
        QPixmap pixmap = QPixmap::fromImage(renderEmojiImage(emoji, size, scale, strikethrough));
        pixmap.setDevicePixelRatio(scale);
        icon.addPixmap(pixmap);
    }

    cache.insert(key, icon);
    return icon;
}

static QPointer<QScreen> s_pMainScreen;

void Helper::setMainScreen(QScreen* pScreen) {
	if(pScreen)
		s_pMainScreen = pScreen;
}

void Helper::placeOnMainScreen(QWidget* pWindow) {
	QScreen* pMain = s_pMainScreen;
	if(!pWindow || !pMain || pWindow->isVisible())
		return;

	//	A window that was never moved has no position of its own yet, so the system would pick one
	//	(usually the screen with the cursor). One that was restored from settings keeps its spot as long
	//	as that spot is on the main window's screen.
	if(pWindow->testAttribute(Qt::WA_Moved) && QGuiApplication::screenAt(pWindow->geometry().center()) == pMain)
		return;

	QRect avail = pMain->availableGeometry();
	QSize size = pWindow->size().boundedTo(avail.size());
	if(size != pWindow->size())
		pWindow->resize(size);
	QRect rect(QPoint(0, 0), size);
	rect.moveCenter(avail.center());
	pWindow->move(rect.topLeft());
}
