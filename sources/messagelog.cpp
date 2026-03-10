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


#include <QMenu>
#include <QAction>
#include <QScrollBar>
#include <QTextBlock>
#include "messagelog.h"
#include <QRegularExpression>
#include <QLocale>
#include "chathelper.h"

const QString acceptOp("accept");
const QString declineOp("decline");
const QString cancelOp("cancel");

lmMessageLog::lmMessageLog(QWidget *parent) : MessageBrowser (parent) {
// TODO long-ass-fucking-time-ago
//	connect(this, SIGNAL(linkClicked(QUrl)), this, SLOT(log_linkClicked(QUrl)));
//	connect(this->page(), SIGNAL(linkHovered(QString, QString, QString)),
//			this, SLOT(log_linkHovered(QString, QString, QString)));

    connect(this, SIGNAL(anchorClicked(const QUrl &)), this, SLOT(onAnchorClicked(const QUrl &)));

	createContextMenu();

	participantAvatars.clear();
	hasData = false;
	messageTime = false;
	messageDate = false;
	allowLinks = false;
	pathToLink = false;
	trimMessage = true;
	fontSizeVal = 0;
	sendFileMap.clear();
	receiveFileMap.clear();
	lastId = QString();
	messageLog.clear();
	linkHovered = false;
	outStyle = false;
	autoScroll = true;
}

lmMessageLog::~lmMessageLog() {
}

void lmMessageLog::initMessageLog(bool clearLog) {
    if(clearLog){
		messageLog.clear();
    }
	lastId = QString();
    clear();
}

void lmMessageLog::createContextMenu(void) {
	contextMenu = new QMenu(this);
	copyAction = new QAction("&Copy", this);
    copyAction->setShortcut(QKeySequence::Copy);
	connect(copyAction, &QAction::triggered, this, &lmMessageLog::copyAction_triggered);
    contextMenu->addAction(copyAction);
	copyLinkAction = contextMenu->addAction("&Copy Link", this, SLOT(copyLinkAction_triggered()));
	contextMenu->addSeparator();
	selectAllAction = new QAction("Select &All", this);
    connect(selectAllAction, &QAction::triggered, this, &lmMessageLog::selectAllAction_triggered);
    contextMenu->addAction(selectAllAction);
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	setContextMenuPolicy(Qt::CustomContextMenu);
}

void lmMessageLog::appendMessageLog(MessageType type, QString* lpszUserId, QString* lpszUserName, MessageXml* pMessage,
		bool bReload) {

	if(!pMessage && type != MT_Error)
		return;

	QString message;
	QString html;
	QString caption;
	QDateTime time;
	QFont font;
	QString fontStyle;
	QString id = QString();
	bool addToLog = true;

    removeMessageLog(MT_ChatState);

	switch(type) {
	case MT_Message:
		time.setMSecsSinceEpoch(pMessage->header(XN_TIME).toLongLong());
		message = pMessage->data(XN_MESSAGE);
		font.fromString(pMessage->data(XN_FONT));
        appendMessage(lpszUserId, lpszUserName, &message, &time, &font);
		lastId = *lpszUserId;
		break;
	case MT_PublicMessage:
	case MT_GroupMessage:
		time.setMSecsSinceEpoch(pMessage->header(XN_TIME).toLongLong());
		message = pMessage->data(XN_MESSAGE);
		font.fromString(pMessage->data(XN_FONT));
        appendPublicMessage(lpszUserId, lpszUserName, &message, &time, &font, type);
		lastId = *lpszUserId;
		break;
	case MT_Broadcast:
		time.setMSecsSinceEpoch(pMessage->header(XN_TIME).toLongLong());
		message = pMessage->data(XN_BROADCAST);
		appendBroadcast(lpszUserId, lpszUserName, &message, &time);
		lastId  = QString();
		break;
	case MT_ChatState:
		message = pMessage->data(XN_CHATSTATE);
		caption = getChatStateMessage((ChatState)Helper::indexOf(ChatStateNames, CS_Max, message));
        if(!caption.isNull()) {
            html = templates.stateMsg;
            html.replace("%icon%", "" );  //Blank
			html.replace("%sender%", caption.arg(*lpszUserName));
			html.replace("%message%", "");
            appendMessageLog(&html, type);
		}
		addToLog = false;
		break;
	case MT_Failed:
		message = pMessage->data(XN_MESSAGE);
		font.fromString(pMessage->data(XN_FONT));
        html = templates.sysMsg;
		caption = tr("This message was not delivered to %1:");
        fontStyle = getFontStyle(&font, true);
		decodeMessage(&message);
        html.replace("%icon%", "<span style='font-size:32px;'>"+Icons::Alert+"</span>");
		html.replace("%sender%", caption.arg(*lpszUserName));
		html.replace("%style%", fontStyle);
		html.replace("%message%", message);
        appendMessageLog(&html, type);
		lastId  = QString();
		break;
	case MT_Error:
        html = templates.sysMsg;
        html.replace("%icon%", "<span style='font-size:32px;'>"+Icons::Alert+"</span>");
		html.replace("%sender%", tr("Your message was not sent."));
		html.replace("%message%", "");
        appendMessageLog(&html, type);
		lastId  = QString();
		addToLog = false;
		break;
	case MT_File:
    case MT_Folder:
        id = getFileTempId(pMessage);
        html = getFileMessageText(type, lpszUserName, pMessage, bReload);
		lastId = QString();
        appendMessageLog(&html, MT_File, new QTextBlockData(id));
		break;
	case MT_Join:
	case MT_Leave:
		message = pMessage->data(XN_GROUPMSGOP);
		caption = getChatRoomMessage((GroupMsgOp)Helper::indexOf(GroupMsgOpNames, GMO_Max, message));
		if(!caption.isNull()) {
            html = templates.sysMsg;
            html.replace("%icon%", "" );  //Blank
			html.replace("%sender%", caption.arg(*lpszUserName));
			html.replace("%message%", "");
            appendMessageLog(&html, type);
		}
		lastId = QString();
        break;
    case MT_Audio:
    case MT_Video:
        id = getStreamTempId(pMessage);
        html = getStreamMessageText(type, lpszUserName, pMessage, bReload);
        lastId = QString();
        appendMessageLog(&html, type, new QTextBlockData(id));
        break;
	default:
		break;
	}

	if(!bReload && addToLog && pMessage) {
		MessageXml xmlMessage = pMessage->clone();
		QString userId = lpszUserId ? *lpszUserId : QString();
		QString userName = lpszUserName ? *lpszUserName : QString();
		messageLog.append(SingleMessage(type, userId, userName, xmlMessage, id));
    }
}

