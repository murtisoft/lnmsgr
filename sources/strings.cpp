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


#include "strings.h"

QString lmStrings::m_appName;
QString lmStrings::m_appDesc;
QString lmStrings::m_autoConn;
QStringList lmStrings::m_fontSize;
QStringList lmStrings::m_statusDesc;
QStringList lmStrings::m_soundDesc;
QStringList lmStrings::m_awayTimeDesc;
QStringList lmStrings::m_userListView;

lmStrings::lmStrings(void) {
}

lmStrings::~lmStrings(void) {
}

void lmStrings::retranslate(void) {
	m_appName.clear();
	m_appDesc.clear();
	m_autoConn.clear();
	m_fontSize.clear();
	m_statusDesc.clear();
	m_soundDesc.clear();
	m_awayTimeDesc.clear();
	m_userListView.clear();
}

const QString lmStrings::appName(void) {
	if(m_appName.isEmpty())
        m_appName = tr("LAN Messenger");
	return m_appName;
}

const QString lmStrings::appDesc(void) {
	if(m_appDesc.isEmpty())
        m_appDesc = tr("LAN Messenger is a free peer-to-peer messaging application for\n"\
                       "intra-network communication and does not require a server.\n"\
                       "LAN Messenger works on essentially every popular desktop platform.\n"\
                       "\n"\
                       "You can support LAN Messenger by contributing code, translations,\n"\
                       "bug fixes or reports.\n"\
                       "Help us grow by sharing the project with others!");
	return m_appDesc;
}

const QString lmStrings::autoConn(void) {
	if(m_autoConn.isEmpty())
		m_autoConn = tr("Automatic");
	return m_autoConn;
}

const QStringList lmStrings::fontSize(void) {
	if(m_fontSize.isEmpty()) {
		m_fontSize.append(tr("Small text"));
		m_fontSize.append(tr("Medium text"));
		m_fontSize.append(tr("Large text"));
	}
	return m_fontSize;
}

const QStringList lmStrings::statusDesc(void) {
	if(m_statusDesc.isEmpty()) {
		m_statusDesc.append(tr("Available"));
		m_statusDesc.append(tr("Busy"));
		m_statusDesc.append(tr("Do Not Disturb"));
		m_statusDesc.append(tr("Be Right Back"));
		m_statusDesc.append(tr("Away"));
		m_statusDesc.append(tr("Appear Offline"));
	}
	return m_statusDesc;
}

const QStringList lmStrings::soundDesc(void) {
	if(m_soundDesc.isEmpty()) {
		m_soundDesc.append(tr("Incoming message"));
		m_soundDesc.append(tr("User is online"));
		m_soundDesc.append(tr("User is offline"));
		m_soundDesc.append(tr("Incoming file transfer"));
		m_soundDesc.append(tr("File transfer completed"));
		m_soundDesc.append(tr("Incoming public message"));
        m_soundDesc.append(tr("Microphone on"));
        m_soundDesc.append(tr("Microphone off"));
        m_soundDesc.append(tr("Camera on"));
        m_soundDesc.append(tr("Camera off"));
        m_soundDesc.append(tr("Speaker on"));
        m_soundDesc.append(tr("Speaker off"));
        m_soundDesc.append(tr("Incoming call"));
        m_soundDesc.append(tr("Outgoing call"));
        m_soundDesc.append(tr("Nudge"));
        m_soundDesc.append(tr("Talking while microphone muted"));
	}
	return m_soundDesc;
}

const QStringList lmStrings::awayTimeDesc(void) {
    if(m_awayTimeDesc.isEmpty()) {
        m_awayTimeDesc.append(tr("5 minutes"));
        m_awayTimeDesc.append(tr("10 minutes"));
        m_awayTimeDesc.append(tr("15 minutes"));
        m_awayTimeDesc.append(tr("20 minutes"));
        m_awayTimeDesc.append(tr("30 minutes"));
        m_awayTimeDesc.append(tr("45 minutes"));
        m_awayTimeDesc.append(tr("60 minutes"));
        m_awayTimeDesc.append(tr("Never"));
    }
    return m_awayTimeDesc;
}

const QStringList lmStrings::userListView(void) {
	if(m_userListView.isEmpty()) {
		m_userListView.append(tr("Detailed"));
		m_userListView.append(tr("Compact"));
	}
	return m_userListView;
}
