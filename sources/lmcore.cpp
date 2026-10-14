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


#include <QMessageBox>
#include <QTranslator>
#include <QGuiApplication>
#include <utilapiset.h>
#include "zdebuglog.h"
#include "lmcore.h"
#include "translationloader.h"
#include "translatabletext.h"
#include "definitionsdir.h"

lmCore::lmCore(void) {
	pMessaging = new lmMessaging();
    connect(pMessaging, SIGNAL(messageReceived(MessageType,QString*,MessageXml*)),
        this, SLOT(receiveMessage(MessageType,QString*,MessageXml*)));
	connect(pMessaging, SIGNAL(connectionStateChanged()), this, SLOT(connectionStateChanged()));
	pMainWindow = new lmFormMain();
	connect(pMainWindow, SIGNAL(appExiting()), this, SLOT(exitApp()));
	connect(pMainWindow, SIGNAL(chatStarting(QString*)), this, SLOT(startChat(QString*)));
	connect(pMainWindow, SIGNAL(chatRoomStarting(QString*)), this, SLOT(startChatRoom(QString*)));
    connect(pMainWindow, SIGNAL(messageSent(MessageType,QString*,MessageXml*)),
        this, SLOT(sendMessage(MessageType,QString*,MessageXml*)));
	connect(pMainWindow, SIGNAL(showTransfers()), this, SLOT(showTransfers()));
	connect(pMainWindow, SIGNAL(showHistory()), this, SLOT(showHistory()));
	connect(pMainWindow, SIGNAL(showSettings()), this, SLOT(showSettings()));
	connect(pMainWindow, SIGNAL(showAbout()), this, SLOT(showAbout()));
	connect(pMainWindow, SIGNAL(showBroadcast()), this, SLOT(showBroadcast()));
	connect(pMainWindow, SIGNAL(userListChanged()), this, SLOT(refreshBroadcastUsers()));
	connect(pMainWindow, SIGNAL(userListChanged()), this, SLOT(refreshSelectUsers()));
	connect(pMainWindow, SIGNAL(showPublicChat()), this, SLOT(showPublicChat()));
	pPublicChatWindow = new lmFormChatRoom();
    connect(pPublicChatWindow, SIGNAL(messageSent(MessageType,QString*,MessageXml*)),
        this, SLOT(sendMessage(MessageType,QString*,MessageXml*)));
	connect(pPublicChatWindow, SIGNAL(chatStarting(QString*)), this, SLOT(startChat(QString*)));
	chatWindows.clear();
	chatRoomWindows.clear();
    pTransferWindow = nullptr;
    pHistoryWindow = nullptr;
    pSettingsDialog = nullptr;
    pUserInfoWindow = nullptr;
    pUserSelectDialog = nullptr;
    selectCaps = 0;
    pAboutDialog = nullptr;
    pBroadcastWindow = nullptr;
    pTimer = nullptr;
    pSoundPlayer = new lmSoundPlayer();
    pHotkeys = new lmHotkeyHandler(this);
    connect(pHotkeys, &lmHotkeyHandler::triggered, this, &lmCore::hotkeyTriggered);
    connect(pHotkeys, &lmHotkeyHandler::released, this, &lmCore::hotkeyReleased);
    connect(pHotkeys, &lmHotkeyHandler::registrationFailed, this, &lmCore::hotkeyRegistrationFailed);
    pMediaDevices = new QMediaDevices(this);
    connect(pMediaDevices, &QMediaDevices::audioInputsChanged, this, [this]() {
        if (m_audioStream)
            m_audioStream->refreshMicDevice();      // unplugged or plugged in during a call
    });
    //  Cameras and desktops come and go during a call too
    auto refreshVideo = [this]() {
        if (m_videoCapture)
            m_videoCapture->refresh();
    };
    connect(pMediaDevices, &QMediaDevices::videoInputsChanged, this, refreshVideo);
    connect(qGuiApp, &QGuiApplication::screenAdded, this, refreshVideo);
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, refreshVideo);
}

lmCore::~lmCore(void) {
}

void lmCore::init(const QStringList& options) {
	//	prevent auto app exit when last visible window is closed
	qApp->setQuitOnLastWindowClosed(false);

	pInitParams = new MessageXml();
    pInitParams->addData(XN_LOGFILE, DefinitionsDir::freeLogFile()); //Define path no matter what so logging can be initialized later.
    if(options.contains(LM_OPT_TRACE)) {
        pInitParams->addData(XN_TRACEMODE, LM_TRUE);     //Commandline argument will override UI setting.
	}
	const QString portPrefix = QString(LM_OPT_PORT) + "=";
	const QString configPrefix = QString(LM_OPT_CONFIG) + "=";
	for(const QString& option : options) {
		if(option.startsWith(portPrefix))
			pInitParams->addData(XN_PORT, option.mid(portPrefix.length()));
		else if(option.startsWith(configPrefix))
			pInitParams->addData(XN_CONFIG, option.mid(configPrefix.length()));
	}

    lmDebugLog::init(pInitParams->data(XN_LOGFILE), Helper::stringToBool(pInitParams->data(XN_TRACEMODE)));
    lmDebugLog::write("Application initialized");

    loadSettings();
    lmDebugLog::write("Settings loaded");

	pMessaging->init(pInitParams);
	pMainWindow->init(pMessaging->localUser, pMessaging->isConnected());
	pPublicChatWindow->init(pMessaging->localUser, pMessaging->isConnected());
}

bool lmCore::start(void) {
	lmDebugLog::write("Application started");
	pMessaging->start();

	if(pMessaging->isConnected() && !pMessaging->canReceive()) {
		showPortConflictMessage();
		//	stop the application
		stop();
		return false;
	}

	pMainWindow->start();
	pHotkeys->loadSettings(pSettings);	//	after the main window, a conflict is reported as a toast
	
	pTimer = new QTimer(this);
    connect(pTimer, SIGNAL(timeout()), this, SLOT(timer_timeout()));
	//	Set the timer to trigger 10 seconds after the application starts. After the
	//	first trigger, the timeout period will be decided by user settings.
	adaptiveRefresh = false;
	pTimer->start(10000);
	bool autoStart = pSettings->value(IDS_AUTOSTART, IDS_AUTOSTART_VAL).toBool();
	lmSettings::setAutoStart(autoStart);

	return true;
}

