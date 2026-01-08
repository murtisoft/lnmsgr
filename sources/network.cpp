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


#include "trace.h"
#include "network.h"

lmNetwork::lmNetwork(void) {
	pUdpNetwork = new lmNetworkUdp();
	pTcpNetwork = new lmNetworkTcp();
	pWebNetwork = new lmNetworkWeb();
	connect(pUdpNetwork, SIGNAL(broadcastReceived(DatagramHeader*, QString*)), 
		this, SLOT(udp_receiveBroadcast(DatagramHeader*, QString*)));
	connect(pTcpNetwork, SIGNAL(newConnection(QString*, QString*)),
		this, SLOT(tcp_newConnection(QString*, QString*)));
	connect(pTcpNetwork, SIGNAL(connectionLost(QString*)),
		this, SLOT(tcp_connectionLost(QString*)));
	connect(pTcpNetwork, SIGNAL(messageReceived(DatagramHeader*, QString*)), 
		this, SLOT(tcp_receiveMessage(DatagramHeader*, QString*)));
	connect(pTcpNetwork, SIGNAL(progressReceived(QString*, QString*)),
		this, SLOT(tcp_receiveProgress(QString*, QString*)));
	connect(pWebNetwork, SIGNAL(messageReceived(QString*)),
		this, SLOT(web_receiveMessage(QString*)));
	pTimer = NULL;
	pCrypto = new lmCrypto();
	ipAddress = QString();
	subnetMask = QString();
	networkInterface = QNetworkInterface();
    interfaceName = QString();
	isConnected = false;
	canReceive = false;
}

lmNetwork::~lmNetwork(void) {
}

void lmNetwork::init(MessageXml *pInitParams) {
	lmTrace::write("Network initialized");

	pSettings = new lmSettings();
	isConnected = getIPAddress();

    lmTrace::write("Network interface selected: " + (networkInterface.isValid() ? networkInterface.humanReadableName() : "None") +
                    "\nIP address obtained: " + (ipAddress.isEmpty() ? "NULL" : ipAddress) +
					"\nSubnet mask obtained: " + (subnetMask.isEmpty() ? "NULL" : subnetMask) +
					"\nConnection status: " + (isConnected ? "OK" : "Fail"));

	int port = pInitParams->data(XN_PORT).toInt();
	pUdpNetwork->init(port);
	pTcpNetwork->init(port);
}

void lmNetwork::start(void) {
	lmTrace::write("Network started");
	pCrypto->generateRSA();

	pTimer = new QTimer(this);
	connect(pTimer, SIGNAL(timeout()), this, SLOT(timer_timeout()));
	pTimer->start(2000);

	pUdpNetwork->setCrypto(pCrypto);
	pTcpNetwork->setCrypto(pCrypto);
	if(isConnected) {
		pUdpNetwork->setMulticastInterface(networkInterface);
		pUdpNetwork->setIPAddress(ipAddress, subnetMask);
		pUdpNetwork->start();
		pTcpNetwork->setIPAddress(ipAddress);
		pTcpNetwork->start();
		canReceive = pUdpNetwork->canReceive;
	}
}

void lmNetwork::stop(void) {
	pTimer->stop();

	pUdpNetwork->stop();
	pTcpNetwork->stop();

	lmTrace::write("Network stopped");
}

QString lmNetwork::physicalAddress(void) {
    if(networkInterface.isValid())
        return networkInterface.hardwareAddress();

    return QString();

//	//	get the first active network interface
//	QNetworkInterface networkInterface;

//	if(getNetworkInterface(&networkInterface))
//		return networkInterface.hardwareAddress();

//	return QString();
}

void lmNetwork::setLocalId(QString* lpszLocalId) {
	pUdpNetwork->setLocalId(lpszLocalId);
	pTcpNetwork->setLocalId(lpszLocalId);
}

void lmNetwork::sendBroadcast(QString* lpszData) {
	pUdpNetwork->sendBroadcast(lpszData);
}

void lmNetwork::addConnection(QString* lpszUserId, QString* lpszAddress) {
	pTcpNetwork->addConnection(lpszUserId, lpszAddress);
}

void lmNetwork::sendMessage(QString* lpszReceiverId, QString* lpszAddress, QString* lpszData) {
    Q_UNUSED(lpszAddress);
	pTcpNetwork->sendMessage(lpszReceiverId, lpszData);
}

void lmNetwork::initSendFile(QString* lpszReceiverId, QString* lpszAddress, QString* lpszData) {
	pTcpNetwork->initSendFile(lpszReceiverId, lpszAddress, lpszData);
}

void lmNetwork::initReceiveFile(QString* lpszSenderId, QString* lpszAddress, QString* lpszData) {
	pTcpNetwork->initReceiveFile(lpszSenderId, lpszAddress, lpszData);
}

