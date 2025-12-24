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


#ifndef QMESSAGEBROWSER_H
#define QMESSAGEBROWSER_H

#include <QTextBrowser>

class QMessageBrowser : public QTextBrowser
{
    Q_OBJECT

public:
    explicit QMessageBrowser(QWidget* parent = nullptr);
    virtual ~QMessageBrowser();

    void insertMoreMessagesAnchor(const QString &text);
    void insertMoreMessagesAnchor(QTextCursor cursor, const QString &text);
    void insertMessage(QTextCursor cursor, const QString &sender, const QString &receiver, const QDateTime &time, const QString &avatarUrl, const QString &text);

    typedef struct {
        QTextCursor cursor;
        int scrollBarMaximum;
    } InsertWithoutScrollingData;

    InsertWithoutScrollingData beginInsertWithoutScrolling();
    void endInsertWithoutScrollig(InsertWithoutScrollingData data);

private slots:
    void onAnchorClicked(const QUrl &arg1);

Q_SIGNALS:
    void moreMessagesAnchorClicked();
};

#endif // QMESSAGEBROWSER_H
