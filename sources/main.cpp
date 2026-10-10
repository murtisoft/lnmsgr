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

#include "translationloader.h"
#include "lmcore.h"
#include "definitionsdir.h"
#include <QResource>
#include <QMessageBox>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSslSocket>
#include <QDir>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

//	Define a unique application id. This is a combination of two GUIDs
const QString appId = "93fab548-2cf5-4a1e-8758-a416a5ec2120-6fc5009f-84e8-4489-a444-7f934bcf9166";

int showSwitches(void) {
    QString msg =   "Usage:    lm [switches]\n" \
        "    All switches are optional.\n" \
        "\n" \
        "/loopback - Allows loopback communication with local machine.\n" \
        "/nohistory - Deletes existing history.\n" \
        "/nofilehistory - Deletes existing file transfer history.\n" \
        "/noconfig - Resets preferences to default values.\n" \
        "/sync - Synchronizes application settings and their external dependencies.\n" \
        "/unsync - Removes all external application dependencies.\n" \
        "/term - Closes an instance which is already running.\n" \
        "/inst - Returns a value indicating whether an instance is already running.\n" \
        "/quit - Closes once the command line parameters are processed.\n" \
        "/? - Display this help.\n" \
        "\n" \
        "Example:\n" \
        "Start in loopback mode with default settings: lm /loopback /noconfig\n" \
        "\n" \
        "Some command line switches are mutually exclusive. If multiple switches\n" \
        "are specified, they will take precedence in the order given above.\n";
    QMessageBox::information(NULL, IDA_TITLE, msg, QMessageBox::Ok);
    return 0;
}

int main(int argc, char *argv[]) {
    Application application(appId, argc, argv);

    QApplication::setApplicationName(IDA_PRODUCT);
    QApplication::setOrganizationName(IDA_COMPANY);
    QApplication::setOrganizationDomain(IDA_DOMAIN);

    QString messageList;
    QStringList arguments = QApplication::arguments();
    bool instSwitch = false;

    for(int index = 0; index < arguments.count(); index++) {
        if(arguments.at(index).compare("/?", Qt::CaseInsensitive) == 0)
            return showSwitches();
        else if(arguments.at(index).compare("/inst", Qt::CaseInsensitive) == 0)
            instSwitch = true;
        else
            messageList += arguments.at(index) + "\n";
    }

    // Single instance check. If another instance is running, hand our command
    // line over to it (an empty message just brings its window forward) and exit silently.
    {
        QLocalSocket socket;
        socket.connectToServer(appId);
        if (socket.waitForConnected(500)) {
            if (instSwitch)
                return 1;   // /inst: an instance is already running

#ifdef Q_OS_WIN
            AllowSetForegroundWindow(ASFW_ANY);   // let the running instance take focus
#endif
            // argument 0 is the executable path, only the switches are sent
            QStringList switches = arguments.mid(1);
            switches.removeAll(QString());
            socket.write(switches.join("\n").toUtf8());
            socket.flush();
            socket.waitForBytesWritten(1000);
            socket.disconnectFromServer();
            if (socket.state() != QLocalSocket::UnconnectedState)
                socket.waitForDisconnected(1000);
            return 0;
        }
    }

    if (instSwitch)
        return 0;   // /inst: no instance is running

    // Mark this as the running instance
    QLocalServer server;
    if (!server.listen(appId)) {
        QLocalServer::removeServer(appId);   // stale socket from a crashed instance
        server.listen(appId);
    }

    QDir::setCurrent(QApplication::applicationDirPath());

#ifdef Q_OS_MAC
    QDir dir(QApplication::applicationDirPath());
    dir.cdUp();
    dir.cd("Plugins");
    QApplication::setLibraryPaths(QStringList(dir.absolutePath()));
#endif

    QApplication::setWindowIcon(QIcon(IDR_APPICON));

    application.loadTranslations(DefinitionsDir::resLangDir());
    application.loadTranslations(DefinitionsDir::sysLangDir());
    application.loadTranslations(DefinitionsDir::userLangDir());

    lmCore core;
    //	handle command line args if this is first instance
    //	some args are handled when the application is initializing
    core.init(messageList);
    messageList += "/new\n";	//	indicates this is a new instance
    //	the remaining args are handled after initializing all the layers
    if(!core.receiveAppMessage(messageList))
        return 0;
    if(!core.start())
        return 1;

#ifdef QT_NO_SSL
    if(QMessageBox::critical(nullptr, IDA_TITLE, "Qt is compiled with QT_NO_SSL. Quit application?", QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
        return 2;
#else
    if(!QSslSocket::supportsSsl()) {
        if(QMessageBox::critical(nullptr, IDA_TITLE, "Messenger can not find ssl at startup. Probably missing openssl dll. Quit application?", QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
            return 3;
    }
#endif

    QObject::connect(&application, SIGNAL(aboutToQuit()), &core, SLOT(aboutToExit()));

    // Receive command lines (or a plain "bring me forward") from later instances
    QObject::connect(&server, &QLocalServer::newConnection, &server, [&server, &core]() {
        while (QLocalSocket* client = server.nextPendingConnection()) {
            auto buffer = new QByteArray;
            QObject::connect(client, &QLocalSocket::readyRead, client, [client, buffer]() {
                buffer->append(client->readAll());
            });
            QObject::connect(client, &QLocalSocket::disconnected, client, [client, buffer, &core]() {
                buffer->append(client->readAll());
                QString message = QString::fromUtf8(*buffer);
                delete buffer;
                client->deleteLater();
                core.receiveAppMessage(message);
            });
        }
    });

    return application.exec();
}