void lmNetwork::fileOperation(FileMode mode, QString* lpszUserId, QString* lpszData) {
	pTcpNetwork->fileOperation(mode, lpszUserId, lpszData);
}

void lmNetwork::sendWebMessage(QString *lpszUrl, QString *lpszData) {
	pWebNetwork->sendMessage(lpszUrl, lpszData);
}

void lmNetwork::settingsChanged(void) {
	pUdpNetwork->settingsChanged();
	pTcpNetwork->settingsChanged();
}

void lmNetwork::timer_timeout(void) {
	bool prev = isConnected;
    isConnected = getIPAddress(false);

	if(prev != isConnected) {
        lmTrace::write("Network interface selected: " + (networkInterface.isValid() ? networkInterface.humanReadableName() : "None") +
            "\nIP address obtained: " + (ipAddress.isEmpty() ? "NULL" : ipAddress) +
			"\nSubnet mask obtained: " + (subnetMask.isEmpty() ? "NULL" : subnetMask) +
			"\nConnection status: " + (isConnected ? "OK" : "Fail"));

		if(isConnected) {
			pUdpNetwork->setMulticastInterface(networkInterface);
			pUdpNetwork->setIPAddress(ipAddress, subnetMask);
			pUdpNetwork->start();
			pTcpNetwork->setIPAddress(ipAddress);
			pTcpNetwork->start();
			canReceive = pUdpNetwork->canReceive;
		} else {
			pUdpNetwork->stop();
			pTcpNetwork->stop();
		}
        emit connectionStateChanged();
	}
}

void lmNetwork::udp_receiveBroadcast(DatagramHeader* pHeader, QString* lpszData) {
	emit broadcastReceived(pHeader, lpszData);
}

void lmNetwork::tcp_newConnection(QString* lpszUserId, QString* lpszAddress) {
	emit newConnection(lpszUserId, lpszAddress);
}

void lmNetwork::tcp_connectionLost(QString* lpszUserId) {
	emit connectionLost(lpszUserId);
}

void lmNetwork::tcp_receiveMessage(DatagramHeader* pHeader, QString* lpszData) {
	emit messageReceived(pHeader, lpszData);
}

void lmNetwork::tcp_receiveProgress(QString* lpszUserId, QString* lpszData) {
	emit progressReceived(lpszUserId, lpszData);
}

void lmNetwork::web_receiveMessage(QString *lpszData) {
	emit webMessageReceived(lpszData);
}

bool lmNetwork::getIPAddress(bool verbose) {
	// If an interface is already being used, get it. Ignore all others
    networkInterface = QNetworkInterface::interfaceFromName(interfaceName);
	if(networkInterface.isValid()) {
		QNetworkAddressEntry addressEntry;
        if(isInterfaceUp(&networkInterface) && getIPAddress(&networkInterface, &addressEntry)) {
			ipAddress = addressEntry.ip().toString();
			subnetMask = addressEntry.netmask().toString();
			return true;
		}
		ipAddress = QString();
		subnetMask = QString();
		return false;
	}

    // Currently, not using preferred connection, since using preferred connection is not
    // working properly.
	// Get the preferred interface name from settings if checking for the first time
    //if(szInterfaceName.isNull())
    //	szInterfaceName = pSettings->value(IDS_CONNECTION, IDS_CONNECTION_VAL).toString();

	//bool usePreferred = (szInterfaceName.compare(IDS_CONNECTION_VAL, Qt::CaseInsensitive) != 0);
	bool usePreferred = false;

    lmTrace::write("Checking for active network interface...", verbose);

	//	get a list of all network interfaces available in the system
	QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();

    bool activeFound = false;

	//	return the preferred interface if it is active
	for(int index = 0; index < allInterfaces.count(); index++) {
		// Skip to the next interface if it is not the preferred one
		// Checked only if searching for the preferred adapter
        if(usePreferred && interfaceName.compare(allInterfaces[index].name()) != 0)
			continue;

		if(isInterfaceUp(&allInterfaces[index])) {
            activeFound = true;
            lmTrace::write("Active network interface found: " + allInterfaces[index].humanReadableName(),
                verbose);
			QNetworkAddressEntry addressEntry;
			if(getIPAddress(&allInterfaces[index], &addressEntry)) {
				ipAddress = addressEntry.ip().toString();
				subnetMask = addressEntry.netmask().toString();
				networkInterface = allInterfaces[index];
                interfaceName = allInterfaces[index].name();
				return true;
			}
		}
	}

    lmTrace::write(QString("Warning: ") + (activeFound ? "No IP address found" : "No active network interface found"),
        verbose);
	ipAddress = QString();
	subnetMask = QString();
	return false;
}