//	This is the initial point where settings are used in the application
void lmCore::loadSettings(void) {
	pSettings = new lmSettings();

    if(pSettings->value(IDS_DEBUGLOG, IDS_DEBUGLOG_VAL).toBool())
        lmDebugLog::setEnabled(true);    //--trace may already have enabled it for this session

	auto warn = [this](const QString& message) {
		startupWarnings << message;
		lmDebugLog::write(message);
	};
	if(!pSettings->migrateSettings()) {
		// settings were reset
		warn(tr("Your preferences file is corrupt or invalid. %1 is unable to recover your settings.").arg(lmStrings::appName()));
	}
	if(pInitParams->dataExists(XN_CONFIG)) {
		QString configFile = pInitParams->data(XN_CONFIG);
		if(!pSettings->loadFromConfig(configFile)) {
			warn(tr("Preferences could not be imported from '%1'. File may not exist, or may not be compatible with this version of %2.")
				.arg(configFile, lmStrings::appName()));
		}
	}
	lang = pSettings->value(IDS_LANGUAGE, IDS_LANGUAGE_VAL).toString();
	Application::setLanguage(lang);
	Application::setLayoutDirection(tr("LAYOUT_DIRECTION") == RTL_LAYOUT ? Qt::RightToLeft : Qt::LeftToRight);
	messageTop = pSettings->value(IDS_MESSAGETOP, IDS_MESSAGETOP_VAL).toBool();
	pubMessagePop = pSettings->value(IDS_PUBMESSAGEPOP, IDS_PUBMESSAGEPOP_VAL).toBool();
	refreshTime = pSettings->value(IDS_REFRESHTIME, IDS_REFRESHTIME_VAL).toInt() * 1000;
}

//	Called after the preferences file was deleted. Open chat windows would otherwise keep their size
//	and save it again when they close.
void lmCore::resetWindowGeometry(void) {
	if(pPublicChatWindow)
		pPublicChatWindow->resetGeometry();
	for(int index = 0; index < chatWindows.count(); index++)
		chatWindows[index]->resetGeometry();
	for(int index = 0; index < chatRoomWindows.count(); index++)
		chatRoomWindows[index]->resetGeometry();
}

void lmCore::settingsChanged(void) {
	pMessaging->settingsChanged();
	if(pSoundPlayer)
		pSoundPlayer->settingsChanged();
	pMainWindow->settingsChanged();
	if(pPublicChatWindow)
		pPublicChatWindow->settingsChanged();
	for(int index = 0; index < chatWindows.count(); index++)
		chatWindows[index]->settingsChanged();
	for(int index = 0; index < chatRoomWindows.count(); index++)
		chatRoomWindows[index]->settingsChanged();
	if(pTransferWindow)
		pTransferWindow->settingsChanged();
	if(pUserInfoWindow)
		pUserInfoWindow->settingsChanged();
	if(pHistoryWindow)
		pHistoryWindow->settingsChanged();
	if(pSettingsDialog)
		pSettingsDialog->settingsChanged();
	if(pAboutDialog)
		pAboutDialog->settingsChanged();
	if(pBroadcastWindow)
		pBroadcastWindow->settingsChanged();

	messageTop = pSettings->value(IDS_MESSAGETOP, IDS_MESSAGETOP_VAL).toBool();
	pubMessagePop = pSettings->value(IDS_PUBMESSAGEPOP, IDS_PUBMESSAGEPOP_VAL).toBool();
	refreshTime = pSettings->value(IDS_REFRESHTIME, IDS_REFRESHTIME_VAL).toInt() * 1000;
	pTimer->setInterval(refreshTime);
	bool autoStart = pSettings->value(IDS_AUTOSTART, IDS_AUTOSTART_VAL).toBool();
	lmSettings::setAutoStart(autoStart);
	pHotkeys->loadSettings(pSettings);
	if(m_audioStream) {
		m_audioStream->setMicDevice(micDeviceSetting());	//	switch microphone mid-call
		applyMicActivation();
	}
	if(m_videoCapture)
		m_videoCapture->setSource(camDeviceSetting());	//	switch camera or desktop mid-call
	QString appLang = pSettings->value(IDS_LANGUAGE, IDS_LANGUAGE_VAL).toString();
	if(appLang.compare(lang) != 0) {
		lang = appLang;
		Application::setLanguage(lang);
		Application::setLayoutDirection(tr("LAYOUT_DIRECTION") == RTL_LAYOUT ? Qt::RightToLeft : Qt::LeftToRight);
		lmStrings::retranslate();
	}

}

void lmCore::stop(void) {
	pHotkeys->clear();
	for(int index = 0; index < chatWindows.count(); index++) {
        chatWindows[index]->btnHangUp_clicked();
		chatWindows[index]->stop();
        chatWindows[index]->deleteLater();
    }

	for(int index = 0; index < chatRoomWindows.count(); index++) {
		chatRoomWindows[index]->stop();
        chatRoomWindows[index]->deleteLater();
	}

	if(pTransferWindow) {
		pTransferWindow->stop();
        pTransferWindow->deleteLater();
	}

	if(pHistoryWindow) {
		pHistoryWindow->stop();
        pHistoryWindow->deleteLater();
	}

	if(pUserInfoWindow)
        pUserInfoWindow->deleteLater();

	if(pBroadcastWindow) {
		pBroadcastWindow->stop();
        pBroadcastWindow->deleteLater();
	}

	if(pPublicChatWindow) {
		pPublicChatWindow->stop();
        pPublicChatWindow->deleteLater();
	}

	if(pTimer)
		pTimer->stop();

	pMessaging->stop();
	pMainWindow->stop();

	lmDebugLog::write("Application stopped");
}

//	This slot handles the exit signal emitted by main window when the user
//	selects quit from the menu.
void lmCore::exitApp(void) {
    qApp->quit();  //When you have many chat windows open, calling this once isnt enough for some reason.
    qApp->quit();  //stop function doesnt seem to handle closing all windows properly, but this bodge works perfect, and i am lazy.
}

//	This slot handles the signal emitted by QApplication when the application
//	quits either by user interaction or from operating system signal.
void lmCore::aboutToExit(void) {
	stop();
	pSettings->setValue(IDS_VERSION, IDA_VERSION);

	lmDebugLog::write("Application exit");

	pSettings->sync();
}

void lmCore::timer_timeout(void) {
	//	Refresh the contacts list whenever the timer triggers
	pMessaging->update();
	if(adaptiveRefresh) {
		//	The refresh interval is doubled until the refresh time defined by user is reached.
		//	Then keep refreshing at that interval.
		int nextInterval = pTimer->interval() * 2;
		int maxInterval = pSettings->value(IDS_REFRESHTIME, IDS_REFRESHTIME_VAL).toInt() * 1000;
		int interval = qMin(nextInterval, maxInterval);
		adaptiveRefresh = (nextInterval >= maxInterval) ? false : true;
		pTimer->setInterval(interval);
	} else if(refreshTime > pTimer->interval()) {
		pTimer->setInterval(refreshTime);
	}
}

void lmCore::startChat(QString* lpszUserId) {
	//	return if a chat window is already open for this user
	for(int index = 0; index < chatWindows.count(); index++)
		if(!chatWindows[index]->groupMode && chatWindows[index]->peerIds.contains(*lpszUserId)) {
			showChatWindow(chatWindows[index], true);
			return;
		}

	createChatWindow(lpszUserId);
	showChatWindow(chatWindows.last(), true);
}

