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


#include <QFile>
#include <QDataStream>
#include <qregularexpression.h>
#include "sharedchatfunctions.h"
#include "definitionsui.h"

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
    //  The html safe codes and their replacements do not change, build them once instead of for every message.
    struct SmileyEntry {
        QString code;
        QString replacement;
    };
    static const QList<SmileyEntry> entries = []() {
        QList<SmileyEntry> list;
        list.reserve(SM_MAPCOUNT);
        for (int index = 0; index < SM_MAPCOUNT; index++) {
            QString code = smileyCode[index];
            makeHtmlSafe(&code);
            if (smileyEmoji[index].startsWith(":/"))
                list.append({code, "<img src='qrc" + smileyEmoji[index] + "'/>"});     //Legacy png handler
            else
                list.append({code, "&#8203;<span style='font-size:18px; vertical-align: middle;'>" + smileyEmoji[index] + "</span>&#8203;"});
        }
        return list;
    }();
    //  Characters a code can start with. A message without any of them has no text smileys, no need to look for all 81.
    static const QList<bool> startsCode = []() {
        QList<bool> table(128, false);
        for (const SmileyEntry& entry : entries) {
            ushort first = entry.code.at(0).unicode();
            if (first < 128) {
                table[first] = true;
                table[QChar(first).toLower().unicode()] = true;     //  codes are matched case insensitive
                table[QChar(first).toUpper().unicode()] = true;
            }
        }
        return table;
    }();

    // Handle any other unicode emoji that is not on the list, first. This doesnt handle everything because unicode emojis are a fucking mess.
    //  The lowest emoji presentation character is U+231A, plain text without anything above it can skip the regex.
    bool maybeEmoji = false;
    for (QChar ch : std::as_const(*lpszMessage))
        if (ch.unicode() >= 0x231A) {
            maybeEmoji = true;
            break;
        }
    if (maybeEmoji) {
        static const QRegularExpression emojiRegex("(\\p{Emoji_Presentation}(?:\\x{200D}\\p{Emoji_Presentation})*)");
        lpszMessage->replace(emojiRegex, "&#8203;<span style='font-size:18px; vertical-align: middle;'>\\1</span>&#8203;");
    }

    //	replace text emoticons with corresponding images
    bool maybeSmiley = false;
    for (QChar ch : std::as_const(*lpszMessage)) {
        ushort u = ch.unicode();
        if (u >= 128 || startsCode.at(u)) {
            maybeSmiley = true;
            break;
        }
    }
    if (!maybeSmiley)
        return;

    for (const SmileyEntry& entry : entries)
        lpszMessage->replace(entry.code, entry.replacement, Qt::CaseInsensitive);
}

QTextBlockData::QTextBlockData(QString id)
{
    this->id = id;
}

QTextBlockData::~QTextBlockData()
{

}

