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


#ifndef AFORMHISTORY_H
#define AFORMHISTORY_H

#include <QWidget>
#include <QList>
#include <QTreeWidget>
#include <qevent.h>
#include "ui_aformhistory.h"
#include "settings.h"
#include "history.h"
#include "messagelog.h"

class lmFormHistory : public QWidget
{
	Q_OBJECT

public:
    lmFormHistory(QWidget *parent = nullptr, Qt::WindowFlags flags = {});
	~lmFormHistory();

	void init(void);
	void updateList(void);
	void stop(void);
	void settingsChanged(void);

protected:
    bool eventFilter(QObject* pObject, QEvent* pEvent);
	void changeEvent(QEvent* pEvent);

private slots:
	void tvMsgList_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
	void btnClearHistory_clicked(void);

private:
	void setUIText(void);
	void displayList(void);

	Ui::HistoryWindow ui;
	lmSettings* pSettings;
	lmMessageLog* pMessageLog;
	QList<MsgInfo> msgList;
};

#endif // AFORMHISTORY_H