void lmCore::startChatRoom(QString* lpszThreadId) {
	createChatRoomWindow(lpszThreadId);
	showChatRoomWindow(chatRoomWindows.last(), true, false, true);
}

void lmCore::sendMessage(MessageType type, QString* lpszUserId, MessageXml* pMessage) {
	switch(type) {
	case MT_Broadcast:
	case MT_UserName:
	case MT_Message:
	case MT_PublicMessage:
	case MT_GroupMessage:
	case MT_Query:
	case MT_ChatState:
	case MT_Version:
    case MT_File:
    case MT_Avatar:
    case MT_Folder:
		pMessaging->sendMessage(type, lpszUserId, pMessage);
		break;
	case MT_Status:
	case MT_Note:
		pMessaging->sendMessage(type, lpszUserId, pMessage);
		processPublicMessage(type, lpszUserId, pMessage);
		routeGroupMessage(type, lpszUserId, pMessage);
        routeMessage(type, lpszUserId, pMessage);
		break;
	case MT_Refresh:
		pMessaging->update();
		break;
    case MT_Audio:
    case MT_Video:
        pMessaging->sendMessage(type, lpszUserId, pMessage);
        {
            int streamOp = Helper::indexOf(StreamOpNames, SO_Max, pMessage->data(XN_STREAMOP));
            bool isCallPeer = lpszUserId && *lpszUserId == callPeerId;
            if(streamOp == SO_Request) {
                if(lpszUserId && callPeerId.isEmpty())
                    callPeerId = *lpszUserId;
            } else if(isCallPeer) {
                //  only messages to the user we are in a call with may change the call state
                if(streamOp == SO_Abort || streamOp == SO_Decline)
                    callEnded(type);
                else if(streamOp == SO_Accept)
                    callConnected(type);
            }
        }
        break;
	default:
		break;
	}
}

void lmCore::receiveMessage(MessageType type, QString* lpszUserId, MessageXml* pMessage) {
	processMessage(type, lpszUserId, pMessage);
}

bool lmCore::receiveAppMessage(const QStringList& options) {
	bool doNotExit = true;

	//	no options means another instance was started plainly, just bring the window forward
	if(options.isEmpty()) {
		pMainWindow->restore();
		return doNotExit;
	}

	if(options.contains(LM_OPT_NEW)) {
		if(options.contains(LM_OPT_LOOPBACK))
			pMessaging->setLoopback(true);
	}
	if(options.contains(LM_OPT_RESET_HISTORY)) {
		QFile::remove(History::historyFile());
		if(pHistoryWindow)
			pHistoryWindow->updateList();
	}
	if(options.contains(LM_OPT_RESET_FILEHIST)) {
		QFile::remove(DefinitionsDir::transferHistory());
		if(pTransferWindow)
			pTransferWindow->updateList();
	}
	if(options.contains(LM_OPT_RESET_CONFIG)) {
		QFile::remove(DefinitionsDir::avatarFile());
		QFile::remove(pSettings->fileName());
		pSettings->sync();
		resetWindowGeometry();
		settingsChanged();
	}
	const QString autoStartPrefix = QString(LM_OPT_AUTOSTART) + "=";
	for(const QString& option : options) {
		if(!option.startsWith(autoStartPrefix))
			continue;
		bool on = option.mid(autoStartPrefix.length()) == "true";
		pSettings->setValue(IDS_AUTOSTART, on, IDS_AUTOSTART_VAL);
		pSettings->sync();
		lmSettings::setAutoStart(on);
		if(!options.contains(LM_OPT_NEW))
			settingsChanged();	//	let a running instance refresh its UI
	}
	if(options.contains(LM_OPT_TERMINATE)) {
		doNotExit = false;
		exitApp();
	}
	if(options.contains(LM_OPT_EXIT_AFTER_ARGS)) {
		doNotExit = false;
	}

	return doNotExit;
}

void lmCore::connectionStateChanged(void) {
	bool connected = pMessaging->isConnected();

	pMainWindow->connectionStateChanged(connected);
	if(pPublicChatWindow)
		pPublicChatWindow->connectionStateChanged(connected);
	for(int index = 0; index < chatWindows.count(); index++)
		chatWindows[index]->connectionStateChanged(connected);
	for(int index = 0; index < chatRoomWindows.count(); index++)
		chatRoomWindows[index]->connectionStateChanged(connected);
	if(pBroadcastWindow)
		pBroadcastWindow->connectionStateChanged(connected);

	if(pMessaging->isConnected() && !pMessaging->canReceive()) {
		showPortConflictMessage();
		exitApp();
	}
}

void lmCore::showTransfers(void) {
	createTransferWindow();
	showTransferWindow(true);
}

void lmCore::showHistory(void) {
	if(!pHistoryWindow) {
		pHistoryWindow = new lmFormHistory();
		pHistoryWindow->init();
	}
	
	Helper::placeOnMainScreen(pHistoryWindow);

	//	if window is minimized it, restore it to previous state
	if(pHistoryWindow->windowState().testFlag(Qt::WindowMinimized))
		pHistoryWindow->setWindowState(pHistoryWindow->windowState() & ~Qt::WindowMinimized);
	pHistoryWindow->setWindowState(pHistoryWindow->windowState() | Qt::WindowActive);
	pHistoryWindow->raise();	// make window the top most window of the application
	pHistoryWindow->show();
	pHistoryWindow->activateWindow();	// bring window to foreground
}

void lmCore::showSettings(void) {
	if(!pSettingsDialog) {
		pSettingsDialog = new lmFormSettings(pMainWindow);
		connect(pSettingsDialog, SIGNAL(historyCleared()), this, SLOT(historyCleared()));
		connect(pSettingsDialog, SIGNAL(fileHistoryCleared()), this, SLOT(fileHistoryCleared()));
		connect(pSettingsDialog, &lmFormSettings::settingsReset, this, &lmCore::resetWindowGeometry);
		//	While a hotkey is being recorded, release the global ones. Otherwise the system would swallow
		//	the very combination the user is trying to (re)assign.
		connect(pSettingsDialog, &lmFormSettings::hotkeyCaptureChanged, pHotkeys, &lmHotkeyHandler::setSuspended);
		connect(pSettingsDialog, &lmFormSettings::micTestChanged, this, &lmCore::micTestChanged);
		pSettingsDialog->init();
	}

	bool accepted = pSettingsDialog->exec();
	pHotkeys->setSuspended(false);	//	in case the dialog closed while recording
	micTestChanged(false);			//	and while testing the microphone
	if(accepted)
		settingsChanged();
}

void lmCore::showAbout(void) {
	if(!pAboutDialog) {
		pAboutDialog = new lmFormAbout(pMainWindow);
		pAboutDialog->init();
	}

	pAboutDialog->exec();
}

