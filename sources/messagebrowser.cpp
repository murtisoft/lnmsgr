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


#include "messagebrowser.h"

#include <QScrollBar>

MessageBrowser::MessageBrowser(QWidget* parent)
    : QTextBrowser (parent)
{
    setOpenLinks(false);
    setOpenExternalLinks(true);

    connect(this, SIGNAL(anchorClicked(const QUrl &)), this, SLOT(onAnchorClicked(const QUrl &)));
}

MessageBrowser::~MessageBrowser()
{

}

void MessageBrowser::insertMessage(QTextCursor cursor, const QString &sender, const QString &receiver, const QDateTime &time, const QString &avatarUrl, const QString &text)
{
    QString html = "<table border='0' width='100%'><tr><td rowspan='2'><img src='%AVATAR_URL%'></td><td style='background: #0000ff;font-size: 2px;'></td></tr><tr><td width='100%' style='padding-left: 10px;'>%TEXT%</td></tr></table>";

    html.replace("%SENDER%", sender);
    html.replace("%RECEIVER%", receiver);
    html.replace("%TIME%", "11:55 PM");
    html.replace("%AVATAR_URL%", avatarUrl);
    html.replace("%TEXT%", text);

    cursor.insertHtml(html);
}

MessageBrowser::InsertWithoutScrollingData MessageBrowser::beginInsertWithoutScrolling()
{
    InsertWithoutScrollingData data;

    QScrollBar *scrollBar = verticalScrollBar();
    data.scrollBarMaximum = scrollBar->maximum();

    setUpdatesEnabled(false);
    data.cursor = textCursor();

    return data;
}

void MessageBrowser::endInsertWithoutScrollig(InsertWithoutScrollingData data)
{
    setTextCursor(data.cursor);
    setUpdatesEnabled(true);

    QScrollBar *scrollBar = verticalScrollBar();
    int delta = scrollBar->maximum() - data.scrollBarMaximum;
    scrollBar->setValue(delta);
}

void MessageBrowser::insertMoreMessagesAnchor(const QString &text)
{
    QTextCursor cursor = textCursor();
    moveCursor(QTextCursor::MoveOperation::Start);
    insertMoreMessagesAnchor(cursor, text);
}

void MessageBrowser::insertMoreMessagesAnchor(QTextCursor cursor, const QString &text)
{
    cursor.insertHtml(QString("<a href='#more_messages'>%1</a>").arg(text));
}

void MessageBrowser::onAnchorClicked(const QUrl &arg1)
{
    if(arg1.toString() == "#more_messages") {

        QTextCursor cursor = textCursor();

        cursor.select(QTextCursor::SelectionType::BlockUnderCursor);
        cursor.removeSelectedText();

        emit moreMessagesAnchorClicked();
    }
    // failed down
//    else {
//    //    emit QTextBrowser::anchorClicked(arg1);
//    }
}
