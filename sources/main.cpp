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
#include <QCommandLineParser>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#ifdef Q_OS_WIN
#include <windows.h>
#include <cstdio>
#endif

//	Define a unique application id. This is a combination of two GUIDs
const QString appId = "93fab548-2cf5-4a1e-8758-a416a5ec2120-6fc5009f-84e8-4489-a444-7f934bcf9166";

#ifdef Q_OS_WIN
//	A GUI program has no console of its own. Attach to the one it was started from, unless
//	the output is already redirected to a file or pipe.
static void attachParentConsole(void) {
    static bool done = false;
    if(done)
        return;
    done = true;

    auto isValid = [](DWORD id) {
        HANDLE handle = GetStdHandle(id);
        return handle != nullptr && handle != INVALID_HANDLE_VALUE;
    };
    bool needOut = !isValid(STD_OUTPUT_HANDLE);
    bool needErr = !isValid(STD_ERROR_HANDLE);
    if(!needOut && !needErr)
        return;
    if(!AttachConsole(ATTACH_PARENT_PROCESS))
        return;

    FILE* file = nullptr;
    if(needOut)
        freopen_s(&file, "CONOUT$", "w", stdout);
    if(needErr)
        freopen_s(&file, "CONOUT$", "w", stderr);
}
#endif

static void consoleWrite(const QString& text, bool toStderr = false) {
#ifdef Q_OS_WIN
    attachParentConsole();
#endif
    QTextStream(toStderr ? stderr : stdout) << text << Qt::flush;
}

static int usageError(const QString& message) {
    consoleWrite(QString("Error: %1\nTry --help for the list of switches.\n").arg(message), true);
    return 2;
}

//	Only these are forwarded to an already running instance. Everything else only matters
//	when starting up.
static bool isAction(const QString& option) {
    static const QStringList actions = {
        LM_OPT_RESET_HISTORY, LM_OPT_RESET_FILEHIST, LM_OPT_RESET_CONFIG, LM_OPT_TERMINATE
    };
    return actions.contains(option) || option.startsWith(QString(LM_OPT_AUTOSTART) + "=");
}

static QStringList remoteOptions(const QStringList& options) {
    QStringList result;
    for(const QString& option : options)
        if(isAction(option))
            result << option;
    return result;
}

//	switches as the user would have typed them, for messages
static QString switchList(const QStringList& options, bool actions) {
    QStringList names;
    for(const QString& option : options)
        if(isAction(option) == actions)
            names << "--" + option;
    return names.join(", ");
}