void lmCore::showBroadcast(void) {
	if(!pBroadcastWindow) {
		pBroadcastWindow = new lmFormBroadcast();
        connect(pBroadcastWindow, SIGNAL(messageSent(MessageType,QString*,MessageXml*)),
            this, SLOT(sendMessage(MessageType,QString*,MessageXml*)));
		pBroadcastWindow->init(pMessaging->isConnected());
	}

	Helper::placeOnMainScreen(pBroadcastWindow);
	if(pBroadcastWindow->isHidden()) {
		QList<QTreeWidgetItem*> contactsList = pMainWindow->getContactsList();
		pBroadcastWindow->show(&contactsList);
	} else {
		pBroadcastWindow->show();
	}
}

//	keep the broadcast window's user list in sync with the main window
void lmCore::refreshBroadcastUsers(void) {
	if(!pBroadcastWindow || pBroadcastWindow->isHidden())
		return;

	QList<QTreeWidgetItem*> contactsList = pMainWindow->getContactsList();
	pBroadcastWindow->refreshUsers(&contactsList);
}

void lmCore::showPublicChat() {
	//	Show public chat window
	showPublicChatWindow(true);
}

void lmCore::historyCleared(void) {
	if(pHistoryWindow)
		pHistoryWindow->updateList();
}

void lmCore::fileHistoryCleared(void) {
	if(pTransferWindow)
		pTransferWindow->updateList();
}

void lmCore::showTrayMessage(TrayMessageType type, QString szMessage, QString szTitle, TrayMessageIcon icon) {
	pMainWindow->showTrayMessage(type, szMessage, szTitle, icon);
}

void lmCore::addContacts(QStringList* pExcludList) {
    lmFormChatRoom* chatRoomWindow = static_cast<lmFormChatRoom*>(sender());
    QStringList selectedContacts = showSelectContacts(chatRoomWindow, UC_GroupMessage, pExcludList);
	chatRoomWindow->selectContacts(&selectedContacts);
}

void lmCore::chatWindow_closed(QString* lpszUserId) {
	for(int index = 0; index < chatWindows.count(); index++) {
		if(chatWindows[index]->peerIds.contains(*lpszUserId)) {
			chatWindows.removeAt(index);
			break;
		}
	}
}

void lmCore::chatRoomWindow_closed(QString* lpszThreadId) {
	for(int index = 0; index < chatRoomWindows.count(); index++) {
		if(chatRoomWindows[index]->threadId.compare(*lpszThreadId) == 0) {
			chatRoomWindows.removeAt(index);
			break;
		}
	}
}

void lmCore::processMessage(MessageType type, QString* lpszUserId, MessageXml* pMessage) {
	switch(type) {
	case MT_Announce:
		pMainWindow->addUser(pMessaging->getUser(lpszUserId));
		processPublicMessage(type, lpszUserId, pMessage);
		break;
    case MT_Depart:
        pMainWindow->removeUser(lpszUserId);
        processPublicMessage(type, lpszUserId, pMessage);
        routeMessage(type, lpszUserId, pMessage);
        routeGroupMessage(type, lpszUserId, pMessage);
        break;
	case MT_Status:
	case MT_UserName:
	case MT_Note:
		pMainWindow->updateUser(pMessaging->getUser(lpszUserId));
		processPublicMessage(type, lpszUserId, pMessage);
		routeMessage(type, lpszUserId, pMessage);
		routeGroupMessage(type, lpszUserId, pMessage);
		break;
	case MT_Group:
		//	only the main window cares, the user moves to the group they reported
		pMainWindow->updateUser(pMessaging->getUser(lpszUserId));
		break;
	case MT_Avatar:
        pMainWindow->receiveMessage(type, lpszUserId, pMessage);
        processPublicMessage(type, lpszUserId, pMessage);
        routeMessage(type, lpszUserId, pMessage);
        routeGroupMessage(type, lpszUserId, pMessage);
		break;
	case MT_Message:
		routeMessage(type, lpszUserId, pMessage);
		break;
	case MT_GroupMessage:
		routeGroupMessage(type, lpszUserId, pMessage);
		break;
	case MT_PublicMessage:
		processPublicMessage(type, lpszUserId, pMessage);
		break;
	case MT_Broadcast:
		routeMessage(type, lpszUserId, pMessage);
		break;
	case MT_Failed:
		routeMessage(type, lpszUserId, pMessage);
		break;
	case MT_Error:
		break;
	case MT_Query:
		showUserInfo(pMessage);
		break;
	case MT_ChatState:
		routeMessage(type, lpszUserId, pMessage);
		break;
	case MT_File:
    case MT_Folder:
        processFile(type, lpszUserId, pMessage);
		break;
    case MT_Audio:
    case MT_Video:
        processStream(type, lpszUserId, pMessage);
        break;
	case MT_Version:
	case MT_WebFailed:
		break;
    default:
        break;
	}
}

void lmCore::playLoopSound(SoundEvent event)
{
    if (pSoundPlayer)
        pSoundPlayer->playLoop(event);
}

void lmCore::stopLoopSound()
{
    if (pSoundPlayer)
        pSoundPlayer->stopLoop();
}

void lmCore::callRequested(MessageType type) {
    if (callPhase != CP_Idle) return; // busy
    callPhase = CP_Calling;
    emit callPhaseChanged(callPhase);
    playLoopSound(SE_RingOut);
}