void lmMessageLog::updateStreamMessage(StreamMode mode, StreamOp op, QString streamId)
{
    QString tempId = getStreamTempId(mode, streamId);

    //	update the entry in message log
    for(int index = 0; index < messageLog.count(); index++) {
        SingleMessage msg = messageLog.at(index);
        if(tempId.compare(msg.id) == 0) {
            MessageXml xmlMessage = msg.message;
            xmlMessage.removeData(XN_STREAMOP);
            xmlMessage.addData(XN_STREAMOP, StreamOpNames[op]);
            msg.message = xmlMessage;

            QString html = getStreamMessageText(msg.type, &msg.userName, &msg.message);
            replaceMessageLog(msg.type, tempId, html);
            break;
        }
    }
}

void lmMessageLog::updateFileMessage(FileMode mode, FileOp op, QString fileId)
{
    QString tempId = getFileTempId(mode, fileId);

	//	update the entry in message log
	for(int index = 0; index < messageLog.count(); index++) {
		SingleMessage msg = messageLog.at(index);
		if(tempId.compare(msg.id) == 0) {
			MessageXml xmlMessage = msg.message;
			xmlMessage.removeData(XN_FILEOP);
			xmlMessage.addData(XN_FILEOP, FileOpNames[op]);
			msg.message = xmlMessage;

            QString html = getFileMessageText(msg.type, &msg.userName, &msg.message);
            replaceMessageLog(MT_File, tempId, html);
            break;
		}
	}
}

void lmMessageLog::updateUserName(QString* lpszUserId, QString* lpszUserName) {
	//	update the entries in message log
	for(int index = 0; index < messageLog.count(); index++) {
		SingleMessage msg = messageLog.takeAt(index);
		if(lpszUserId->compare(msg.userId) == 0)
			msg.userName = *lpszUserName;
		messageLog.insert(index, msg);
	}

	reloadMessageLog();
}

void lmMessageLog::updateAvatar(QString* lpszUserId, QString* lpszFilePath) {
	participantAvatars.insert(*lpszUserId, *lpszFilePath);

	reloadMessageLog();
}

void lmMessageLog::reloadMessageLog(void) {
    initMessageLog(false);
	for(int index = 0; index < messageLog.count(); index++) {
		SingleMessage msg = messageLog[index];
		appendMessageLog(msg.type, &msg.userId, &msg.userName, &msg.message, true);
	}
}

QString lmMessageLog::prepareMessageLogForSave(OutputFormat format) {
	QDateTime time;

	if(format == HtmlFormat) {
		QString html =
			"<html><head><style type='text/css'>"\
			"*{font-size: 9pt;} body {-webkit-nbsp-mode: space; word-wrap: break-word;}"\
			"span.salutation {float:left; font-weight: bold;} span.time {float: right;}"\
			"span.message {clear: both; display: block;} p {border-bottom: 1px solid #CCC;}"\
			"</style></head><body>";

		for(int index = 0; index < messageLog.count(); index++) {
			SingleMessage msg = messageLog.at(index);
			if(msg.type == MT_Message || msg.type == MT_GroupMessage) {
				time.setMSecsSinceEpoch(msg.message.header(XN_TIME).toLongLong());
				QString messageText = msg.message.data(XN_MESSAGE);
				decodeMessage(&messageText, true);
				QString htmlMsg =
					"<p><span class='salutation'>" + msg.userName + ":</span>"\
					"<span class='time'>" + QLocale().toString(time.time(), QLocale::ShortFormat) + "</span>"\
					"<span class='message'>" + messageText + "</span></p>";
				html.append(htmlMsg);
			}
		}

		html.append("</body></html>");
		return html;
	} else {
		QString text;
		for(int index = 0; index < messageLog.count(); index++) {
			SingleMessage msg = messageLog.at(index);
			if(msg.type == MT_Message || msg.type == MT_GroupMessage) {
				time.setMSecsSinceEpoch(msg.message.header(XN_TIME).toLongLong());
				QString textMsg =
					msg.userName + " [" + QLocale().toString(time.time(), QLocale::ShortFormat) + "]:\n" +
					msg.message.data(XN_MESSAGE) + "\n\n";
				text.append(textMsg);
			}
		}

		return text;
	}
}