int main(int argc, char *argv[]) {
    Application application(appId, argc, argv);

    QApplication::setApplicationName(IDA_PRODUCT);
    QApplication::setOrganizationName(IDA_COMPANY);
    QApplication::setOrganizationDomain(IDA_DOMAIN);

    // Command line
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "LAN Messenger\n\n"
        "All switches are optional. Some are mutually exclusive; if several are given,\n"
        "they take precedence in this order: --help, --version, --is-running, --terminate.\n\n"
        "While an instance is already running, only --autostart, --reset-history,\n"
        "--reset-file-history, --reset-config and --terminate are passed on to it;\n"
        "the other switches only apply when starting up and are ignored.");
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    QCommandLineOption optHelp = parser.addHelpOption();
    QCommandLineOption optVersion = parser.addVersionOption();
    QCommandLineOption optLoopback("loopback", "Allow loopback communication with the local machine.");
    QCommandLineOption optPort("port", "Use <number> for this session instead of the configured port. All peers must use the same port to see each other.", "number");
    QCommandLineOption optConfig("config", "Import preferences from <file> at startup.", "file");
    QCommandLineOption optTrace("trace", "Enable debug logging, overriding the preference.");
    QCommandLineOption optResetHistory("reset-history", "Delete existing chat history.");
    QCommandLineOption optResetFileHistory("reset-file-history", "Delete existing file transfer history.");
    QCommandLineOption optResetConfig("reset-config", "Reset preferences to default values.");
    QCommandLineOption optAutostart("autostart", "Set whether the application starts with the system.", "true|false");
    QCommandLineOption optIsRunning("is-running", "Exit with code 1 if an instance is already running, 0 if not.");
    QCommandLineOption optTerminate("terminate", "Close the already running instance.");
    QCommandLineOption optExitAfterArgs("exit-after-args", "Exit once the other switches have been processed.");
    parser.addOptions({optLoopback, optPort, optConfig, optTrace,
                       optResetHistory, optResetFileHistory, optResetConfig,
                       optAutostart, optIsRunning, optTerminate, optExitAfterArgs});

    if(!parser.parse(QApplication::arguments()))
        return usageError(parser.errorText());
    if(parser.isSet(optHelp)) {
        consoleWrite(parser.helpText());
        return 0;
    }
    if(parser.isSet(optVersion)) {
        consoleWrite(QString("%1 %2\n").arg(QString(IDA_PRODUCT), QString(IDA_VERSION)));
        return 0;
    }
    if(!parser.positionalArguments().isEmpty())
        return usageError(QString("unexpected argument '%1'").arg(parser.positionalArguments().first()));

    QStringList options;
    if(parser.isSet(optLoopback))
        options << LM_OPT_LOOPBACK;
    if(parser.isSet(optTrace))
        options << LM_OPT_TRACE;
    if(parser.isSet(optResetHistory))
        options << LM_OPT_RESET_HISTORY;
    if(parser.isSet(optResetFileHistory))
        options << LM_OPT_RESET_FILEHIST;
    if(parser.isSet(optResetConfig))
        options << LM_OPT_RESET_CONFIG;
    if(parser.isSet(optTerminate))
        options << LM_OPT_TERMINATE;
    if(parser.isSet(optExitAfterArgs))
        options << LM_OPT_EXIT_AFTER_ARGS;
    if(parser.isSet(optAutostart)) {
        QString value = parser.value(optAutostart).toLower();
        if(value == "true" || value == "yes" || value == "on" || value == "1")
            options << QString(LM_OPT_AUTOSTART) + "=true";
        else if(value == "false" || value == "no" || value == "off" || value == "0")
            options << QString(LM_OPT_AUTOSTART) + "=false";
        else
            return usageError(QString("invalid value '%1' for --autostart (expected true or false)").arg(parser.value(optAutostart)));
    }
    if(parser.isSet(optPort)) {
        bool ok = false;
        int port = parser.value(optPort).toInt(&ok);
        if(!ok || port < 1 || port > 65535)
            return usageError(QString("invalid port '%1' (expected 1-65535)").arg(parser.value(optPort)));
        options << QString(LM_OPT_PORT) + "=" + QString::number(port);
    }
    if(parser.isSet(optConfig)) {
        //	made absolute here, since the working directory is changed below
        QFileInfo info(parser.value(optConfig));
        if(!info.isFile())
            return usageError(QString("config file '%1' does not exist").arg(parser.value(optConfig)));
        options << QString(LM_OPT_CONFIG) + "=" + info.absoluteFilePath();
    }

    // Single instance check. If another instance is running, hand it the actions (none at all
    // just brings its window forward) and exit silently.
    {
        QLocalSocket socket;
        socket.connectToServer(appId);
        if(socket.waitForConnected(500)) {
            if(parser.isSet(optIsRunning)) {
                consoleWrite("An instance is already running.\n");
                return 1;
            }

#ifdef Q_OS_WIN
            AllowSetForegroundWindow(ASFW_ANY);   // let the running instance take focus
#endif
            socket.write(remoteOptions(options).join("\n").toUtf8());
            socket.flush();
            socket.waitForBytesWritten(1000);
            socket.disconnectFromServer();
            if(socket.state() != QLocalSocket::UnconnectedState)
                socket.waitForDisconnected(1000);

            // only say something if switches were given; a plain second start stays silent
            QString sent = switchList(options, true);
            QString ignored = switchList(options, false);
            if(!sent.isEmpty())
                consoleWrite("An instance is already running. Sent to it: " + sent + "\n");
            if(!ignored.isEmpty())
                consoleWrite("An instance is already running. Ignored, as these only apply at startup: " + ignored + "\n", true);
            return 0;
        }
    }

    //	nothing is running, so there is nothing to report or terminate
    if(parser.isSet(optIsRunning)) {
        consoleWrite("No instance is running.\n");
        return 0;
    }
    if(parser.isSet(optTerminate)) {
        consoleWrite("No instance is running, nothing to terminate.\n");
        return 0;
    }

    // Mark this as the running instance
    QLocalServer server;
    if(!server.listen(appId)) {
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
    //	some options are handled when the application is initializing
    core.init(options);
    for(const QString& warning : core.startupWarnings)
        consoleWrite("Warning: " + warning + "\n", true);
    options << LM_OPT_NEW;	//	indicates this is a new instance
    //	the remaining options are handled after initializing all the layers
    if(!core.receiveAppMessage(options))
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

    // Receive actions (or a plain "bring me forward") from later instances
    QObject::connect(&server, &QLocalServer::newConnection, &server, [&server, &core]() {
        while(QLocalSocket* client = server.nextPendingConnection()) {
            auto buffer = new QByteArray;
            QObject::connect(client, &QLocalSocket::readyRead, client, [client, buffer]() {
                buffer->append(client->readAll());
            });
            QObject::connect(client, &QLocalSocket::disconnected, client, [client, buffer, &core]() {
                buffer->append(client->readAll());
                QStringList message = QString::fromUtf8(*buffer).split("\n", Qt::SkipEmptyParts);
                delete buffer;
                client->deleteLater();
                core.receiveAppMessage(message);
            });
        }
    });

    return application.exec();
}
