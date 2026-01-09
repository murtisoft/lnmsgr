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


#include <QFile>
#include <QDataStream>
#include <QPainter.h>
#include <qicon.h>
#include <qregularexpression.h>
#include "chathelper.h"

QDataStream &operator << (QDataStream &out, const SingleMessage &message) {
    out << qint32(message.type) << message.userId << message.userName << message.message.toString()
        << message.id;
    return out;
}

QDataStream &operator >> (QDataStream &in, SingleMessage &message) {
    qint32 type;
    QString userId;
    QString userName;
    QString xmlMessage;
    QString id;
    in >> type >> userId >> userName >> xmlMessage >> id;
    message = SingleMessage((MessageType)type, userId, userName, MessageXml(xmlMessage), id);
    return in;
}

void ChatHelper::makeHtmlSafe(QString* lpszMessage) {
	for(int index = 0; index < HTMLESC_COUNT; index++)
		lpszMessage->replace(htmlSymbol[index], htmlEscape[index]);
}

void ChatHelper::encodeSmileys(QString* lpszMessage) {
    //	replace all emoticon images with corresponding text code
    for (int index = 0; index < SM_MAPCOUNT; index++) {
        QString code = smileyCode[index];
        makeHtmlSafe(&code);

        if (smileyEmoji[index].startsWith(":/")) {
            lpszMessage->replace("<img src=\"" + smileyEmoji[index] + "\" />", code); //Legacy png handler
        } else {
            // Replaces only if NOT preceded or followed by a Zero Width Joiner
            QRegularExpression lonelyEmoji("(?<!\\x{200D})" + QRegularExpression::escape(smileyEmoji[index]) + "(?!\\x{200D})");
            lpszMessage->replace(lonelyEmoji, code);
        }
    }
}

void ChatHelper::decodeSmileys(QString* lpszMessage) {
    // Handle any other unicode emoji that is not on the list, first. This doesnt handle everything because unicode emojis are a fucking mess.
    static const QRegularExpression emojiRegex("(\\p{Emoji_Presentation}(?:\\x{200D}\\p{Emoji_Presentation})*)");
    lpszMessage->replace(emojiRegex, "&#8203;<span style='font-size:18px; vertical-align: middle;'>\\1</span>&#8203;");

    //	replace text emoticons with corresponding images
    for (int index = 0; index < SM_MAPCOUNT; index++) {
        QString code = smileyCode[index];
        makeHtmlSafe(&code);

        if (smileyEmoji[index].startsWith(":/")) {
            lpszMessage->replace(code, "<img src='qrc" + smileyEmoji[index] + "'/>", Qt::CaseInsensitive);  //Legacy png handler
        } else {
            lpszMessage->replace(code, "&#8203;<span style='font-size:18px; vertical-align: middle;'>" + smileyEmoji[index] + "</span>&#8203;", Qt::CaseInsensitive);
        }
    }
}

//Renders emojis to a pixmap so they can be used as an icon. Unicode, so every OS has its own flavor.
//Using colorful emojis, i am deliberately avoiding garbage monochrome brutalist iconpacks of the last decade and a half.
//Man, i could write a fucking manifesto about this...
QIcon ChatHelper::renderEmoji(const QString& emoji, int size) {
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    QFont font = painter.font();
    font.setPointSize(qRound(size * 0.7));
    painter.setFont(font);

    painter.drawText(pixmap.rect(), Qt::AlignCenter, emoji);
    return QIcon(pixmap);
}

QTextBlockData::QTextBlockData(QString id)
{
    this->id = id;
}

QTextBlockData::~QTextBlockData()
{

}