void lmMessageLog::setAutoScroll(bool enable) {
	autoScroll = enable;
}

void lmMessageLog::abortPendingFileOperations(void) {
    QMap<QString, MessageXml>::iterator sIndex = sendFileMap.begin();
    while(sIndex != sendFileMap.end()) {
        MessageXml fileData = sIndex.value();
        FileOp fileOp = (FileOp)Helper::indexOf(FileOpNames, FO_Max, fileData.data(XN_FILEOP));
        if(fileOp == FO_Request) {
            updateFileMessage(FM_Send, FO_Abort, fileData.data(XN_FILEID));
            sIndex.value().removeData(XN_FILEOP);
            sIndex.value().addData(XN_FILEOP, FileOpNames[FO_Abort]);
        }
        sIndex++;
    }
    QMap<QString, MessageXml>::iterator rIndex = receiveFileMap.begin();
    while(rIndex != receiveFileMap.end()) {
        MessageXml fileData = rIndex.value();
        FileOp fileOp = (FileOp)Helper::indexOf(FileOpNames, FO_Max, fileData.data(XN_FILEOP));
        if(fileOp == FO_Request) {
            updateFileMessage(FM_Receive, FO_Abort, fileData.data(XN_FILEID));
            rIndex.value().removeData(XN_FILEOP);
            rIndex.value().addData(XN_FILEOP, FileOpNames[FO_Abort]);
        }
        rIndex++;
    }
}

void lmMessageLog::saveMessageLog(QString filePath) {
    if(messageLog.isEmpty())
        return;

    QDir dir = QFileInfo(filePath).dir();
    if(!dir.exists())
        dir.mkpath(dir.absolutePath());

    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly))
        return;

    QDataStream stream(&file);
    stream << peerId << peerName << messageLog;

    file.close();
}

void lmMessageLog::restoreMessageLog(QString filePath, bool reload) {
    messageLog.clear();

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return;

    QDataStream stream(&file);
    stream >> peerId >> peerName >> messageLog;

    file.close();

    if(reload)
        reloadMessageLog();
}

void lmMessageLog::changeEvent(QEvent* event) {
	switch(event->type()) {
	case QEvent::LanguageChange:
		setUIText();
		break;
	default:
		break;
	}

    QWidget::changeEvent(event);
}

void lmMessageLog::resizeEvent(QResizeEvent *event)
{
    MessageBrowser::resizeEvent(event);

    if(autoScroll) {
        QTextCursor cursor = textCursor();
        scrollToEnd(cursor);
    }
}

void lmMessageLog::onAnchorClicked(const QUrl &url)
{
    QString linkPath = url.toString();

    //	this is a hack so qdesktopservices can open a network path
    if(linkPath.startsWith("file")) {
        linkPath = linkPath.mid(5);
        if(linkPath.startsWith("//")) {   // proper file:// URL, open directly
            QDesktopServices::openUrl(QUrl(linkPath));
        } else {                          // UNC network path hack
            QDesktopServices::openUrl(QUrl(QDir::toNativeSeparators(linkPath)));
        }
        return;
    } else if(linkPath.startsWith("www")) {
        // prepend 'http://' to link
        linkPath.prepend("http://");
        QDesktopServices::openUrl(QUrl(linkPath));
        return;
    } else if(!linkPath.startsWith("lm")) {
        QDesktopServices::openUrl(url);
        return;
    }

    QStringList linkData = linkPath.split("/", Qt::SkipEmptyParts);
    FileMode mode;
    FileOp op;

    if(linkData[2].compare(acceptOp) == 0) {
        mode = FM_Receive;
        op = FO_Accept;
    } else if(linkData[2].compare(declineOp) == 0) {
        mode = FM_Receive;
        op = FO_Decline;
    } else if(linkData[2].compare(cancelOp) == 0) {
        mode = FM_Send;
        op = FO_Cancel;
    } else	// unknown link command
        return;

    //	Remove the link and show a confirmation message.
    updateFileMessage(mode, op, linkData[3]);

    fileOperation(linkData[3], linkData[2], linkData[1], mode);
}

void lmMessageLog::log_linkHovered(const QString& link, const QString& title, const QString& textContent) {
	Q_UNUSED(title);
	Q_UNUSED(textContent);
	linkHovered = !link.isEmpty();
}

void lmMessageLog::showContextMenu(const QPoint& pos) {
    QTextCursor cursor = textCursor();
    copyAction->setEnabled(cursor.selectionStart() != cursor.selectionEnd());
	copyLinkAction->setEnabled(linkHovered);
	//	Copy Link is currently hidden since it performs the same action as regular Copy
    copyLinkAction->setVisible(false);
    selectAllAction->setEnabled(!document()->isEmpty());
	contextMenu->exec(mapToGlobal(pos));
}

