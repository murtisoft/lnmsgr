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

#include "messaging.h"
#include "definitionsdir.h"
#include "zdebuglog.h"

lmMessaging::lmMessaging(void) {
	pNetwork = new lmNetwork();
    connect(pNetwork, SIGNAL(broadcastReceived(DatagramHeader*,QString*)),
        this, SLOT(receiveBroadcast(DatagramHeader*,QString*)));
    connect(pNetwork, SIGNAL(messageReceived(DatagramHeader*,QString*)),
        this, SLOT(receiveMessage(DatagramHeader*,QString*)));
    connect(pNetwork, SIGNAL(newConnection(QString*,QString*)),
        this, SLOT(newConnection(QString*,QString*)));
	connect(pNetwork, SIGNAL(connectionLost(QString*)),
		this, SLOT(connectionLost(QString*)));
    connect(pNetwork, SIGNAL(progressReceived(QString*,QString*)),
        this, SLOT(receiveProgress(QString*,QString*)));
	connect(pNetwork, SIGNAL(connectionStateChanged()), this, SLOT(network_connectionStateChanged()));
    localUser = NULL;
	userList.clear();
	receivedList.clear();
	pendingList.clear();
    fileList.clear();
    folderList.clear();
	loopback = false;
}

lmMessaging::~lmMessaging(void) {
}

void lmMessaging::init(MessageXml *pInitParams) {
	lmDebugLog::write("Messaging initialized");

	pNetwork->init(pInitParams);

    QString logonName = Helper::getLogonName();
    QString szAddress = pNetwork->physicalAddress();
    QString userId = createUserId(&szAddress, &logonName);

	pNetwork->setLocalId(&userId);
	
	pSettings = new lmSettings();
	QString userStatus = pSettings->value(IDS_STATUS, IDS_STATUS_VAL).toString();
	int sIndex = Helper::statusIndexFromCode(userStatus);
	//	if status not recognized, default to available
	if(sIndex < 0)
		userStatus = statusCode[0];
	QString userName = getUserName();

	int nAvatar = pSettings->value(IDS_AVATAR, IDS_AVATAR_VAL).toInt();
	QString userNote = pSettings->value(IDS_NOTE, IDS_NOTE_VAL).toString();
    uint userCaps = UC_File | UC_GroupMessage | UC_Folder | UC_Nudge | UC_Audio | UC_Video;
    QString userGroup = Helper::normalizeGroupName(pSettings->value(IDS_USERGROUP, IDS_USERGROUP_VAL).toString());
    localUser = new User(userId, IDA_VERSION, pNetwork->ipAddress, userName, userStatus,
                         userGroup, nAvatar, userNote, DefinitionsDir::avatarFile(),
                         QString::number(userCaps));

	nTimeout = pSettings->value(IDS_TIMEOUT, IDS_TIMEOUT_VAL).toInt() * 1000;
	nMaxRetry = pSettings->value(IDS_MAXRETRIES, IDS_MAXRETRIES_VAL).toInt();

	pTimer = new QTimer(this);
	connect(pTimer, SIGNAL(timeout()), this, SLOT(timer_timeout()));
	pTimer->start(1000);

	msgId = 1;
}

void lmMessaging::start(void) {
	lmDebugLog::write("Messaging started");
	pNetwork->start();

	sendBroadcast(MT_Depart, NULL);
	sendBroadcast(MT_Announce, NULL);
}

void lmMessaging::update(void) {
	lmDebugLog::write("Refreshing contacts list...");
	sendBroadcast(MT_Announce, NULL);

	for(int index = 0; index < userList.count(); index++)
		sendMessage(MT_Ping, &userList[index].id, NULL);
}

void lmMessaging::stop(void) {
    sendBroadcast(MT_Depart, NULL);
	pNetwork->stop();

	pSettings->setValue(IDS_STATUS, localUser->status);
	pSettings->setValue(IDS_AVATAR, localUser->avatar);

	//	make sure the correct version is set in the preferences file
	pSettings->setValue(IDS_VERSION, IDA_VERSION);

	lmDebugLog::write("Messaging stopped");
}

bool lmMessaging::isConnected(void) {
	return pNetwork->isConnected;
}

bool lmMessaging::canReceive(void) {
	return pNetwork->canReceive;
}

void lmMessaging::setLoopback(bool on) {
	loopback = on;
}

User* lmMessaging::getUser(QString* id) {
	for(int index = 0; index < userList.count(); index++)
		if(userList[index].id.compare(*id) == 0)
			return &userList[index];

	return NULL;
}