void lmCore::callConnected(MessageType type) {
    callPhase = CP_Connected;
    emit callPhaseChanged(callPhase);
    stopLoopSound();
    //  Attach the call to the chat window of the call peer
    lmFormChat* w = nullptr;
    for (lmFormChat* cw : std::as_const(chatWindows)) {
        if (!cw->groupMode && cw->peerId == callPeerId) {
            w = cw;
            break;
        }
    }
    User* pUser = pMessaging->getUser(&callPeerId);
    if (!w || !pUser) return;

    if (!m_audioStream) {
        m_audioStream = new lmAudioStream();     // no parent: it runs on its own thread
        connect(m_audioStream, &lmAudioStream::micMuted, this, [this](bool muted) {
            pSoundPlayer->play(muted ? SE_MicOff : SE_MicOn);
        });
        connect(m_audioStream, &lmAudioStream::speakerMuted, this, [this](bool muted) {
            pSoundPlayer->play(muted ? SE_SpeakerOff : SE_SpeakerOn);
        });
        connect(m_audioStream, &lmAudioStream::mutedTalk, this, [this]() {
            //  The microphone test mutes the call microphone on purpose, talking then is expected.
            if (!micMutedByTest)
                pSoundPlayer->play(SE_MutedTalk);
        });
        connect(m_audioStream, &lmAudioStream::micDeviceChanged, this, [this](const QString& name, bool fallback) {
            if (!fallback)
                return;
            QString message = name.isEmpty()
                ? tr("No microphone is available. Your voice cannot be sent.")
                : tr("Your selected microphone is not available. Using %1 instead.").arg(name);
            lmDebugLog::write(message);
            showTrayMessage(TM_Connection, message, tr("Microphone"), TMI_Warning);
        });
        connect(w, &lmFormChat::micToggled,
                m_audioStream, &lmAudioStream::setMicMuted);
        connect(w, &lmFormChat::speakerToggled,
                m_audioStream, &lmAudioStream::setSpeakerMuted);
    }
    m_audioStream->setMicDevice(micDeviceSetting());     // queued ahead of start(), so the call opens the right device
    m_audioStream->setCallSounds(type != MT_Video);      // a video call starts and ends with the camera sounds
    applyMicActivation();
    m_audioStream->start(QHostAddress(pUser->address),
                         pSettings->value(IDS_UDPPORT, IDS_UDPPORT_VAL).toInt(),
                         pMessaging->crypto(), pUser->id);

    //  A video call carries the voice above plus video. Both sides use the same ports, see lmVideoStream.
    if (type != MT_Video)
        return;
    if (!m_videoStream) {
        m_videoStream = new lmVideoStream();     // no parent: it runs on its own thread
        m_videoCapture = new lmVideoCapture(this);
        connect(m_videoStream, &lmVideoStream::frameReceived, w, &lmFormChat::showRemoteVideo);
        connect(m_videoCapture, &lmVideoCapture::frameCaptured, w, &lmFormChat::showLocalVideo);
        connect(m_videoCapture, &lmVideoCapture::frameCaptured, this, [this](const QImage& image) {
            if (m_videoStream)
                m_videoStream->sendFrame(image);
        });
        connect(w, &lmFormChat::camToggled, m_videoCapture, &lmVideoCapture::setMuted);
    }
    m_videoStream->start(QHostAddress(pUser->address),
                         pSettings->value(IDS_UDPPORT, IDS_UDPPORT_VAL).toInt(),
                         pMessaging->crypto(), pUser->id);
    m_videoCapture->setSource(camDeviceSetting());
    m_videoCapture->start();
    pSoundPlayer->play(SE_CamOn);
}

void lmCore::callEnded(MessageType type) {
    callPhase = CP_Idle;
    emit callPhaseChanged(callPhase);
    stopLoopSound();

    callPeerId.clear();
    if (m_audioStream) { m_audioStream->stop(); delete m_audioStream; m_audioStream = nullptr; }
    if (m_videoStream)
        pSoundPlayer->play(SE_CamOff);
    if (m_videoCapture) { m_videoCapture->stop(); delete m_videoCapture; m_videoCapture = nullptr; }
    if (m_videoStream) { m_videoStream->stop(); delete m_videoStream; m_videoStream = nullptr; }
}

//  The microphone test in preferences listens to the microphone, and during a call that must not reach the other
//  side. So the call microphone is muted for the duration of the test, and restored afterwards.
void lmCore::micTestChanged(bool testing) {
    if (testing) {
        lmFormChat* w = callWindow();
        if (w && !w->isMicrophoneMuted()) {
            w->toggleMicrophone();
            micMutedByTest = true;
        }
    } else if (micMutedByTest) {
        micMutedByTest = false;
        lmFormChat* w = callWindow();
        if (w && w->isMicrophoneMuted())
            w->toggleMicrophone();
    }
}

int lmCore::micActivationSetting(void) const {
    return qBound(0, pSettings->value(IDS_MICACTIVATION, IDS_MICACTIVATION_VAL).toInt(), int(MA_Max) - 1);
}

void lmCore::applyMicActivation(void) {
    if (m_audioStream)
        m_audioStream->setMicActivation(micActivationSetting(), pSettings->value(IDS_MICTHRESHOLD, IDS_MICTHRESHOLD_VAL).toInt());
}

QString lmCore::camDeviceSetting(void) const {
    return pSettings->value(IDS_CAMDEVICE, IDS_CAMDEVICE_VAL).toString();
}

QByteArray lmCore::micDeviceSetting(void) const {
    return pSettings->value(IDS_MICDEVICE, IDS_MICDEVICE_VAL).toString().toUtf8();
}

//  The chat window that owns the connected call, or nullptr when there is none.
lmFormChat* lmCore::callWindow(void) const {
    if (callPhase != CP_Connected)
        return nullptr;
    for (lmFormChat* cw : std::as_const(chatWindows)) {
        if (!cw->groupMode && cw->peerId == callPeerId)
            return cw;
    }
    return nullptr;
}

//  Global hotkeys press the call buttons of the active call window, so the buttons, icons and sounds
//  behave exactly as if the user had clicked them. Outside of a call there is nothing to toggle.
void lmCore::hotkeyTriggered(lmHotkeyHandler::Action action) {
    //  While the microphone test in preferences runs, it owns the microphone hotkey. That is where push to talk
    //  can be tried out, and a toggle must not unmute a call microphone that is muted for the test.
    if (action == lmHotkeyHandler::HK_Mic && pSettingsDialog && pSettingsDialog->isMicTesting()) {
        if (pSettingsDialog->testPushToTalk(true)) {
            pSoundPlayer->play(SE_MicOn);
            pttSoundPlayed = true;
        }
        return;
    }

    lmFormChat* w = callWindow();
    if (!w)
        return;
    switch (action) {
    case lmHotkeyHandler::HK_Mic:
        //  Push to talk uses the microphone hotkey as a button to hold. The microphone itself stays unmuted.
        if (micActivationSetting() == MA_PushToTalk) {
            if (m_audioStream) {
                m_audioStream->setPushToTalk(true);
                if (!w->isMicrophoneMuted()) {      // muted by hand, nothing is going out, so don't pretend
                    pSoundPlayer->play(SE_MicOn);
                    pttSoundPlayed = true;
                }
            }
        } else {
            w->toggleMicrophone();
        }
        break;
    case lmHotkeyHandler::HK_Speaker:   w->toggleSpeaker();    break;
    case lmHotkeyHandler::HK_Camera:    w->toggleCamera();     break;
    default:                                                   break;
    }
}

void lmCore::hotkeyReleased(lmHotkeyHandler::Action action) {
    if (action != lmHotkeyHandler::HK_Mic)
        return;
    if (pSettingsDialog && pSettingsDialog->isMicTesting())
        pSettingsDialog->testPushToTalk(false);
    if (m_audioStream)
        m_audioStream->setPushToTalk(false);
    if (pttSoundPlayed) {
        pttSoundPlayed = false;
        pSoundPlayer->play(SE_MicOff);
    }
}

void lmCore::hotkeyRegistrationFailed(lmHotkeyHandler::Action action, const QKeySequence& sequence, const QString& reason) {
    QString message = tr("The %1 hotkey (%2) is not active: %3")
        .arg(lmHotkeyHandler::actionName(action), sequence.toString(QKeySequence::NativeText), reason);
    lmDebugLog::write(message);
    showTrayMessage(TM_Connection, message, tr("Hotkey"), TMI_Warning);
}