void lmMessageLog::copyAction_triggered(void) {
    copy();
}

void lmMessageLog::copyLinkAction_triggered(void) {
//  TODO long-ass-fucking-time-ago
//	pageAction(QWebPage::CopyLinkToClipboard)->trigger();
}

void lmMessageLog::selectAllAction_triggered(void) {
    selectAll();
}

void lmMessageLog::scrollToEnd(QTextCursor &cursor)
{
    cursor.movePosition(QTextCursor::MoveOperation::End);
    setTextCursor(cursor);
    ensureCursorVisible();

    QScrollBar *scrollBar = verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void lmMessageLog::appendMessageLog(QString *lpszHtml, MessageType type, QTextBlockData *data) {

    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::MoveOperation::End);

    insertMessageLog(cursor, *lpszHtml, type, data);

    if(autoScroll)
        scrollToEnd(cursor);
}

void lmMessageLog::removeMessageLog(MessageType type) {

    replaceMessageLog(type, QString(), QString());
}

void lmMessageLog::replaceMessageLog(MessageType type, QString id, QString html)
{
    auto frames = document()->rootFrame()->childFrames();

    for(int i=frames.size()-1; i>=0; i--) {

        QTextFrame *frame = frames.at(i);
        QTextCursor cursor = frame->firstCursorPosition();

        cursor.movePosition(QTextCursor::MoveOperation::StartOfBlock);

        if(isSameBlock(cursor, type, id)) {
            if(!html.isEmpty()) {
                QTextCursor insertCursor = textCursor();
                insertCursor.setPosition(frame->lastPosition() + 1);
                insertMessageLog(insertCursor, html, type, new QTextBlockData(id));
            }

            QTextCursor deleteCursor = textCursor();
            deleteCursor.setPosition(frame->firstPosition());
            deleteCursor.setPosition(frame->lastPosition() + 1, QTextCursor::MoveMode::KeepAnchor);
            deleteCursor.removeSelectedText();
            break;
        }
    }
}

void lmMessageLog::insertMessageLog(QTextCursor cursor, QString &html, MessageType type, QTextBlockData *data)
{
    QTextFrameFormat frameFormat;
    frameFormat.setMargin(0);
    frameFormat.setTopMargin(-12);
    frameFormat.setPadding(0);
    frameFormat.setBorder(0);
    QTextFrame *frame = cursor.insertFrame(frameFormat);
    frame->frameFormat().setMargin(0);
    frame->firstCursorPosition().insertHtml(html);

    QTextBlock block = frame->firstCursorPosition().block();

    block.setUserState(type);
    if(data != nullptr)
        block.setUserData(data);
}

bool lmMessageLog::isSameBlock(QTextCursor &cursor, MessageType type, QString &id) const
{
    bool result = false;

    QTextBlock block = cursor.block();

    if(block.userState() == type) {
        if(id.isNull()) {
            result = true;
        }
        else {
            QTextBlockUserData *data = block.userData();
            if(data != nullptr) {
                if(static_cast<QTextBlockData*>(data)->id == id)
                    result = true;
            }
        }
    }

    return result;
}

void lmMessageLog::appendBroadcast(QString* lpszUserId, QString* lpszUserName, QString* lpszMessage, QDateTime* pTime) {
	Q_UNUSED(lpszUserId);

	decodeMessage(lpszMessage);

    QString html = templates.pubMsg;
	QString caption = tr("Broadcast message from %1:");
    html.replace("%icon%", "<span style='font-size:32px;'>"+Icons::Broadcast+"</span>");
	html.replace("%sender%", caption.arg(*lpszUserName));

    QString timeStr = getTimeString(pTime);
    QStringList parts = timeStr.split('|');
    html.replace("%date%", parts.value(0));
    html.replace("%time%", parts.value(1));

	html.replace("%style%", "");
	html.replace("%message%", *lpszMessage);

    appendMessageLog(&html, MT_Broadcast);
}

void lmMessageLog::appendMessage(QString* lpszUserId, QString* lpszUserName, QString* lpszMessage, QDateTime* pTime,
                                  QFont* pFont) {
	QString html = QString();
	bool localUser = (lpszUserId->compare(localId) == 0);

	decodeMessage(lpszMessage);

    QString fontStyle = getFontStyle(pFont, localUser);

	if(lpszUserId->compare(lastId) != 0) {
        html = localUser ? templates.inMsg : templates.inMsg;

		//	get the avatar image for this user from the cache folder
		QString filePath = participantAvatars.value(*lpszUserId);
		//	if image not found, use the default avatar image for this user
        //QString iconPath = QFile::exists(filePath) ? QUrl::fromLocalFile(filePath).toString() : "qrc" AVT_DEFAULT;
        QString icon;
        if (QFile::exists(filePath)) {
            icon = QUrl::fromLocalFile(filePath).toString();
            icon = "<img border='0' width='48' height='48' src='"+ icon +"'>";
        } else {
            icon = QString("<span style='font-size:32px;'>"+avtEmoji[20]+"</span>"); //Hidden Default Avatar
        }

        html.replace("%icon%", icon);
		html.replace("%sender%", *lpszUserName);

        QString timeStr = getTimeString(pTime);
        QStringList parts = timeStr.split('|');
        html.replace("%date%", parts.value(0));
        html.replace("%time%", parts.value(1));

		html.replace("%style%", fontStyle);
		html.replace("%message%", *lpszMessage);

    } else {
        html = localUser ? templates.inNextMsg : templates.inNextMsg;

        QString timeStr = getTimeString(pTime);
        QStringList parts = timeStr.split('|');
        html.replace("%date%", parts.value(0));
        html.replace("%time%", parts.value(1));

		html.replace("%style%", fontStyle);
		html.replace("%message%", *lpszMessage);

    }

    appendMessageLog(&html, MT_Message);

	hasData = true;
}