bool lmNetwork::getIPAddress(QNetworkInterface* pNetworkInterface, QNetworkAddressEntry *pAddressEntry) {
	//lmTrace::write("Querying IP address from network interface...");

	//	get a list of all associated ip addresses of the interface
	QList<QNetworkAddressEntry> addressEntries = pNetworkInterface->addressEntries();
	//	return the first address which is an ipv4 address
	for(int index = 0; index < addressEntries.count(); index++) {
		if(addressEntries[index].ip().protocol() == QAbstractSocket::IPv4Protocol) {
			*pAddressEntry = addressEntries[index];
			//lmTrace::write("IPv4 address found for network interface.");
			return true;
		}
	}
	// if ipv4 address is not present, check for ipv6 address
	for(int index = 0; index < addressEntries.count(); index++) {
		if(addressEntries[index].ip().protocol() == QAbstractSocket::IPv6Protocol) {
			*pAddressEntry = addressEntries[index];
			//lmTrace::write("IPv6 address found for network interface.");
			return true;
		}
	}

	//lmTrace::write("Warning: No IP address found for network interface.");
	return false;
}

bool lmNetwork::getNetworkInterface(QNetworkInterface* pNetworkInterface) {
	// If an interface is already being used, get it. Ignore all others
	if(networkInterface.isValid()) {
		*pNetworkInterface = networkInterface;
		return isInterfaceUp(pNetworkInterface);
	}

	// Get the preferred interface name from settings if checking for the first time
    if(interfaceName.isNull())
        interfaceName = pSettings->value(IDS_CONNECTION, IDS_CONNECTION_VAL).toString();

    QString szPreferred = interfaceName;
	// Currently, hard coding usePreferred to False, since using preferred connection is not
	// working properly.
	//bool usePreferred = (szPreferred.compare(IDS_CONNECTION_VAL, Qt::CaseInsensitive) != 0);
	bool usePreferred = false;

	// Return true if preferred interface is available
	if(usePreferred && getNetworkInterface(pNetworkInterface, &szPreferred))
		return true;

	// Return true if a fallback interface is available
	if(!usePreferred && getNetworkInterface(pNetworkInterface, NULL))
		return true;

	return false;
}

bool lmNetwork::getNetworkInterface(QNetworkInterface* pNetworkInterface, QString* lpszPreferred) {
	lmTrace::write("Checking for active network interface...");

	//	get a list of all network interfaces available in the system
	QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();

	//	return the preferred interface if it is active
	for(int index = 0; index < allInterfaces.count(); index++) {
		// Skip to the next interface if it is not the preferred one
		// Checked only if searching for the preferred adapter
		if(lpszPreferred && lpszPreferred->compare(allInterfaces[index].name()) != 0)
			continue;

		if(isInterfaceUp(&allInterfaces[index])) {
			*pNetworkInterface = allInterfaces[index];
			lmTrace::write("Active network interface found: " + pNetworkInterface->humanReadableName());
			return true;
		}
	}

	lmTrace::write("Warning: No active network interface found");
	return false;
}

bool lmNetwork::isInterfaceUp(QNetworkInterface* pNetworkInterface) {
	if(pNetworkInterface->flags().testFlag(QNetworkInterface::IsUp)
		&& pNetworkInterface->flags().testFlag(QNetworkInterface::IsRunning)
		&& !pNetworkInterface->flags().testFlag(QNetworkInterface::IsLoopBack)) {
			return true;
	}

	return false;
}

bool lmNetwork::getNetworkAddressEntry(QNetworkAddressEntry* pAddressEntry) {
	//	get the first active network interface
	QNetworkInterface networkInterface;

	if(getNetworkInterface(&networkInterface)) {
		//lmTrace::write("Querying IP address from network interface...");

		//	get a list of all associated ip addresses of the interface
		QList<QNetworkAddressEntry> addressEntries = networkInterface.addressEntries();
		//	return the first address which is an ipv4 address
		for(int index = 0; index < addressEntries.count(); index++) {
			if(addressEntries[index].ip().protocol() == QAbstractSocket::IPv4Protocol) {
				*pAddressEntry = addressEntries[index];
				this->networkInterface = networkInterface;
                this->interfaceName = networkInterface.name();
				//lmTrace::write("IPv4 address found for network interface.");
				return true;
			}
		}
		// if ipv4 address is not present, check for ipv6 address
		for(int index = 0; index < addressEntries.count(); index++) {
			if(addressEntries[index].ip().protocol() == QAbstractSocket::IPv6Protocol) {
				*pAddressEntry = addressEntries[index];
				this->networkInterface = networkInterface;
                this->interfaceName = networkInterface.name();
				//lmTrace::write("IPv6 address found for network interface.");
				return true;
			}
		}

		//lmTrace::write("Warning: No IP address found for network interface.");
	}

	return false;
}