void lmCore::processStream(MessageType type, QString *lpszUserId, MessageXml* pMessage) {
    int streamOp = Helper::indexOf(StreamOpNames, SO_Max, pMessage->data(XN_STREAMOP));

    //  While busy, ignore call state messages from anyone but our call peer
    if(streamOp != SO_Request && callPhase != CP_Idle && *lpszUserId != callPeerId)
        return;

    switch(streamOp) {
    case SO_Request:
        if(callPhase != CP_Idle) {  //Auto decline if busy
            MessageXml xml;
            xml.addData(XN_STREAMOP, StreamOpNames[SO_Decline]);
            xml.addData(XN_STREAMID, pMessage->data(XN_STREAMID));
            //  send directly: going through sendMessage() would end our ongoing call
            pMessaging->sendMessage(type, lpszUserId, &xml);

            //  let the local user see the declined call in the chat with the caller
            MessageXml logXml;
            logXml.addData(XN_STREAMID, pMessage->data(XN_STREAMID));
            logXml.addData(XN_STREAMOP, StreamOpNames[SO_Decline]);
            logXml.addData(XN_STREAMMODE, StreamModeNames[SM_In]);
            lmFormChat* pWindow = nullptr;
            for(int index = 0; index < chatWindows.count(); index++) {
                if(chatWindows[index]->peerIds.contains(*lpszUserId)
                    && chatWindows[index]->threadId == pMessage->data(XN_THREAD)) {
                    pWindow = chatWindows[index];
                    break;
                }
            }
            if(!pWindow) {
                createChatWindow(lpszUserId);
                pWindow = chatWindows.last();
            }
            pWindow->showMissedCall(type, lpszUserId, &logXml);
            showChatWindow(pWindow, messageTop, true);
            return;
        }
        callPeerId = *lpszUserId;
        callPhase = CP_Ringing;
        emit callPhaseChanged(callPhase);
        playLoopSound(SE_RingIn);
        break;
    case SO_Accept:
        callConnected(type);
        return;      //Prevents double calling, therefore breaking socket binding.
    case SO_Decline:
    case SO_Abort:
    case SO_Error:
        callEnded(type);
        break;
    default:
        break;
    }
    routeMessage(type, lpszUserId, pMessage);
}

void lmCore::processFile(MessageType type, QString *lpszUserId, MessageXml* pMessage) {
	int fileOp = Helper::indexOf(FileOpNames, FO_Max, pMessage->data(XN_FILEOP));
    int fileMode = Helper::indexOf(FileModeNames, FM_Max, pMessage->data(XN_MODE));
	switch(fileOp) {
	case FO_Accept:
        initFileTransfer(type, (FileMode)fileMode, lpszUserId, pMessage);
        showTransferWindow();
		break;
    default:
        break;
	}
	if(fileOp != FO_Request && pTransferWindow)
		pTransferWindow->receiveMessage(type, lpszUserId, pMessage);

	routeMessage(type, lpszUserId, pMessage);
}

void lmCore::routeMessage(MessageType type, QString* lpszUserId, MessageXml* pMessage) {
    bool windowExists = false;
    bool needsNotice = (type == MT_Message || type == MT_Broadcast || type == MT_Failed
                        || (type == MT_File && pMessage->data(XN_FILEOP) == FileOpNames[FO_Request])
                        || (type == MT_Folder && pMessage->data(XN_FILEOP) == FileOpNames[FO_Request])
                        || (type == MT_Audio  && pMessage->data(XN_STREAMOP) == StreamOpNames[SO_Request])
                        || (type == MT_Video  && pMessage->data(XN_STREAMOP) == StreamOpNames[SO_Request])
                        || type == MT_GroupMessage);

    //	If no specific user is specified, send this message to all windows
    if(!lpszUserId) {
        for(int index = 0; index < chatWindows.count(); index++) {
            if(type == MT_Status || type == MT_Note)
                chatWindows[index]->updateUser(pMessaging->localUser);
            chatWindows[index]->receiveMessage(type, lpszUserId, pMessage);
        }
    } else {
        QString threadId = pMessage ? pMessage->data(XN_THREAD) : QString();
        switch(type) {
        case MT_Avatar:
        case MT_Status:
        case MT_Note:
        case MT_UserName:
            for(int index = 0; index < chatWindows.count(); index++)
                if(chatWindows[index]->peerIds.contains(*lpszUserId)
                    || chatWindows[index]->localId.compare(*lpszUserId) == 0) {
                    if(type != MT_Avatar)
                        chatWindows[index]->updateUser(pMessaging->getUser(lpszUserId));
                    chatWindows[index]->receiveMessage(type, lpszUserId, pMessage);
                }
            break;
        default:
            for(int index = 0; index < chatWindows.count(); index++) {
                if(chatWindows[index]->peerIds.contains(*lpszUserId)
                    && chatWindows[index]->threadId == threadId) {
                    chatWindows[index]->receiveMessage(type, lpszUserId, pMessage);
                    if(needsNotice)
                        showChatWindow(chatWindows[index], messageTop, needsNotice);
                    windowExists = true;
                    break;
                }
            }
            break;
        }
    }

    //	create a new window if no chat window with this user exists and the
    //	incoming message is of type that needs notice
    if(!windowExists && needsNotice) {
        createChatWindow(lpszUserId);
        chatWindows.last()->receiveMessage(type, lpszUserId, pMessage);
        if(needsNotice)
            showChatWindow(chatWindows.last(), messageTop, needsNotice);
    }
}