void lmMessageLog::appendPublicMessage(QString* lpszUserId, QString* lpszUserName, QString* lpszMessage,
                                        QDateTime *pTime, QFont *pFont, MessageType messageType) {
	QString html = QString();
	bool localUser = (lpszUserId->compare(localId) == 0);

	decodeMessage(lpszMessage);

    QString fontStyle = getFontStyle(pFont, localUser);

	if(lpszUserId->compare(lastId) != 0) {
		outStyle = !outStyle;
        html = outStyle ? templates.inMsg : templates.inMsg;

        //	get the avatar image for this user from the cache folder
        QString filePath = participantAvatars.value(*lpszUserId);
        //	if image not found, use the default avatar image for this user
        //QString iconPath = QFile::exists(filePath) ? QUrl::fromLocalFile(filePath).toString() : "qrc" AVT_DEFAULT;
        QString icon;
        if (QFile::exists(filePath)) {
            icon = QUrl::fromLocalFile(filePath).toString();
            icon = "<img border='0' width='48' height='48' src='"+ icon +"'>";
        } else {
            icon = QString("<span style='font-size:32px;'>"+avtEmoji[20]+"</span>"); //Hidden Default Avatar
        }

        html.replace("%icon%", icon);
		html.replace("%sender%", *lpszUserName);

        QString timeStr = getTimeString(pTime);
        QStringList parts = timeStr.split('|');
        html.replace("%date%", parts.value(0));
        html.replace("%time%", parts.value(1));

		html.replace("%style%", fontStyle);
		html.replace("%message%", *lpszMessage);

    } else {
        html = outStyle ? templates.inNextMsg : templates.inNextMsg;

        QString timeStr = getTimeString(pTime);
        QStringList parts = timeStr.split('|');
        html.replace("%date%", parts.value(0));
        html.replace("%time%", parts.value(1));

		html.replace("%style%", fontStyle);
		html.replace("%message%", *lpszMessage);

    }

    appendMessageLog(&html, messageType);

	hasData = true;
}

// This function is called to display a audio/video request message in chatlog
QString lmMessageLog::getStreamMessageText(MessageType type, QString* lpszUserName, MessageXml* pMessage, bool bReload)
{
    QString html;
    QString caption;
    QString streamId = pMessage->data(XN_STREAMID);
    QString szStatus;
    QString streamType;
    html = templates.reqMsg;

    switch(type) {
    case MT_Audio:
        streamType = "audio";
        html.replace("%icon%", "<span style='font-size:32px;'>"+Icons::Telephone+"</span>");
        break;
    case MT_Video:
        streamType = "video";
        html.replace("%icon%", "<span style='font-size:32px;'>"+Icons::Camera+"</span>");
        break;
    default:
        throw std::logic_error("lmMessageLog::getStreamMessageText: not yet implemented");
        break;
    }

    //This type of message doesnt come with a timestamp, but I want one anyway.
    if (messageDate) {
        html.replace("%date%", QDate::currentDate().toString("d MMMM yyyy"));
        html.replace("%time%", QTime::currentTime().toString("hh:mm"));
    } else {
        html.replace("%date%", "");
        html.replace("%time%", QTime::currentTime().toString("hh:mm"));
    }

    StreamMode streamMode = (StreamMode)Helper::indexOf(StreamModeNames, SM_Max, pMessage->data(XN_STREAMMODE));
    StreamOp streamOp = (StreamOp)Helper::indexOf(StreamOpNames, SO_Max, pMessage->data(XN_STREAMOP));
    //"","request","accept","decline","error","abort",

        if(streamMode == SM_Out) {

        //"Requesting audio/video call from user"
        caption = tr("Requesting %1 call from %2.");
        html.replace("%sender%", caption.arg(streamType, *lpszUserName));
        html.replace("%message%", "");

        switch(streamOp) {
        case SO_Request:
            html.replace("%links%", "<a href='lm://" + streamType + "/" + cancelOp + "/" + streamId + "'>" + tr("Cancel") + "</a>");
            break;
        case SO_Accept:
            html.replace("%links%", tr("Call accepted."));
            break;
        case SO_Decline:
            html.replace("%links%", tr("Call declined."));
            break;
        case SO_Error:
            html.replace("%links%", tr("Call error."));
            break;
        case SO_Abort:
            html.replace("%links%", tr("Call aborted."));
            break;
        default:
            html = QString();
        }

    }else{
            //"User is requesting audio/video call"
            caption = tr("%1 is requesting %2 call.");
            html.replace("%sender%", caption.arg(*lpszUserName, streamType));
            html.replace("%message%", "");

            switch(streamOp) {
            case SO_Request:
                html.replace("%links%",
                             "<a href='lm://" + streamType + "/" + acceptOp + "/" + streamId + "'>" + tr("Accept") + "</a>&nbsp;&nbsp;" +
                                 "<a href='lm://" + streamType + "/" + declineOp + "/" + streamId + "'>" + tr("Decline") + "</a>");
                break;
            case SO_Accept:
                html.replace("%links%", tr("Call accepted."));
                break;
            case SO_Decline:
                html.replace("%links%", tr("Call declined."));
                break;
            case SO_Error:
                html.replace("%links%", tr("Call error."));
                break;
            case SO_Abort:
                html.replace("%links%", tr("Call aborted."));
                break;
            default:
                html = QString();
            }
    }
    return html;
}