void lmMessaging::settingsChanged(void) {
	nTimeout = pSettings->value(IDS_TIMEOUT, IDS_TIMEOUT_VAL).toInt() * 1000;
	nMaxRetry = pSettings->value(IDS_MAXRETRIES, IDS_MAXRETRIES_VAL).toInt();
	pNetwork->settingsChanged();

	QString userName = getUserName();
	if(localUser->name.compare(userName) != 0) {
		localUser->name = userName;
		MessageXml xmlMessage;
		xmlMessage.addData(XN_NAME, userName);
		sendMessage(MT_UserName, NULL, &xmlMessage);
	}

	//	the group is reported to everyone the same way as the user name
	QString userGroup = Helper::normalizeGroupName(pSettings->value(IDS_USERGROUP, IDS_USERGROUP_VAL).toString());
	if(localUser->group.compare(userGroup) != 0) {
		localUser->group = userGroup;
		MessageXml xmlMessage;
		xmlMessage.addData(XN_GROUP, userGroup);
		sendMessage(MT_Group, NULL, &xmlMessage);
	}
}

int lmMessaging::userCount(void) {
	return userList.count();
}

void lmMessaging::network_connectionStateChanged(void) {
    if(isConnected()) {
		localUser->address = pNetwork->ipAddress;
        if(localUser->id.isNull()) {
            QString logonName = Helper::getLogonName();
            QString szAddress = pNetwork->physicalAddress();
            QString userId = createUserId(&szAddress, &logonName);
            localUser->id = userId;
            pNetwork->setLocalId(&userId);
        }
    }
	emit connectionStateChanged();
}

void lmMessaging::timer_timeout(void) {
	//	check if any pending message has timed out
	checkPendingMsg();
}

QString lmMessaging::createUserId(QString* lpszAddress, QString* lpszUserName) {
    QString userId = *lpszAddress;
    if (!userId.isNull()) {
        if (lpszUserName)
            userId.append(*lpszUserName);
        userId.remove(":");
    }
    return userId;
}

QString lmMessaging::getUserName(void) {
	QString userName = pSettings->value(IDS_USERNAME, IDS_USERNAME_VAL).toString();
	if(userName.isEmpty())
		userName = Helper::getLogonName();
	return userName;
}

void lmMessaging::getUserInfo(MessageXml* pMessage) {
	QString firstName = pSettings->value(IDS_USERFIRSTNAME, IDS_USERFIRSTNAME_VAL).toString();
	QString lastName = pSettings->value(IDS_USERLASTNAME, IDS_USERLASTNAME_VAL).toString();
	QString about = pSettings->value(IDS_USERABOUT, IDS_USERABOUT_VAL).toString();
	firstName = firstName.isEmpty() ? "N/A" : firstName;
	lastName = lastName.isEmpty() ? "N/A" : lastName;
	about = about.isEmpty() ? "N/A" : about;

	pMessage->addData(XN_USERID, localUser->id);
	pMessage->addData(XN_NAME, localUser->name);
	pMessage->addData(XN_ADDRESS, localUser->address);
	pMessage->addData(XN_VERSION, localUser->version);
	pMessage->addData(XN_STATUS, localUser->status);
	pMessage->addData(XN_NOTE, localUser->note);
	pMessage->addData(XN_LOGON, Helper::getLogonName());
	pMessage->addData(XN_HOST, Helper::getHostName());
	pMessage->addData(XN_OS, Helper::getOSName());
	pMessage->addData(XN_FIRSTNAME, firstName);
	pMessage->addData(XN_LASTNAME, lastName);
	pMessage->addData(XN_ABOUT, about);
}

bool lmMessaging::addUser(QString szUserId, QString szVersion, QString szAddress, QString szName, QString szStatus,
                           QString szGroup, QString szAvatar, QString szNote, QString szCaps) {
	for(int index = 0; index < userList.count(); index++)
		if(userList[index].id.compare(szUserId) == 0)
			return false;

	lmDebugLog::write("Adding new user: " + szUserId + ", " + szVersion + ", " + szAddress);

	int nAvatar = szAvatar.isNull() ? -1 : szAvatar.toInt();

    //	the user reports their own group, peers without one end up in the default group
    userList.append(User(szUserId, szVersion, szAddress, szName, szStatus, Helper::normalizeGroupName(szGroup),
                         nAvatar, szNote, QString(), szCaps));
	if(!szStatus.isNull()) {
		MessageXml xmlMessage;
		xmlMessage.addHeader(XN_FROM, szUserId);
		xmlMessage.addData(XN_STATUS, szStatus);
		//	send a status message to app layer, this is different from announce message
		emit messageReceived(MT_Status, &szUserId, &xmlMessage);
		int statusIndex = Helper::statusIndexFromCode(szStatus);
		if(statusType[statusIndex] == StatusTypeOffline) // offline status
			return false;	//	no need to send a new user message to app layer
	}

	emit messageReceived(MT_Announce, &szUserId, NULL);
	return true;
}

