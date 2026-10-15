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

#ifndef FIREWALLCHECK_H
#define FIREWALLCHECK_H

#include <QObject>

class QThread;

//	Detects a Windows Firewall rule that blocks this program from receiving connections. Windows creates such a
//	rule when the user cancels its "Allow access" prompt, and never asks again afterwards.
//	The check needs no admin rights and runs on its own thread, enumerating firewall rules can be slow.
//	On other platforms nothing happens.
class lmFirewallCheck : public QObject {
	Q_OBJECT

public:
	explicit lmFirewallCheck(QObject* parent = nullptr);
	~lmFirewallCheck(void) override;

	//	Start looking, the result comes as the blocked() signal. Does nothing if a check is already running.
	void start(void);

signals:
	//	An enabled inbound block rule for this executable exists on an active, enabled firewall profile.
	void blocked(void);

private:
	QThread* m_thread = nullptr;
};

#endif // FIREWALLCHECK_H