// This function is called to display a file request message in chatlog
QString lmMessageLog::getFileMessageText(MessageType type, QString* lpszUserName, MessageXml* pMessage, bool bReload)
{
    QString html;
	QString caption;
    QString fileId = pMessage->data(XN_FILEID);
	QString szStatus;
    QString fileType;

    switch(type) {
    case MT_File:
        fileType = "file";
        break;
    case MT_Folder:
        fileType = "folder";
        break;
    default:
        throw std::logic_error("lmMessageLog::getFileMessageText: not yet implemented");
    }

    html = templates.reqMsg;
    html.replace("%icon%", "<span style='font-size:32px;'>"+Icons::File+"</span>");

    //This type of message doesnt come with a timestamp, but I want one anyway.
    if (messageDate) {
        html.replace("%date%", QDate::currentDate().toString("d MMMM yyyy"));
        html.replace("%time%", QTime::currentTime().toString("hh:mm"));
    } else {
        html.replace("%date%", "");
        html.replace("%time%", QTime::currentTime().toString("hh:mm"));
    }

	FileOp fileOp = (FileOp)Helper::indexOf(FileOpNames, FO_Max, pMessage->data(XN_FILEOP));
    FileMode fileMode = (FileMode)Helper::indexOf(FileModeNames, FM_Max, pMessage->data(XN_MODE));

    if(fileMode == FM_Send) {
		caption = tr("Sending '%1' to %2.");
        html.replace("%sender%", caption.arg(pMessage->data(XN_FILENAME), *lpszUserName));
        html.replace("%message%", "");

		switch(fileOp) {
		case FO_Request:
			sendFileMap.insert(fileId, *pMessage);
            html.replace("%links%", "<a href='lm://" + fileType + "/" + cancelOp + "/" + fileId + "'>" + tr("Cancel") + "</a>");
			break;
		case FO_Cancel:
		case FO_Accept:
		case FO_Decline:
        case FO_Error:
        case FO_Abort:
        case FO_Complete:
			szStatus = getFileStatusMessage(FM_Send, fileOp);
            html.replace("%links%", szStatus);
			break;
		default:
            html = QString();
		}
    } else {
		if(autoFile) {
            if(type == MT_File)
                caption = tr("%1 is sending you a file:");
            else
                caption = tr("%1 is sending you a folder:");
            html.replace("%sender%", caption.arg(*lpszUserName));
            html.replace("%message%", pMessage->data(XN_FILENAME) + " (" +
				Helper::formatSize(pMessage->data(XN_FILESIZE).toLongLong()) + ")");
            html.replace("%fileid%", "");
		} else {
            if(type == MT_File)
                caption = tr("%1 sends you a file:");
            else
                caption = tr("%1 sends you a folder:");
            html.replace("%sender%", caption.arg(*lpszUserName));
            html.replace("%message%", pMessage->data(XN_FILENAME) + " (" +
				Helper::formatSize(pMessage->data(XN_FILESIZE).toLongLong()) + ")");
		}

		switch(fileOp) {
		case FO_Request:
			receiveFileMap.insert(fileId, *pMessage);

			if(autoFile) {
                html.replace("%links%", tr("Accepted"));
                if(!bReload)
                    fileOperation(fileId, acceptOp, fileType);
			} else {
                html.replace("%links%",
                    "<a href='lm://" + fileType + "/" + acceptOp + "/" + fileId + "'>" + tr("Accept") + "</a>&nbsp;&nbsp;" +
                    "<a href='lm://" + fileType + "/" + declineOp + "/" + fileId + "'>" + tr("Decline") + "</a>");
			}
			break;
		case FO_Cancel:
		case FO_Accept:
		case FO_Decline:
        case FO_Error:
        case FO_Abort:
            szStatus = getFileStatusMessage(FM_Receive, fileOp);
            html.replace("%links%", szStatus);
            break;
        case FO_Complete:
        {
            QString fileName = pMessage->data(XN_FILENAME);
            QString filePath = QDir(DefinitionsDir::fileStorageDir()).absoluteFilePath(fileName);
            QString folderPath = DefinitionsDir::fileStorageDir();
            html.replace("%links%",
                         tr("Completed.") + "&nbsp;&nbsp;" +
                             "<a href='" + QUrl::fromLocalFile(filePath).toString() + "'>" + tr("Open") + "</a>&nbsp;&nbsp;" +
                             "<a href='" + QUrl::fromLocalFile(folderPath).toString() + "'>" + tr("Show in Folder") + "</a>");
            break;
        }
		default:
            html = QString();
        }
	}

    return html;
}

