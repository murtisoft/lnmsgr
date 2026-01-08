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


#ifndef NETWORKUDP_H
#define NETWORKUDP_H

#include <QObject>
#include <QUdpSocket>
#include <QNetworkAddressEntry>
#include <QHostAddress>
#include <QList>
#include "shared.h"
#include "datagram.h"
#include "settings.h"
#include "crypto.h"

class lmNetworkUdp : public QObject {
	Q_OBJECT

public:
	lmNetworkUdp(void);
	~lmNetworkUdp(void);

	void init(int nPort = 0);
	void start(void);
	void stop(void);
	void setLocalId(QString* lpszLocalId);
	void setCrypto(lmCrypto* pCrypto);
	void sendBroadcast(QString* lpszData);
	void settingsChanged(void);
	void setMulticastInterface(const QNetworkInterface& networkInterface);
	void setIPAddress(const QString& szAddress, const QString& szSubnet);

	bool isConnected;
	bool canReceive;

signals:
	void broadcastReceived(DatagramHeader* pHeader, QString* lpszData);
	void connectionStateChanged(void);

private slots:
	void processPendingDatagrams(void);

private:
	void sendDatagram(QHostAddress remoteAddress, QByteArray& baDatagram);
	bool startReceiving(void);
	void parseDatagram(QString* lpszAddress, QByteArray& baDatagram);
	void setDefaultBroadcast(void);

	lmSettings*		pSettings;
	QUdpSocket*			pUdpReceiver;
	QUdpSocket*			pUdpSender;
	lmCrypto*			pCrypto;

	bool				isRunning;
    quint16				nUdpPort;
	QHostAddress		multicastAddress;
	QString				localId;
	QNetworkInterface	multicastInterface;
	QHostAddress		ipAddress;
	QHostAddress		subnetMask;
	QList<QHostAddress>	broadcastList;
	QHostAddress		defBroadcast;
};

#endif // NETWORKUDP_H