void lmMessaging::updateUser(MessageType type, QString szUserId, QString szUserData) {
	User* pUser = getUser(&szUserId);
	if(!pUser)
		return;

	MessageXml updateMsg;	
	switch(type) {
	case MT_Status:
		if(pUser->status.compare(szUserData) != 0) {
			QString oldStatus = pUser->status;
			pUser->status = szUserData;

			int statusIndex = Helper::statusIndexFromCode(oldStatus);
			if(statusType[statusIndex] == StatusTypeOffline) // old status is offline
				emit messageReceived(MT_Announce, &szUserId, NULL);
				
			updateMsg.addData(XN_STATUS, pUser->status);
			emit messageReceived(MT_Status, &szUserId, &updateMsg);

			statusIndex = Helper::statusIndexFromCode(pUser->status);
			if(statusType[statusIndex] == StatusTypeOffline) { // new status is offline
				// Send a dummy xml message. A non null xml message implies that the
				// user is only in offline status, and not actually offline.
				MessageXml xmlMessage;
				emit messageReceived(MT_Depart, &szUserId, &xmlMessage);
			}
		}
		break;
	case MT_UserName:
		if(pUser->name.compare(szUserData) != 0) {
			pUser->name = szUserData;
			updateMsg.addData(XN_NAME, pUser->name);
			emit messageReceived(MT_UserName, &szUserId, &updateMsg);
		}
		break;
	case MT_Note:
		if(pUser->note.compare(szUserData) != 0) {
			pUser->note = szUserData;
			updateMsg.addData(XN_NOTE, pUser->note);
			emit messageReceived(MT_Note, &szUserId, &updateMsg);
		}
		break;
	case MT_Group: {
		QString group = Helper::normalizeGroupName(szUserData);
		if(pUser->group.compare(group) != 0) {
			pUser->group = group;
			updateMsg.addData(XN_GROUP, pUser->group);
			emit messageReceived(MT_Group, &szUserId, &updateMsg);
		}
		break;
	}
    case MT_Avatar:
        pUser->avatarPath = szUserData;
        break;
	default:
		break;
	}
}

void lmMessaging::removeUser(QString szUserId) {
	for(int index = 0; index < userList.count(); index++)
		if(userList.value(index).id.compare(szUserId) == 0) {
			MessageXml statusMsg;
			statusMsg.addData(XN_STATUS, statusCode[ST_COUNT - 1]);
			emit messageReceived(MT_Status, &szUserId, &statusMsg);
			emit messageReceived(MT_Depart, &szUserId, NULL);
			userList.removeAt(index);
			return;
		}
}

bool lmMessaging::addReceivedMsg(qint64 msgId, QString userId) {
	ReceivedMsg received(msgId, userId);

	if(receivedList.contains(received))
		return false;

	receivedList.append(received);
	return true;
}

void lmMessaging::addPendingMsg(qint64 msgId, MessageType type, QString* lpszUserId, MessageXml* pMessage) {
	MessageXml xmlMessage;
	if(pMessage)
		xmlMessage = pMessage->clone();
	pendingList.append(PendingMsg(msgId, true, QDateTime::currentDateTime(), type, *lpszUserId, xmlMessage, 0));
}

void lmMessaging::removePendingMsg(qint64 msgId) {
	for(int index = 0; index < pendingList.count(); index++) {
		if(pendingList[index].msgId == msgId) {
			pendingList[index].active = false;
			pendingList.removeAt(index);
			return;
		}
	}
}

void lmMessaging::removeAllPendingMsg(QString* lpszUserId) {
	for(int index = 0; index < pendingList.count(); index++) {
		if(pendingList[index].userId.compare(*lpszUserId) == 0) {
			pendingList.removeAt(index);
			index--;
		}
	}
}

void lmMessaging::checkPendingMsg(void) {
	for(int index = 0; index < pendingList.count(); index++) {
		//	check if message has timed out
		if(pendingList[index].active && pendingList[index].timeStamp.msecsTo(QDateTime::currentDateTime()) > nTimeout) {
			if(pendingList[index].retry < nMaxRetry) {
				//	send the message once more
				pendingList[index].retry++;
				pendingList[index].timeStamp = QDateTime::currentDateTime();
				resendMessage(pendingList[index].type, pendingList[index].msgId, &pendingList[index].userId, &pendingList[index].xmlMessage);
			}
			else {
				MessageXml statusMsg;
				//	max retries exceeded. mark message as failed.
				switch(pendingList[index].type) {
				case MT_Message:
					emit messageReceived(MT_Failed, &pendingList[index].userId, &pendingList[index].xmlMessage);
					break;
				case MT_Ping:
					statusMsg.addData(XN_STATUS, statusCode[ST_COUNT - 1]);
					emit messageReceived(MT_Status, &pendingList[index].userId, &statusMsg);
					emit messageReceived(MT_Depart, &pendingList[index].userId, NULL);
					removeUser(pendingList[index].userId);
					break;
                default:
                    break;
				}
				pendingList[index].active = false;
				pendingList.removeAt(index);
				index--;	//	since next item will have this index now
			}
		}
	}
}

void lmMessaging::resendMessage(MessageType type, qint64 msgId, QString* lpszUserId, MessageXml* pMessage) {
	if(lpszUserId && !getUser(lpszUserId))
		return;

	prepareMessage(type, msgId, true, lpszUserId, pMessage);
}