QString lmMessageLog::getFontStyle(QFont* pFont, bool size) {
	QString style = "font-family:\"" + pFont->family() + "\"; ";
	if(pFont->italic())
		style.append("font-style:italic; ");
	if(pFont->bold())
		style.append("font-weight:bold; ");

	if(size) {
		style.append("font-size:" + QString::number(pFont->pointSize()) + "pt; ");
	}
	else
		style.append(fontStyle[fontSizeVal] + " ");

	if(pFont->strikeOut())
		style.append("text-decoration:line-through; ");
	if(pFont->underline())
		style.append("text-decoration:underline; ");

	return style;
}

QString lmMessageLog::getFileStatusMessage(FileMode mode, FileOp op) {
	QString message;

	switch(op) {
	case FO_Accept:
        message = (mode == FM_Send) ? tr("Accepted") : tr("Accepted");
		break;
	case FO_Decline:
        message = (mode == FM_Send) ? tr("Declined") : tr("Declined");
		break;
	case FO_Cancel:
		message = (mode == FM_Send) ? tr("Canceled") : tr("Canceled");
		break;
    case FO_Error:
    case FO_Abort:
        message = (mode == FM_Send) ? tr("Interrupted") : tr("Interrupted");
        break;
    case FO_Complete:
        message = (mode == FM_Send) ? tr("Completed") : tr("Completed");
        break;
	default:
		break;
	}

	return message;
}

QString lmMessageLog::getChatStateMessage(ChatState chatState) {
	QString message = QString();

	switch(chatState) {
	case CS_Composing:
		message = tr("%1 is typing...");
		break;
	case CS_Paused:
		message = tr("%1 has entered text");
		break;
	default:
		break;
	}

	return message;
}

QString lmMessageLog::getChatRoomMessage(GroupMsgOp op) {
	QString message = QString();

	switch(op) {
	case GMO_Join:
		message = tr("%1 has joined this conversation");
		break;
	case GMO_Leave:
		message = tr("%1 has left this conversation");
		break;
	default:
		break;
	}

	return message;
}

void lmMessageLog::fileOperation(QString fileId, QString action, QString fileType, FileMode mode) {
    MessageXml fileData, xmlMessage;

    MessageType type;
    if(fileType.compare("file") == 0)
        type = MT_File;
    else if(fileType.compare("folder") == 0)
        type = MT_Folder;
    else
        return;

    if(action.compare(acceptOp) == 0) {
        fileData = receiveFileMap.value(fileId);
		xmlMessage.addData(XN_MODE, FileModeNames[FM_Receive]);
		xmlMessage.addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
		xmlMessage.addData(XN_FILEOP, FileOpNames[FO_Accept]);
        xmlMessage.addData(XN_FILEID, fileData.data(XN_FILEID));
		xmlMessage.addData(XN_FILEPATH, fileData.data(XN_FILEPATH));
		xmlMessage.addData(XN_FILENAME, fileData.data(XN_FILENAME));
		xmlMessage.addData(XN_FILESIZE, fileData.data(XN_FILESIZE));
	}
    else if(action.compare(declineOp) == 0) {
        fileData = receiveFileMap.value(fileId);
		xmlMessage.addData(XN_MODE, FileModeNames[FM_Receive]);
		xmlMessage.addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
		xmlMessage.addData(XN_FILEOP, FileOpNames[FO_Decline]);
        xmlMessage.addData(XN_FILEID, fileData.data(XN_FILEID));
	}
    else if(action.compare(cancelOp) == 0) {
        if(mode == FM_Receive)
            fileData = receiveFileMap.value(fileId);
        else
            fileData = sendFileMap.value(fileId);
        xmlMessage.addData(XN_MODE, FileModeNames[mode]);
        xmlMessage.addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
        xmlMessage.addData(XN_FILEOP, FileOpNames[FO_Cancel]);
        xmlMessage.addData(XN_FILEID, fileData.data(XN_FILEID));
	}

    emit messageSent(type, &peerId, &xmlMessage);
}