void lmCore::routeGroupMessage(MessageType type, QString* lpszUserId, MessageXml* pMessage) {
	if(!lpszUserId) {
		for(int index = 0; index < chatRoomWindows.count(); index++) {
			if(type == MT_Status || type == MT_Note)
				chatRoomWindows[index]->updateUser(pMessaging->localUser);
			chatRoomWindows[index]->receiveMessage(type, lpszUserId, pMessage);
		}
	} else {
		QString threadId;
		int op;
		bool windowExists = false;

		switch(type) {
		case MT_Depart:
			// If pMessage is NULL, the user is actually offline. Remove user
			// from all chat rooms.
			if(!pMessage) {
				for(int index = 0; index < chatRoomWindows.count(); index++)
					if(chatRoomWindows[index]->peerIds.contains(*lpszUserId))
						chatRoomWindows[index]->removeUser(lpszUserId);
			}
			break;
		case MT_Status:
		case MT_UserName:
		case MT_Note:
			for(int index = 0; index < chatRoomWindows.count(); index++)
				if(chatRoomWindows[index]->peerIds.contains(*lpszUserId)) {
					chatRoomWindows[index]->updateUser(pMessaging->getUser(lpszUserId));
					chatRoomWindows[index]->receiveMessage(type, lpszUserId, pMessage);
				}
			break;
        case MT_Avatar:
			for(int index = 0; index < chatRoomWindows.count(); index++)
				if(chatRoomWindows[index]->peerIds.contains(*lpszUserId))
					chatRoomWindows[index]->receiveMessage(type, lpszUserId, pMessage);
			break;
		default:
			threadId = pMessage->data(XN_THREAD);
			op = Helper::indexOf(GroupMsgOpNames, GMO_Max, pMessage->data(XN_GROUPMSGOP));

			if(op == GMO_Request) {
				//	Check if a chat room with the thread id already exists
				for(int index = 0; index < chatRoomWindows.count(); index++) {
					if(chatRoomWindows[index]->threadId.compare(threadId) == 0) {
						chatRoomWindows[index]->addUser(pMessaging->localUser);
						windowExists = true;
						break;
					}
				}
				if(!windowExists) {
					createChatRoomWindow(&threadId);
					showChatRoomWindow(chatRoomWindows.last(), messageTop, true);
				}
			}
			for(int index = 0; index < chatRoomWindows.count(); index++) {
				if(chatRoomWindows[index]->threadId.compare(threadId) == 0) {
					switch(op) {
					case GMO_Request:
					case GMO_Join:
						chatRoomWindows[index]->addUser(pMessaging->getUser(lpszUserId));
						break;
					case GMO_Message:
						chatRoomWindows[index]->receiveMessage(type, lpszUserId, pMessage);
						showChatRoomWindow(chatRoomWindows[index], messageTop, true);
						break;
					case GMO_Leave:
						chatRoomWindows[index]->removeUser(lpszUserId);
						break;
					default:
						break;
					}
					break;
				}
			}
			break;
		}
	}
}

void lmCore::processPublicMessage(MessageType type, QString* lpszUserId, MessageXml* pMessage) {
	if(!pPublicChatWindow)
		return;

	switch(type) {
	case MT_Announce:
		pPublicChatWindow->addUser(pMessaging->getUser(lpszUserId));
		break;
	case MT_Depart:
		pPublicChatWindow->removeUser(lpszUserId);
		break;
	case MT_Status:
	case MT_UserName:
	case MT_Note:
		// lpszUserId can be NULL if sent by local user
		if(lpszUserId)
			pPublicChatWindow->updateUser(pMessaging->getUser(lpszUserId));
		else
			pPublicChatWindow->updateUser(pMessaging->localUser);
		break;
	case MT_PublicMessage:
		pPublicChatWindow->receiveMessage(type, lpszUserId, pMessage);
		showPublicChatWindow((pubMessagePop && messageTop), true, pubMessagePop);
		break;
    case MT_Avatar:
		pPublicChatWindow->receiveMessage(type, lpszUserId, pMessage);
		break;
	default:
		break;
	}
}

void lmCore::createTransferWindow(void) {
    if(!pTransferWindow) {
        pTransferWindow = new lmFormTransfer();
        connect(pTransferWindow, SIGNAL(messageSent(MessageType,QString*,MessageXml*)),
            this, SLOT(sendMessage(MessageType,QString*,MessageXml*)));
        connect(pTransferWindow, SIGNAL(showTrayMessage(TrayMessageType,QString,QString,TrayMessageIcon)),
            this, SLOT(showTrayMessage(TrayMessageType,QString,QString,TrayMessageIcon)));
        pTransferWindow->init();
    }
}

void lmCore::showTransferWindow(bool show) {
	Helper::placeOnMainScreen(pTransferWindow);
	bool autoShow = pSettings->value(IDS_AUTOSHOWFILE, IDS_AUTOSHOWFILE_VAL).toBool();
	bool bringToForeground = pSettings->value(IDS_FILETOP, IDS_FILETOP_VAL).toBool();

	if((autoShow && bringToForeground) || show) {
		//	if window is minimized it, restore it to previous state
		if(pTransferWindow->windowState().testFlag(Qt::WindowMinimized))
			pTransferWindow->setWindowState(pTransferWindow->windowState() & ~Qt::WindowMinimized);
		pTransferWindow->setWindowState(pTransferWindow->windowState() | Qt::WindowActive);
		pTransferWindow->raise();	// make main window the top most window of the application
		pTransferWindow->show();
		pTransferWindow->activateWindow();	// bring window to foreground
	}
	else if(autoShow && !bringToForeground) {
		if(pTransferWindow->isHidden())
			pTransferWindow->setWindowState(pTransferWindow->windowState() | Qt::WindowMinimized);
		pTransferWindow->setWindowState(pTransferWindow->windowState() | Qt::WindowActive);
		pTransferWindow->show();
		qApp->alert(pTransferWindow);
	}
}

void lmCore::initFileTransfer(MessageType type, FileMode mode, QString *lpszUserId, MessageXml *pMessage) {
    createTransferWindow();
	
	User* pUser = pMessaging->getUser(lpszUserId);
    pTransferWindow->createTransfer(type, mode, lpszUserId, &pUser->name, pMessage);
}

void lmCore::showUserInfo(MessageXml* pMessage) {
	if(!pUserInfoWindow) {
		pUserInfoWindow = new lmFormUserInfo();
		pUserInfoWindow->init();
	}

	pUserInfoWindow->setInfo(pMessage);
	Helper::placeOnMainScreen(pUserInfoWindow);

	//	if window is minimized it, restore it to previous state
	if(pUserInfoWindow->windowState().testFlag(Qt::WindowMinimized))
		pUserInfoWindow->setWindowState(pUserInfoWindow->windowState() & ~Qt::WindowMinimized);
	pUserInfoWindow->setWindowState(pUserInfoWindow->windowState() | Qt::WindowActive);
	pUserInfoWindow->raise();	// make window the top most window of the application
	pUserInfoWindow->show();
	pUserInfoWindow->activateWindow();	// bring window to foreground
}

void lmCore::createChatWindow(QString* lpszUserId) {
	//	create new chat window for this user
	lmFormChat* pChatWindow = new lmFormChat();
	chatWindows.append(pChatWindow);
	User* pLocalUser = pMessaging->localUser;
	User* pRemoteUser = pMessaging->getUser(lpszUserId);
    connect(pChatWindow, SIGNAL(messageSent(MessageType,QString*,MessageXml*)),
        this, SLOT(sendMessage(MessageType,QString*,MessageXml*)));
	connect(pChatWindow, SIGNAL(showHistory()), this, SLOT(showHistory()));
	connect(pChatWindow, SIGNAL(showTransfers()), this, SLOT(showTransfers()));
	connect(pChatWindow, SIGNAL(closed(QString*)), this, SLOT(chatWindow_closed(QString*)));
	pChatWindow->init(pLocalUser, pRemoteUser, pMessaging->isConnected());
    connect(pChatWindow, SIGNAL(callRequested(MessageType)),
            this, SLOT(callRequested(MessageType)));
    connect(pChatWindow, SIGNAL(callEnded(MessageType)),
            this, SLOT(callEnded(MessageType)));
    connect(pChatWindow, SIGNAL(callConnected(MessageType)),
            this, SLOT(callConnected(MessageType)));
    connect(this, SIGNAL(callPhaseChanged(CallPhase)),
            pChatWindow, SLOT(callPhaseChanged(CallPhase)));
    pChatWindow->callPhaseChanged(callPhase);
    //  Mic and speaker sounds come from the audio stream. There is no video stream yet, so play the camera ones here.
    connect(pChatWindow, &lmFormChat::camToggled, this, [this](bool muted) {
        pSoundPlayer->play(muted ? SE_CamOff : SE_CamOn);
    });
}

