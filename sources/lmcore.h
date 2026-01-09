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


#ifndef LMCORE_H
#define LMCORE_H

#include <QObject>
#include <QTimer>
#include <QSysInfo>
#include <QPointer>
#include "shared.h"
#include "settings.h"
#include "messaging.h"
#include "aformmain.h"
#include "aformchat.h"
#include "history.h"
#include "definitionsdir.h"
#include "aformtransfer.h"
#include "aformhistory.h"
#include "aformsettings.h"
#include "aformuserinfo.h"
#include "aformchatroom.h"
#include "aformuserselect.h"
#include "aformabout.h"
#include "aformbroadcast.h"

class lmCore : public QObject {
	Q_OBJECT

public:
	lmCore(void);
	~lmCore(void);
	void init(const QString& szCommandArgs);
	bool start(void);

public slots:
	bool receiveAppMessage(const QString& szMessage);

private slots:
	void exitApp(void);
	void aboutToExit(void);
	void timer_timeout(void);
	void startChat(QString* lpszUserId);
	void startChatRoom(QString* lpszThreadId);
	void sendMessage(MessageType type, QString* lpszUserId, MessageXml* pMessage);
	void receiveMessage(MessageType type, QString* lpszUserId, MessageXml* pMessage);
	void connectionStateChanged(void);
	void showTransfers(void);
	void showHistory(void);
	void showSettings(void);
	void showAbout(void);
	void showBroadcast(void);
	void showPublicChat();
	void historyCleared(void);
	void fileHistoryCleared(void);
	void showTrayMessage(TrayMessageType type, QString szMessage, QString szTitle = QString(), TrayMessageIcon icon = TMI_Info);
	void updateGroup(GroupOp op, QVariant value1, QVariant value2);
    void addContacts(QStringList *pExcludList);
	void chatWindow_closed(QString* lpszUserId);
	void chatRoomWindow_closed(QString* lpszThreadId);

private:
	void stop(void);
	void loadSettings(void);
	void settingsChanged(void);
	void processMessage(MessageType type, QString* lpszUserId, MessageXml* pMessage);
    void processFile(MessageType type, QString* lpszUserId, MessageXml* pMessage);
	void routeMessage(MessageType type, QString* lpszUserId, MessageXml* pMessage);
	void routeGroupMessage(MessageType type, QString* lpszUserId, MessageXml* pMessage);
	void processPublicMessage(MessageType type, QString* lpszUserId, MessageXml* pMessage);
    void createTransferWindow(void);
	void showTransferWindow(bool show = false);
    void initFileTransfer(MessageType type, FileMode mode, QString* lpszUserId, MessageXml* pMessage);
	void showUserInfo(MessageXml* pMessage);
	void createChatWindow(QString* lpszUserId);
	void showChatWindow(lmFormChat* chatWindow, bool show, bool alert = false);
	void createChatRoomWindow(QString* lpszThreadId);
	void showChatRoomWindow(lmFormChatRoom* chatRoomWindow, bool show, bool alert = false, bool add = false);
	void showPublicChatWindow(bool show, bool alert = false, bool open = false);
    QStringList showSelectContacts(QWidget* parent, uint caps, QStringList* excludeList);
	void showPortConflictMessage(void);

	lmSettings*					pSettings;
	QTimer*							pTimer;
	lmMessaging*					pMessaging;
	lmFormMain*					pMainWindow;
	QList<lmFormChat*>			chatWindows;
	QList<lmFormChatRoom*>		chatRoomWindows;
	lmFormTransfer*				pTransferWindow;
	QPointer<lmFormHistory>		pHistoryWindow;
	QPointer<lmFormSettings>		pSettingsDialog;
	QPointer<lmFormUserInfo>		pUserInfoWindow;
	QPointer<lmFormChatRoom>		pPublicChatWindow;
	QPointer<lmFormUserSelect>	pUserSelectDialog;
	QPointer<lmFormAbout>		pAboutDialog;
	QPointer<lmFormBroadcast>	pBroadcastWindow;
	bool							messageTop;
	bool							pubMessagePop;
	QString							lang;
	bool							adaptiveRefresh;
	int								refreshTime;
	MessageXml*						pInitParams;
};

#endif // LMCORE_H