//	Called when message received, before adding to message log
//	The useDefaults parameter is an override flag that will ignore app settings
//	while decoding the message. If the flag is set, message is not trimmed,
//	smileys are left as text and links will be detected and converted.
void lmMessageLog::decodeMessage(QString* lpszMessage, bool useDefaults) {
	if(!useDefaults && trimMessage)
		*lpszMessage = lpszMessage->trimmed();

	//	The url detection regexps only work with plain text, so link detection is done before
	//	making the text html safe. The converted links are given a "data-isLink" custom
	//	attribute to differentiate them from the message content
    if(useDefaults || allowLinks) {
        //		lpszMessage->replace(QRegularExpression("(((https|http|ftp|file|smb):[/][/]|www.)[\\w\\d:#@%/;$()~_?\\+-=\\\\\\.&]*)"),
        //							 "<a href='\\1'>\\1</a>");
        lpszMessage->replace(
            QRegularExpression("((?:(?:https?|ftp|file)://|www\\.|ftp\\.)[-A-Z0-9+&@#/%=~_|$?!:,.]*[A-Z0-9+&@#/%=~_|$])",
                               QRegularExpression::CaseInsensitiveOption),
            "<a data-isLink='true' href='\\1'>\\1</a>");
        lpszMessage->replace("<a data-isLink='true' href='www", "<a data-isLink='true' href='http://www");

        if(!useDefaults && pathToLink)
            lpszMessage->replace(QRegularExpression("((\\\\\\\\[\\w-]+\\\\[^\\\\/:*?<>|""]+)((?:\\\\[^\\\\/:*?<>|""]+)*\\\\?)$)"),
                                 "<a data-isLink='true' href='file:\\1'>\\1</a>");
    }

/*  NEED2TEST This part needs expansion. OH MY GOD WHAT A NIGHTMARE!
TEST CASES
00 \\Murticom-2026\e\ss.png   pathToLink   Shows up as link, just opens file explorer, which is incorrect. It needs to open the file.
01 //Murticom-2026/e/ss.png   pathToLink
   \\Murticom-2026\e/ss.png
   \\Murticom-2026\e\S p a c e.png
   \\Murticom-2026\e\Ünıcöde.png
   smb:// for linux.
   %WINDIR%
   %USERPROFILE%\Documents\%USERNAME%_log.txt
02 \\Murticom-2026\e\         pathToLink   Shows up as link, just opens file explorer, which is incorrect. It needs to open the correct directory.
03 //Murticom-2026/e/         pathToLink
04 \\Murticom-2026\e          pathToLink   Entire line disappears
05 //Murticom-2026/e          pathToLink
06 \\192.168.0.10\e           pathToLink   Shows up as plain text
07 //192.168.0.10/e           pathToLink
08 https://github.com         allowLinks   Works as expected
   https://google.com/search?q=qt+6
   (https://google.com)
09 www.github.com             allowLinks   Works as expected
10 github.com                 allowLinks   Shows up as plain text
   v3.beta.github.com
11 192.168.0.10               allowLinks   Shows up as plain text
12 192.168.0.10:1111          allowLinks   Shows up as plain text
13 ftp.debian.org             allowLinks   Works, but tries to open in file explorer instead of browser
14 file:///E:/ss.png          This should be caught in the chatbox, and turned into a file send operation. Dont handle it here.IGNORE
15 file:///E:/_qtprojects     This should be caught in the chatbox, and turned into a folder send operation. Dont handle it here.IGNORE
*/


	QString message = QString();
	int index = 0;

	while(index < lpszMessage->length()) {
		int aStart = lpszMessage->indexOf("<a data-isLink='true'", index);
		if(aStart != -1) {
			QString messageSegment = lpszMessage->mid(index, aStart - index);
			processMessageText(&messageSegment, useDefaults);
			message.append(messageSegment);
			index = lpszMessage->indexOf("</a>", aStart) + 4;
			QString linkSegment = lpszMessage->mid(aStart, index - aStart);
			message.append(linkSegment);
		} else {
			QString messageSegment = lpszMessage->mid(index);
			processMessageText(&messageSegment, useDefaults);
			message.append(messageSegment);
			break;
		}
	}

	message.replace("\n", "<br/>");

	*lpszMessage = message;
}

void lmMessageLog::processMessageText(QString* lpszMessageText, bool useDefaults) {
	ChatHelper::makeHtmlSafe(lpszMessageText);
	//	if smileys are enabled, replace text emoticons with corresponding images
	if(!useDefaults && showSmiley)
		ChatHelper::decodeSmileys(lpszMessageText);
}

QString lmMessageLog::getTimeString(QDateTime* pTime) {
    if (!pTime || !messageTime) return "";
    if (messageDate) {
        return pTime->toString("d MMMM yyyy|hh:mm");
    }
    return pTime->toString("|hh:mm");  //string will be split, and date placeholder will be replaced with nothing.
}

void lmMessageLog::setUIText(void) {
	copyAction->setText(tr("&Copy"));
	selectAllAction->setText(tr("Select &All"));
    reloadMessageLog();
}

QString lmMessageLog::getFileTempId(FileMode mode, QString fileId) const
{
    QString tempId = (mode == FM_Send) ? "send" : "receive";
    tempId.append(fileId);
    return tempId;
}

QString lmMessageLog::getFileTempId(MessageXml *pMessage) const
{
    QString fileId = pMessage->data(XN_FILEID);
    FileMode fileMode = (FileMode)Helper::indexOf(FileModeNames, FM_Max, pMessage->data(XN_MODE));
    return getFileTempId(fileMode, fileId);
}

QString lmMessageLog::getStreamTempId(StreamMode mode, QString streamId) const
{
    QString tempId = (mode == SM_Out) ? "outgoing" : "incoming";
    tempId.append(streamId);
    return tempId;
}

QString lmMessageLog::getStreamTempId(MessageXml *pMessage) const
{
    QString streamId = pMessage->data(XN_STREAMID);
    StreamMode streamMode = (StreamMode)Helper::indexOf(StreamModeNames, SM_Max, pMessage->data(XN_STREAMMODE));
    return getStreamTempId(streamMode, streamId);
}
