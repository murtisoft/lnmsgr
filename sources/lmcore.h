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


#ifndef LMCORE_H
#define LMCORE_H

#include <QObject>
#include <QTimer>
#include <QSysInfo>
#include <QPointer>
#include "definitionssettings.h"
#include "messaging.h"
#include "aformmain.h"
#include "aformchat.h"
#include "aformtransfer.h"
#include "aformhistory.h"
#include "aformsettings.h"
#include "aformuserinfo.h"
#include "aformchatroom.h"
#include "aformuserselect.h"
#include "aformabout.h"
#include "aformbroadcast.h"
#include "networkstreamer.h"
#include "soundplayer.h"
#include "hotkeyhandler.h"

//	Command line options, as passed between main() and lmCore (and between instances).
//	These are the parsed, canonical forms of the --switches, not raw user input.
#define LM_OPT_TRACE			"trace"
#define LM_OPT_PORT				"port"		//	sent as "port=<number>"
#define LM_OPT_CONFIG			"config"		//	sent as "config=<absolute path>"
#define LM_OPT_LOOPBACK			"loopback"
#define LM_OPT_AUTOSTART		"autostart"	//	sent as "autostart=true" or "autostart=false"
#define LM_OPT_RESET_HISTORY	"reset-history"
#define LM_OPT_RESET_FILEHIST	"reset-file-history"
#define LM_OPT_RESET_CONFIG		"reset-config"
#define LM_OPT_TERMINATE		"terminate"
#define LM_OPT_EXIT_AFTER_ARGS	"exit-after-args"
#define LM_OPT_NEW				"new"		//	internal: this is a freshly started instance

class lmCore : public QObject {
	Q_OBJECT

public:
	lmCore(void);
	~lmCore(void);
	void init(const QStringList& options);
	QStringList startupWarnings;	//	non-fatal problems found while starting, for the caller to report
	bool start(void);
    void playLoopSound(SoundEvent event);
    void stopLoopSound();
    CallPhase callPhase = CP_Idle;

public slots:
	bool receiveAppMessage(const QStringList& options);

signals:
    void callPhaseChanged(CallPhase phase);

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
	void refreshBroadcastUsers(void);
	void showBroadcast(void);
	void showPublicChat();
	void historyCleared(void);
	void fileHistoryCleared(void);
	void showTrayMessage(TrayMessageType type, QString szMessage, QString szTitle = QString(), TrayMessageIcon icon = TMI_Info);
    void addContacts(QStringList *pExcludList);
	void chatWindow_closed(QString* lpszUserId);
	void chatRoomWindow_closed(QString* lpszThreadId);
    void callRequested(MessageType type);
    void callConnected(MessageType type);
    void callEnded(MessageType type);
    void micTestChanged(bool testing);
    void hotkeyTriggered(lmHotkeyHandler::Action action);
    void hotkeyReleased(lmHotkeyHandler::Action action);
    void hotkeyRegistrationFailed(lmHotkeyHandler::Action action, const QKeySequence& sequence, const QString& reason);

private:
	void stop(void);
	void loadSettings(void);
	void settingsChanged(void);
    void processStream(MessageType type, QString *lpszUserId, MessageXml* pMessage);
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
    lmFormChat* callWindow(void) const;
    QByteArray micDeviceSetting(void) const;
    int micActivationSetting(void) const;
    void applyMicActivation(void);

    lmSettings*                     pSettings;
	QTimer*							pTimer;
	lmMessaging*					pMessaging;
    lmFormMain*                     pMainWindow;
    QList<lmFormChat*>              chatWindows;
    QList<lmFormChatRoom*>          chatRoomWindows;
    lmFormTransfer*                 pTransferWindow;
    QPointer<lmFormHistory>         pHistoryWindow;
	QPointer<lmFormSettings>		pSettingsDialog;
	QPointer<lmFormUserInfo>		pUserInfoWindow;
	QPointer<lmFormChatRoom>		pPublicChatWindow;
    QPointer<lmFormUserSelect>      pUserSelectDialog;
    QPointer<lmFormAbout>           pAboutDialog;
    QPointer<lmFormBroadcast>       pBroadcastWindow;
	bool							messageTop;
	bool							pubMessagePop;
	QString							lang;
	bool							adaptiveRefresh;
	int								refreshTime;
	MessageXml*						pInitParams;
    lmAudioStream*                  m_audioStream = nullptr;
    QString                         callPeerId;     // user the current call (calling/ringing/connected) is with
    lmSoundPlayer*                  pSoundPlayer;
    lmHotkeyHandler*                pHotkeys;
    bool                            pttSoundPlayed = false;      // push to talk went down with a sound, so it comes up with one
    bool                            micMutedByTest = false;     // the call mic was muted for the preferences mic test
    QMediaDevices*                  pMediaDevices;   // main thread, tells the call when mics come and go
};

#endif // LMCORE_H
