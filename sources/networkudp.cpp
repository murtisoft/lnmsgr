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


#include "trace.h"
#include "networkudp.h"

lmNetworkUdp::lmNetworkUdp(void) {
	pUdpReceiver = new QUdpSocket(this);
	pUdpSender = new QUdpSocket(this);
	localId = QString();
	canReceive = false;
	isRunning = false;
	pCrypto = NULL;
    ipAddress = QHostAddress::Any;
    subnetMask = QHostAddress::Any;
	defBroadcast = QHostAddress::Broadcast;
	broadcastList.clear();
}

lmNetworkUdp::~lmNetworkUdp(void) {
}

void lmNetworkUdp::init(int nPort) {
	pSettings = new lmSettings();
	nUdpPort = nPort > 0 ? nPort : pSettings->value(IDS_UDPPORT, IDS_UDPPORT_VAL).toInt();
	multicastAddress = QHostAddress(pSettings->value(IDS_MULTICAST, IDS_MULTICAST_VAL).toString());
	int size = pSettings->beginReadArray(IDS_BROADCASTHDR);
	for(int index = 0; index < size; index++) {
		pSettings->setArrayIndex(index);
		QHostAddress address = QHostAddress(pSettings->value(IDS_BROADCAST).toString());
		if(!broadcastList.contains(address))
			broadcastList.append(address);
	}
	pSettings->endArray();
}

void lmNetworkUdp::start(void) {
	//	start receiving datagrams
	canReceive = startReceiving();
	isRunning = true;
}

void lmNetworkUdp::stop(void) {
	disconnect(pUdpReceiver, SIGNAL(readyRead()), this, SLOT(processPendingDatagrams()));
	if(pUdpReceiver->state() == QAbstractSocket::BoundState) {
		lmTrace::write("Leaving multicast group " + multicastAddress.toString() + " on interface " +
			multicastInterface.humanReadableName());
		bool left = pUdpReceiver->leaveMulticastGroup(multicastAddress, multicastInterface);
        pUdpReceiver->close();
		lmTrace::write((left ? "Success" : "Failed"));
	}
	isRunning = false;
}

void lmNetworkUdp::setLocalId(QString* lpszLocalId) {
	localId = *lpszLocalId;
}

void lmNetworkUdp::setCrypto(lmCrypto* pCrypto) {
	this->pCrypto = pCrypto;
}

void lmNetworkUdp::sendBroadcast(QString* lpszData) {
    if(!isRunning) {
        lmTrace::write("Warning: UDP server not running. Broadcast not sent");
        return;
    }

	QByteArray datagram = lpszData->toUtf8();
	sendDatagram(multicastAddress, datagram);
	for(int index = 0; index < broadcastList.count(); index++) {
		sendDatagram(broadcastList.at(index), datagram);
	}
}

void lmNetworkUdp::settingsChanged(void) {
	QHostAddress address = QHostAddress(pSettings->value(IDS_MULTICAST, IDS_MULTICAST_VAL).toString());
	if(multicastAddress != address) {
		if(pUdpReceiver->state() == QAbstractSocket::BoundState) {
			lmTrace::write("Leaving multicast group " + multicastAddress.toString() + " on interface " +
				multicastInterface.humanReadableName());
			bool left = pUdpReceiver->leaveMulticastGroup(multicastAddress, multicastInterface);
			lmTrace::write((left ? "Success" : "Failed"));
		}
		multicastAddress = address;
		lmTrace::write("Joining multicast group " + multicastAddress.toString() + " on interface " +
			multicastInterface.humanReadableName());
		bool joined = pUdpReceiver->joinMulticastGroup(multicastAddress, multicastInterface);
		lmTrace::write((joined ? "Success" : "Failed"));
	}
	broadcastList.clear();
	broadcastList.append(defBroadcast);
	int size = pSettings->beginReadArray(IDS_BROADCASTHDR);
	for(int index = 0; index < size; index++) {
		pSettings->setArrayIndex(index);
		QHostAddress address = QHostAddress(pSettings->value(IDS_BROADCAST).toString());
		if(!broadcastList.contains(address))
			broadcastList.append(address);
	}
	pSettings->endArray();
}

void lmNetworkUdp::setMulticastInterface(const QNetworkInterface& networkInterface) {
	multicastInterface = networkInterface;
}

void lmNetworkUdp::setIPAddress(const QString& szAddress, const QString& szSubnet) {
	ipAddress = QHostAddress(szAddress);
	subnetMask = QHostAddress(szSubnet);
	setDefaultBroadcast();
	if(!broadcastList.contains(defBroadcast))
		broadcastList.append(defBroadcast);
}

void lmNetworkUdp::processPendingDatagrams(void) {
	while(pUdpReceiver->hasPendingDatagrams()) {
		QByteArray datagram;
		datagram.resize(pUdpReceiver->pendingDatagramSize());
		QHostAddress address;
		pUdpReceiver->readDatagram(datagram.data(), datagram.size(), &address);
        QString szAddress = address.toString();
        parseDatagram(&szAddress, datagram);
	}
}

void lmNetworkUdp::sendDatagram(QHostAddress remoteAddress, QByteArray& datagram) {
	if(!isRunning)
		return;

	lmTrace::write("Sending UDP datagram to " + remoteAddress.toString() + ":" + QString::number(nUdpPort));
	pUdpSender->writeDatagram(datagram.data(), datagram.size(), remoteAddress, nUdpPort);
}

bool lmNetworkUdp::startReceiving(void) {
	lmTrace::write("Binding UDP listener to port " + QString::number(nUdpPort));

    if(pUdpReceiver->bind(QHostAddress::AnyIPv4, nUdpPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
		lmTrace::write("Success");
		lmTrace::write("Joining multicast group " + multicastAddress.toString() +
			" on interface " + multicastInterface.humanReadableName());
		bool joined = pUdpReceiver->joinMulticastGroup(multicastAddress, multicastInterface);
		lmTrace::write((joined ? "Success" : "Failed"));
		connect(pUdpReceiver, SIGNAL(readyRead()), this, SLOT(processPendingDatagrams()));
		return true;
	}

	lmTrace::write("Failed");
	return false;
}

void lmNetworkUdp::parseDatagram(QString* lpszAddress, QByteArray& baDatagram) {
	lmTrace::write("UDP datagram received from " + *lpszAddress);
	DatagramHeader* pHeader = new DatagramHeader(DT_Broadcast, QString(), *lpszAddress);
	QString szData = QString::fromUtf8(baDatagram.data(), baDatagram.length());
	emit broadcastReceived(pHeader, &szData);
}

void lmNetworkUdp::setDefaultBroadcast(void) {
	if(ipAddress.protocol() != QAbstractSocket::IPv4Protocol)
		return;

	//	The network broadcast address is obtained by ORing the host address and
	//	bit inversed subnet mask
	quint32 ipv4 = ipAddress.toIPv4Address();
	quint32 invMask = ~(subnetMask.toIPv4Address());
	defBroadcast = QHostAddress((ipv4 | invMask));
}