void lmCore::showChatWindow(lmFormChat* chatWindow, bool show, bool alert) {
	Helper::placeOnMainScreen(chatWindow);
	if(show) {
		if(chatWindow->windowState().testFlag(Qt::WindowMinimized))
			chatWindow->setWindowState(chatWindow->windowState() & ~Qt::WindowMinimized);
		chatWindow->show();
		chatWindow->activateWindow();
	} else {
		if(chatWindow->isHidden())
			chatWindow->setWindowState(chatWindow->windowState() | Qt::WindowMinimized);
		chatWindow->show();
		if(alert)
			qApp->alert(chatWindow);
	}
}

void lmCore::createChatRoomWindow(QString* lpszThreadId) {
	//	create a new chat room with the specified thread id
	lmFormChatRoom* pChatRoomWindow = new lmFormChatRoom();
	chatRoomWindows.append(pChatRoomWindow);
	User* pLocalUser = pMessaging->localUser;
    connect(pChatRoomWindow, SIGNAL(messageSent(MessageType,QString*,MessageXml*)),
        this, SLOT(sendMessage(MessageType,QString*,MessageXml*)));
    connect(pChatRoomWindow, SIGNAL(contactsAdding(QStringList*)),
        this, SLOT(addContacts(QStringList*)));
	connect(pChatRoomWindow, SIGNAL(chatStarting(QString*)), this, SLOT(startChat(QString*)));
	connect(pChatRoomWindow, SIGNAL(closed(QString*)), this, SLOT(chatRoomWindow_closed(QString*)));
	pChatRoomWindow->init(pLocalUser, pMessaging->isConnected(), *lpszThreadId);
}

void lmCore::showChatRoomWindow(lmFormChatRoom* chatRoomWindow, bool show, bool alert, bool add) {
	Helper::placeOnMainScreen(chatRoomWindow);
	// if show or add is specified, bring to top
	if(show || add) {
		if(chatRoomWindow->windowState().testFlag(Qt::WindowMinimized))
			chatRoomWindow->setWindowState(chatRoomWindow->windowState() & ~Qt::WindowMinimized);
		chatRoomWindow->show();
		chatRoomWindow->activateWindow();
	} else {
		if(chatRoomWindow->isHidden())
			chatRoomWindow->setWindowState(chatRoomWindow->windowState() | Qt::WindowMinimized);
		chatRoomWindow->show();
		if(alert)
			qApp->alert(chatRoomWindow);
	}

	// if add is specified, show Add Contact dialog
	// add should be specified only when the local user is creating a new chat room
	if(add) {
		QStringList excludeList(pMessaging->localUser->id);
        QStringList selectedContacts = showSelectContacts(chatRoomWindow, UC_GroupMessage, &excludeList);

		// if no contacts were selected, close the chat room window
		if(selectedContacts.count() == 0) {
			chatRoomWindows.removeOne(chatRoomWindow);
			chatRoomWindow->close();
		} else
			chatRoomWindow->selectContacts(&selectedContacts);
	}
}

void lmCore::showPublicChatWindow(bool show, bool alert, bool open) {
	if(show || open)
		Helper::placeOnMainScreen(pPublicChatWindow);
	if(show) {
		if(pPublicChatWindow->windowState().testFlag(Qt::WindowMinimized))
			pPublicChatWindow->setWindowState(pPublicChatWindow->windowState() & ~Qt::WindowMinimized);
		pPublicChatWindow->show();
		pPublicChatWindow->activateWindow();
	} else {
		if(open) {
			if(pPublicChatWindow->isHidden())
				pPublicChatWindow->setWindowState(pPublicChatWindow->windowState() | Qt::WindowMinimized);
			pPublicChatWindow->show();
		}
		if(alert)
			qApp->alert(pPublicChatWindow);
	}
}

//	remove the users that do not have the required capabilities or are excluded
void lmCore::filterContactsList(QList<QTreeWidgetItem*>* pContactsList, uint caps, const QStringList& excludeList) {
	for(int index = 0; index < pContactsList->count(); index++) {
		QTreeWidgetItem* pItem = pContactsList->value(index);
		for(int childIndex = 0; childIndex < pItem->childCount(); childIndex++) {
			QTreeWidgetItem* pChildItem = pItem->child(childIndex);
			QString userId = pChildItem->data(0, IdRole).toString();
			User* pUser = pMessaging->getUser(&userId);
			if(!pUser || (pUser->caps & caps) != caps || excludeList.contains(userId)) {
				delete pItem->takeChild(childIndex);
				childIndex--;
			}
		}
	}
}

QStringList lmCore::showSelectContacts(QWidget* parent, uint caps, QStringList* excludeList) {
	QStringList selectedContacts;
	pUserSelectDialog = new lmFormUserSelect(parent);

	//	remember the filter so the list can be refreshed while the dialog is open
	selectCaps = caps;
	selectExclude = *excludeList;

	QList<QTreeWidgetItem*> contactsList = pMainWindow->getContactsList();
	filterContactsList(&contactsList, selectCaps, selectExclude);

	pUserSelectDialog->init(&contactsList);
	if(pUserSelectDialog->exec())
		selectedContacts = pUserSelectDialog->selectedContacts;
	pUserSelectDialog->deleteLater();

	return selectedContacts;
}

//	keep the contact selection dialog's user list in sync with the main window
void lmCore::refreshSelectUsers(void) {
	if(!pUserSelectDialog || pUserSelectDialog->isHidden())
		return;

	QList<QTreeWidgetItem*> contactsList = pMainWindow->getContactsList();
	filterContactsList(&contactsList, selectCaps, selectExclude);
	pUserSelectDialog->refreshUsers(&contactsList);
}

void lmCore::showPortConflictMessage(void) {
	//	show message box
	QMessageBox msgBox;
	msgBox.setWindowTitle(lmStrings::appName());
	msgBox.setWindowIcon(QIcon(IDR_APPICON));
	msgBox.setIcon(QMessageBox::Critical);
	QString msg = tr("A port address conflict has been detected. %1 will close now.");
	msgBox.setText(msg.arg(lmStrings::appName()));
	QString detail = tr("%1 cannot start because another application is using the port "\
		"configured for use with %2.");
	msgBox.setDetailedText(detail.arg(lmStrings::appName(), lmStrings::appName()));
	msgBox.exec();
}
