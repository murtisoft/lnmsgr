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


#include <QDesktopServices>
#include <QTimer>
#include <QUrl>
#include "aformmain.h"
#include "messagelog.h"
#include "history.h"
#include <cstdlib>
#include <qstylehints.h>

lmFormMain::lmFormMain(QWidget *parent, Qt::WindowFlags flags) : QWidget(parent, flags) {
	ui.setupUi(this);

	connect(ui.tvUserList, SIGNAL(itemActivated(QTreeWidgetItem*, int)), 
		this, SLOT(tvUserList_itemActivated(QTreeWidgetItem*, int)));
    connect(ui.tvUserList, SIGNAL(itemContextMenu(QTreeWidgetItem*, QPoint&)),
        this, SLOT(tvUserList_itemContextMenu(QTreeWidgetItem*, QPoint&)));
	connect(ui.tvUserList, SIGNAL(itemDragDropped(QTreeWidgetItem*)),
		this, SLOT(tvUserList_itemDragDropped(QTreeWidgetItem*)));
	connect(ui.tvUserList, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
		this, SLOT(tvUserList_currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
	connect(ui.txtNote, SIGNAL(returnPressed()), this, SLOT(txtNote_returnPressed()));
	connect(ui.txtNote, SIGNAL(lostFocus()), this, SLOT(txtNote_lostFocus()));

    ui.txtNote->installEventFilter(this);
    ui.tvUserList->installEventFilter(this);

	windowLoaded = false;
}

lmFormMain::~lmFormMain(void) {
}

void lmFormMain::init(User* pLocalUser, QList<Group>* pGroupList, bool connected) {
	setWindowIcon(QIcon(IDR_APPICON));

	this->pLocalUser = pLocalUser;

	createMainMenu();
	createToolBar();
	createStatusMenu();
	createAvatarMenu();

	createTrayMenu();
	createTrayIcon();
	connectionStateChanged(connected);

	createGroupMenu();
	createUserMenu();

	ui.lblDividerTop->setBackgroundRole(QPalette::Highlight);
	ui.lblDividerTop->setAutoFillBackground(true);

	ui.tvUserList->setIconSize(QSize(16, 16));
    ui.tvUserList->header()->setSectionsMovable(false);
	ui.tvUserList->header()->setStretchLastSection(false);
    ui.tvUserList->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    btnStatus->setIconSize(QSize(20, 20));
	int index = Helper::statusIndexFromCode(pLocalUser->status);
	//	if status is not recognized, default to available
	index = qMax(index, 0);
	btnStatus->setIcon(QIcon(QPixmap(statusPic[index], "PNG")));
	statusGroup->actions()[index]->setChecked(true);
    QFont font = ui.lblUserName->font();
	int fontSize = ui.lblUserName->fontInfo().pixelSize();
	fontSize += (fontSize * 0.1);
	font.setPixelSize(fontSize);
    font.setBold(true);
    ui.lblUserName->setFont(font);
	ui.lblStatus->setText(statusGroup->checkedAction()->text());
	nAvatar = pLocalUser->avatar;
	ui.txtNote->setText(pLocalUser->note);

	pSoundPlayer = new lmSoundPlayer();
	pSettings = new lmSettings();
	restoreGeometry(pSettings->value(IDS_WINDOWMAIN).toByteArray());
	//	get saved settings
	settingsChanged(true);
	setUIText();

	initGroups(pGroupList);
}

void lmFormMain::moveEvent(QMoveEvent *event) {
    int snap = 10;
    QRect frame = frameGeometry();
    QPoint pos = frame.topLeft();

    // Get screen based on center of window
    QScreen *scr = QGuiApplication::screenAt(frame.center());
    if (!scr) return;

    QRect geom = scr->availableGeometry();

    // Snap Left/Top
    if (qAbs(pos.x() - geom.left()) < snap)  //+-1 adjustments are necessary on win11 for some reason.
        pos.setX(geom.left() + 1 );
    if (qAbs(pos.y() - geom.top()) < snap)
        pos.setY(geom.top() - 1 );
    if (qAbs(pos.x() + frame.width() - geom.right()) < snap)
        pos.setX(geom.right() - (frame.width() - 1) );
    if (qAbs(pos.y() + frame.height() - geom.bottom()) < snap)
        pos.setY(geom.bottom() - (frame.height() + 1) );

    // Update only if changed
    if (pos != frame.topLeft()) {
        this->move(pos);
    }
}

void lmFormMain::start(void) {
	//	if no avatar is set, select a random avatar (useful when running for the first time)
	if(nAvatar > AVT_COUNT) {
        std::srand(QTime::currentTime().msec() & 0xFFFF);
        nAvatar = std::rand() % AVT_COUNT;
	}
	// This method should only be called from here, otherwise an MT_Notify message is sent
	// and the program will connect to the network before start() is called.
	setAvatar();
	pTrayIcon->setVisible(showSysTray);

    int colorSchemeIndex = pSettings->value(IDS_COLORSCHEME, IDS_COLORSCHEME_VAL).toInt();  //NEED2TEST Kinda had to, but its messy, i dont like this.
    switch (colorSchemeIndex) {
    case 0:
        qApp->styleHints()->setColorScheme(Qt::ColorScheme::Unknown);
        break;
    case 1:
        qApp->styleHints()->setColorScheme(Qt::ColorScheme::Light);
        break;
    case 2:
        qApp->styleHints()->setColorScheme(Qt::ColorScheme::Dark);
        break;
    default:
        break;
    }
    QEvent event(QEvent::ThemeChange);
    QCoreApplication::sendEvent(qApp, &event);

	if(pSettings->value(IDS_AUTOSHOW, IDS_AUTOSHOW_VAL).toBool())
		show();
}

void lmFormMain::show(void) {
	windowLoaded = true;
	QWidget::show();
}

void lmFormMain::restore(void) {
	//	if window is minimized, restore it to previous state
	if(windowState().testFlag(Qt::WindowMinimized))
		setWindowState(windowState() & ~Qt::WindowMinimized);
	setWindowState(windowState() | Qt::WindowActive);
	raise();	// make main window the top most window of the application
	show();
	activateWindow();	// bring window to foreground
}

void lmFormMain::minimize(void) {
	// This function actually hides the window, basically the opposite of restore()
	hide();
	showMinimizeMessage();
}

void lmFormMain::stop(void) {
	//	These settings are saved only if the window was opened at least once by the user
	if(windowLoaded) {
		pSettings->setValue(IDS_WINDOWMAIN, saveGeometry());
		pSettings->setValue(IDS_MINIMIZEMSG, showMinimizeMsg, IDS_MINIMIZEMSG_VAL);
	}

	pSettings->beginWriteArray(IDS_GROUPEXPHDR);
	for(int index = 0; index < ui.tvUserList->topLevelItemCount(); index++) {
		pSettings->setArrayIndex(index);
		pSettings->setValue(IDS_GROUP, ui.tvUserList->topLevelItem(index)->isExpanded());
	}
	pSettings->endArray();

	pTrayIcon->hide();

	//	delete all temp files from cache
	QDir cacheDir(DefinitionsDir::cacheDir());
	if(!cacheDir.exists())
		return;
    QDir::Filters filters = QDir::Files | QDir::Readable;
    QDir::SortFlags sort = QDir::Name;
    //  save all cached conversations to history, then delete the files
    QString filter = "msg_*.tmp";
    lmMessageLog* pMessageLog = new lmMessageLog();
    QStringList fileNames = cacheDir.entryList(QStringList() << filter, filters, sort);
    foreach (QString fileName, fileNames) {
        QString filePath = cacheDir.absoluteFilePath(fileName);
        pMessageLog->restoreMessageLog(filePath, false);
        QString szMessageLog = pMessageLog->prepareMessageLogForSave();
        History::save(pMessageLog->peerName, QDateTime::currentDateTime(), &szMessageLog);
        QFile::remove(filePath);
    }
    pMessageLog->deleteLater();

    //  delete all other temp files
    filter = "*.tmp";
    fileNames = cacheDir.entryList(QStringList() << filter, filters, sort);
    foreach (QString fileName, fileNames) {
        QString filePath = cacheDir.absoluteFilePath(fileName);
        QFile::remove(filePath);
    }
}

void lmFormMain::addUser(User* pUser) {
	if(!pUser)
		return;

	int index = Helper::statusIndexFromCode(pUser->status);

	lmUserTreeWidgetUserItem *pItem = new lmUserTreeWidgetUserItem();
	pItem->setData(0, IdRole, pUser->id);
	pItem->setData(0, TypeRole, "User");
	pItem->setData(0, StatusRole, index);
	pItem->setData(0, SubtextRole, pUser->note);
    pItem->setData(0, CapsRole, pUser->caps);
	pItem->setText(0, pUser->name);
	if(statusToolTip)
		pItem->setToolTip(0, lmStrings::statusDesc()[index]);
	
	if(index != -1)
		pItem->setIcon(0, QIcon(QPixmap(statusPic[index], "PNG")));

	lmUserTreeWidgetGroupItem* pGroupItem = (lmUserTreeWidgetGroupItem*)getGroupItem(&pUser->group);
	pGroupItem->addChild(pItem);
	pGroupItem->sortChildren(0, Qt::AscendingOrder);

	// this should be called after item has been added to tree
    setUserAvatar(&pUser->id, &pUser->avatarPath);

	if(isHidden() || !isActiveWindow()) {
		QString msg = tr("%1 is online.");
        showTrayMessage(TM_Status, msg.arg(pItem->text(0)), QString(), TMI_Info, QPixmap(pUser->avatarPath));
		pSoundPlayer->play(SE_UserOnline);
	}

	sendAvatar(&pUser->id);
}

void lmFormMain::updateUser(User* pUser) {
	if(!pUser)
		return;

	QTreeWidgetItem* pItem = getUserItem(&pUser->id);
	if(pItem) {
		updateStatusImage(pItem, &pUser->status);
		int index = Helper::statusIndexFromCode(pUser->status);
		pItem->setData(0, StatusRole, index);
		pItem->setData(0, SubtextRole, pUser->note);
		pItem->setText(0, pUser->name);
		if(statusToolTip)
			pItem->setToolTip(0, lmStrings::statusDesc()[index]);
		QTreeWidgetItem* pGroupItem = pItem->parent();
		pGroupItem->sortChildren(0, Qt::AscendingOrder);
	}
}

void lmFormMain::removeUser(QString* lpszUserId) {
	QTreeWidgetItem* pItem = getUserItem(lpszUserId);
	if(!pItem)
		return;
		
	QTreeWidgetItem* pGroup = pItem->parent();
	pGroup->removeChild(pItem);

	if(isHidden() || !isActiveWindow()) {
		QString msg = tr("%1 is offline.");

        // Retrieve the QIcon from AvatarRole and convert to Pixmap
        QIcon avtIcon = pItem->data(0, AvatarRole).value<QIcon>();
        QPixmap avatar = avtIcon.pixmap(48, 48);

        showTrayMessage(TM_Status, msg.arg(pItem->text(0)), QString(), TMI_Info, avatar);
        pSoundPlayer->play(SE_UserOffline);
	}
}

void lmFormMain::receiveMessage(MessageType type, QString* lpszUserId, MessageXml* pMessage) {
	QString filePath;

	switch(type) {
	case MT_Avatar:
        filePath = pMessage->data(XN_FILEPATH);
        setUserAvatar(lpszUserId, &filePath);
		break;
	default:
		break;
	}
}

void lmFormMain::connectionStateChanged(bool connected) {
	if(connected)
		showTrayMessage(TM_Connection, tr("You are online."));
	else
		showTrayMessage(TM_Connection, tr("You are no longer connected."), lmStrings::appName(), TMI_Warning);

	bConnected = connected;
	setTrayTooltip();
}

void lmFormMain::settingsChanged(bool init) {
	showSysTray = pSettings->value(IDS_SYSTRAY, IDS_SYSTRAY_VAL).toBool();
	showSysTrayMsg = pSettings->value(IDS_SYSTRAYMSG, IDS_SYSTRAYMSG_VAL).toBool();
	//	this setting should be loaded only at window init
	if(init)
		showMinimizeMsg = pSettings->value(IDS_MINIMIZEMSG, IDS_MINIMIZEMSG_VAL).toBool();
	//	this operation should not be done when window inits
	if(!init)
		pTrayIcon->setVisible(showSysTray);
	minimizeHide = pSettings->value(IDS_MINIMIZETRAY, IDS_MINIMIZETRAY_VAL).toBool();
	singleClickActivation = pSettings->value(IDS_SINGLECLICKTRAY, IDS_SINGLECLICKTRAY_VAL).toBool();
	allowSysTrayMinimize = pSettings->value(IDS_ALLOWSYSTRAYMIN, IDS_ALLOWSYSTRAYMIN_VAL).toBool();
	showAlert = pSettings->value(IDS_ALERT, IDS_ALERT_VAL).toBool();
	noBusyAlert = pSettings->value(IDS_NOBUSYALERT, IDS_NOBUSYALERT_VAL).toBool();
	noDNDAlert = pSettings->value(IDS_NODNDALERT, IDS_NODNDALERT_VAL).toBool();
	statusToolTip = pSettings->value(IDS_STATUSTOOLTIP, IDS_STATUSTOOLTIP_VAL).toBool();
	int viewType = pSettings->value(IDS_USERLISTVIEW, IDS_USERLISTVIEW_VAL).toInt();
	ui.tvUserList->setView((UserListView)viewType);
	for(int index = 0; index < ui.tvUserList->topLevelItemCount(); index++) {
		QTreeWidgetItem* item = ui.tvUserList->topLevelItem(index);
		for(int childIndex = 0; childIndex < item->childCount(); childIndex++) {
			QTreeWidgetItem* childItem = item->child(childIndex);
//			QSize itemSize = ui.tvUserList->view() == ULV_Detailed ? QSize(0, 36) : QSize(0, 20);
//			childItem->setSizeHint(0, itemSize);

			QString toolTip = statusToolTip ? lmStrings::statusDesc()[childItem->data(0, StatusRole).toInt()] : QString();
			childItem->setToolTip(0, toolTip);
		}
	}
	pSoundPlayer->settingsChanged();
	ui.lblUserName->setText(pLocalUser->name);	// in case display name has been changed
}

void lmFormMain::showTrayMessage(TrayMessageType type, QString szMessage, QString szTitle, TrayMessageIcon icon, const QPixmap &avatar) {
	if(!showSysTray || !showSysTrayMsg)
		return;

	bool showMsg = showSysTray;
	
	switch(type) {
	case TM_Status:
		if(!showAlert || (pLocalUser->status == "Busy" && noBusyAlert) || (pLocalUser->status == "NoDisturb" && noDNDAlert))
			return;
		break;
    default:
        break;
	}

	if(szTitle.isNull())
		szTitle = lmStrings::appName();

	if(showMsg) {
		lastTrayMessageType = type;
        // Custom Toast Widget to bypass windows' garbage notification system.
        createToast(szTitle, szMessage, icon, avatar);
	}
}

void lmFormMain::createToast(const QString& title, const QString& msg, TrayMessageIcon icon, const QPixmap &avatar) {
    QWidget* toast = new QWidget(nullptr);
    toast->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    toast->setAttribute(Qt::WA_ShowWithoutActivating);
    toast->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout* mainLayout = new QVBoxLayout(toast);
    mainLayout->setContentsMargins(15, 10, 15, 15);

    // Header Row (App Icon + App Name)
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* lblIcon = new QLabel();
    lblIcon->setPixmap(this->windowIcon().pixmap(16, 16));

    QLabel* lblTitle = new QLabel(title);
    lblTitle->setStyleSheet("font-size: 11px;");

    headerLayout->addWidget(lblIcon);
    headerLayout->addWidget(lblTitle);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    // Body Row (Icon/Avatar + Message)
    QHBoxLayout* bodyLayout = new QHBoxLayout();
    QLabel* lblSideIcon = new QLabel();
    lblSideIcon->setFixedSize(48, 48);
    lblSideIcon->setScaledContents(true);

    if (!avatar.isNull()) {
        lblSideIcon->setPixmap(avatar);
    } else {
        QStyle::StandardPixmap sp;
        switch (icon) {
        case TMI_Warning:
            sp = QStyle::SP_MessageBoxWarning;
            break;
        case TMI_Error:
            sp = QStyle::SP_MessageBoxCritical;
            break;
        default:
            sp = QStyle::SP_MessageBoxInformation;
            break;
        }
        lblSideIcon->setPixmap(style()->standardIcon(sp).pixmap(48, 48));
    }

    QLabel* lblMsg = new QLabel(msg);
    lblMsg->setStyleSheet("font-size: 14px;");
    lblMsg->setWordWrap(true);

    bodyLayout->addWidget(lblSideIcon);
    bodyLayout->addSpacing(10);
    bodyLayout->addWidget(lblMsg);
    bodyLayout->addStretch();
    mainLayout->addLayout(bodyLayout);

    toast->setFixedWidth(320);
    toast->adjustSize();

    QRect geo = QGuiApplication::primaryScreen()->availableGeometry();
    toast->move(geo.right() - toast->width(), geo.bottom() - toast->height());

    toast->show();
    QTimer::singleShot(5000, toast, &QWidget::close);
}

QList<QTreeWidgetItem*> lmFormMain::getContactsList(void) {
	QList<QTreeWidgetItem*> contactsList;
	for(int index = 0; index < ui.tvUserList->topLevelItemCount(); index++)
		contactsList.append(ui.tvUserList->topLevelItem(index)->clone());

	return contactsList;
}

bool lmFormMain::eventFilter(QObject* pObject, QEvent* pEvent) {
    Q_UNUSED(pObject);
    if(pEvent->type() == QEvent::KeyPress) {
        QKeyEvent* pKeyEvent = static_cast<QKeyEvent*>(pEvent);
        if(pKeyEvent->key() == Qt::Key_Escape) {
            close();
            return true;
        }
    }

    return false;
}

void lmFormMain::closeEvent(QCloseEvent* pEvent) {
	//	close main window to system tray
	pEvent->ignore();
	minimize();
}

void lmFormMain::changeEvent(QEvent* pEvent) {
	switch(pEvent->type()) {
	case QEvent::WindowStateChange:
		if(minimizeHide) {
			QWindowStateChangeEvent* e = (QWindowStateChangeEvent*)pEvent;
			if(isMinimized() && e->oldState() != Qt::WindowMinimized) {
				QTimer::singleShot(0, this, SLOT(hide()));
				pEvent->ignore();
				showMinimizeMessage();
			}
		}
		break;
	case QEvent::LanguageChange:
		setUIText();
		break;
    default:
        break;
	}

	QWidget::changeEvent(pEvent);
}

void lmFormMain::sendMessage(MessageType type, QString* lpszUserId, MessageXml* pMessage) {
	emit messageSent(type, lpszUserId, pMessage);
}

void lmFormMain::trayShowAction_triggered(void) {
	restore();
}

void lmFormMain::trayHistoryAction_triggered(void) {
	emit showHistory();
}

void lmFormMain::trayFileAction_triggered(void) {
	emit showTransfers();
}

void lmFormMain::traySettingsAction_triggered(void) {
	emit showSettings();
}

void lmFormMain::trayAboutAction_triggered(void) {
	emit showAbout();
}

void lmFormMain::trayExitAction_triggered(void) {
    emit appExiting();
}

void lmFormMain::statusAction_triggered(QAction* action) {
	QString status = action->data().toString();
	int index = Helper::statusIndexFromCode(status);
	if(index != -1) {
		btnStatus->setIcon(QIcon(QPixmap(statusPic[index], "PNG")));
        trayStatusAction->setIcon(QIcon(QPixmap(statusPic[index], "PNG")));

		ui.lblStatus->setText(statusGroup->checkedAction()->text());
		pLocalUser->status = statusCode[index];
		pSettings->setValue(IDS_STATUS, pLocalUser->status);

        sendMessage(MT_Status, NULL, &status);
    }
}

void lmFormMain::avatarAction_triggered(void) {
	setAvatar();
}

void lmFormMain::avatarBrowseAction_triggered(void) {
	QString dir = pSettings->value(IDS_OPENPATH, IDS_OPENPATH_VAL).toString();
	QString fileName = QFileDialog::getOpenFileName(this, tr("Select avatar picture"), dir,
		"Images (*.bmp *.gif *.jpg *.jpeg *.png *.tif *.tiff)");
	if(!fileName.isEmpty()) {
		pSettings->setValue(IDS_OPENPATH, QFileInfo(fileName).dir().absolutePath());
		setAvatar(fileName);
	}
}

void lmFormMain::chatRoomAction_triggered(void) {
	// Generate a thread id which is a combination of the local user id and a guid
	// This ensures that no other user will generate an identical thread id
	QString threadId = Helper::getUuid();
	threadId.prepend(pLocalUser->id);
	emit chatRoomStarting(&threadId);
}

void lmFormMain::publicChatAction_triggered(void) {
	emit showPublicChat();
}

void lmFormMain::refreshAction_triggered(void) {
    QString szUserId;
    QString szMessage;

	sendMessage(MT_Refresh, &szUserId, &szMessage);
}

void lmFormMain::homePageAction_triggered(void) {
	QDesktopServices::openUrl(QUrl(IDA_DOMAIN));
}

void lmFormMain::trayIcon_activated(QSystemTrayIcon::ActivationReason reason) {
	switch(reason) {
	case QSystemTrayIcon::Trigger:
		if(singleClickActivation)
			processTrayIconTrigger();
		break;
	case QSystemTrayIcon::DoubleClick:
		if(!singleClickActivation)
			processTrayIconTrigger();
		break;
    default:
        break;
	}
}

void lmFormMain::trayMessage_clicked(void) {
	switch(lastTrayMessageType) {
	case TM_Status:
		trayShowAction_triggered();
		break;
	case TM_Transfer:
		emit showTransfers();
		break;
    default:
        break;
	}
}

void lmFormMain::tvUserList_itemActivated(QTreeWidgetItem* pItem, int column) {
    Q_UNUSED(column);
    if(pItem->data(0, TypeRole).toString().compare("User") == 0) {
        QString szUserId = pItem->data(0, IdRole).toString();
        emit chatStarting(&szUserId);
    }
}

void lmFormMain::tvUserList_itemContextMenu(QTreeWidgetItem* pItem, QPoint& pos) {
    if(!pItem)
        return;

    if(pItem->data(0, TypeRole).toString().compare("Group") == 0) {
        for(int index = 0; index < pGroupMenu->actions().count(); index++)
            pGroupMenu->actions()[index]->setData(pItem->data(0, IdRole));

		bool defGroup = (pItem->data(0, IdRole).toString().compare(GRP_DEFAULT_ID) == 0);
        pGroupMenu->actions()[3]->setEnabled(!defGroup);
        pGroupMenu->exec(pos);
    } else if(pItem->data(0, TypeRole).toString().compare("User") == 0) {
        for(int index = 0; index < pUserMenu->actions().count(); index++)
            pUserMenu->actions()[index]->setData(pItem->data(0, IdRole));

        bool fileCap = ((pItem->data(0, CapsRole).toUInt() & UC_File) == UC_File);
        pUserMenu->actions()[1]->setEnabled(fileCap);
        bool folderCap = ((pItem->data(0, CapsRole).toUInt() & UC_Folder) == UC_Folder);
        pUserMenu->actions()[2]->setEnabled(folderCap);
        pUserMenu->exec(pos);
    }
}

void lmFormMain::tvUserList_itemDragDropped(QTreeWidgetItem* pItem) {
    if(dynamic_cast<lmUserTreeWidgetUserItem*>(pItem)) {
        QString szUserId = pItem->data(0, IdRole).toString();
        QString szMessage = pItem->parent()->data(0, IdRole).toString();
        sendMessage(MT_Group, &szUserId, &szMessage);
		QTreeWidgetItem* pGroupItem = pItem->parent();
		pGroupItem->sortChildren(0, Qt::AscendingOrder);
    }
	else if(dynamic_cast<lmUserTreeWidgetGroupItem*>(pItem)) {
		int index = ui.tvUserList->indexOfTopLevelItem(pItem);
		QString groupId = pItem->data(0, IdRole).toString();
		emit groupUpdated(GO_Move, groupId, index);
	}
}

void lmFormMain::tvUserList_currentItemChanged(QTreeWidgetItem *pCurrent, QTreeWidgetItem *pPrevious) {
	Q_UNUSED(pPrevious);
	bool bEnabled = (pCurrent && pCurrent->data(0, TypeRole).toString().compare("User") == 0);
	toolChatAction->setEnabled(bEnabled);
	toolFileAction->setEnabled(bEnabled);
}

void lmFormMain::groupAddAction_triggered(void) {
	QString groupName = QInputDialog::getText(this, tr("Add New Group"), tr("Enter a name for the group"));

	if(groupName.isNull())
		return;

	if(getGroupItemByName(&groupName)) {
		QString msg = tr("A group named '%1' already exists. Please enter a different name.");
		QMessageBox::warning(this, lmStrings::appName(), msg.arg(groupName));
		return;
	}

	//generate a group id that is not assigned to any existing group
	QString groupId;
	do {
		groupId = Helper::getUuid();
	} while(getGroupItem(&groupId));
	
	emit groupUpdated(GO_New, groupId, groupName);
	lmUserTreeWidgetGroupItem *pItem = new lmUserTreeWidgetGroupItem();
	pItem->setData(0, IdRole, groupId);
	pItem->setData(0, TypeRole, "Group");
	pItem->setText(0, groupName);
	pItem->setSizeHint(0, QSize(0, 20));
	ui.tvUserList->addTopLevelItem(pItem);
	//	set the item as expanded after adding it to the treeview, else wont work
	pItem->setExpanded(true);
}

void lmFormMain::groupRenameAction_triggered(void) {
	QTreeWidgetItem* pGroupItem = ui.tvUserList->currentItem();
	QString groupId = pGroupItem->data(0, IdRole).toString();
	QString oldName = pGroupItem->data(0, Qt::DisplayRole).toString();
	QString newName = QInputDialog::getText(this, tr("Rename Group"), 
		tr("Enter a new name for the group"), QLineEdit::Normal, oldName);

	if(newName.isNull() || newName.compare(oldName) == 0)
		return;

	if(getGroupItemByName(&newName)) {
		QString msg = tr("A group named '%1' already exists. Please enter a different name.");
		QMessageBox::warning(this, lmStrings::appName(), msg.arg(newName));
		return;
	}

	emit groupUpdated(GO_Rename, groupId, newName);
	pGroupItem->setText(0, newName);
}

void lmFormMain::groupDeleteAction_triggered(void) {
	QTreeWidgetItem* pGroupItem = ui.tvUserList->currentItem();
	QString groupId = pGroupItem->data(0, IdRole).toString();
	QString defGroupId = GRP_DEFAULT_ID;
	QTreeWidgetItem* pDefGroupItem = getGroupItem(&defGroupId);
	while(pGroupItem->childCount()) {
		QTreeWidgetItem* pUserItem = pGroupItem->child(0);
		pGroupItem->removeChild(pUserItem);
		pDefGroupItem->addChild(pUserItem);
        QString szUserId = pUserItem->data(0, IdRole).toString();
        QString szMessage = pUserItem->parent()->data(0, IdRole).toString();
        sendMessage(MT_Group, &szUserId, &szMessage);
	}
	pDefGroupItem->sortChildren(0, Qt::AscendingOrder);

	emit groupUpdated(GO_Delete, groupId, QVariant());
	ui.tvUserList->takeTopLevelItem(ui.tvUserList->indexOfTopLevelItem(pGroupItem));
}

void lmFormMain::userConversationAction_triggered(void) {
	QString userId = ui.tvUserList->currentItem()->data(0, IdRole).toString();
	emit chatStarting(&userId);
}

void lmFormMain::userBroadcastAction_triggered(void) {
	emit showBroadcast();
}

void lmFormMain::userFileAction_triggered(void) {
	QString userId = ui.tvUserList->currentItem()->data(0, IdRole).toString();
	QString dir = pSettings->value(IDS_OPENPATH, IDS_OPENPATH_VAL).toString();
	QString fileName = QFileDialog::getOpenFileName(this, QString(), dir);
	if(!fileName.isEmpty()) {
		pSettings->setValue(IDS_OPENPATH, QFileInfo(fileName).dir().absolutePath());
        sendMessage(MT_File, &userId, &fileName);
	}
}

void lmFormMain::userFolderAction_triggered(void) {
    QString userId = ui.tvUserList->currentItem()->data(0, IdRole).toString();
    QString dir = pSettings->value(IDS_OPENPATH, IDS_OPENPATH_VAL).toString();
    QString path = QFileDialog::getExistingDirectory(this, QString(), dir, QFileDialog::ShowDirsOnly);
    if(!path.isEmpty()) {
        pSettings->setValue(IDS_OPENPATH, QFileInfo(path).absolutePath());
        sendMessage(MT_Folder, &userId, &path);
    }
}

void lmFormMain::userInfoAction_triggered(void) {
	QString userId = ui.tvUserList->currentItem()->data(0, IdRole).toString();
	QString message;
	sendMessage(MT_Query, &userId, &message);
}

void lmFormMain::txtNote_returnPressed(void) {
	//	Shift the focus from txtNote to another control
	ui.tvUserList->setFocus();
}

void lmFormMain::txtNote_lostFocus(void) {
	QString note = ui.txtNote->text();
    pSettings->setValue(IDS_NOTE, note, IDS_NOTE_VAL);
	pLocalUser->note = note;
	sendMessage(MT_Note, NULL, &note);
}

void lmFormMain::createMainMenu(void) {
    pMainMenu = new QMenuBar(this);

    // Messenger menu
    pFileMenu = pMainMenu->addMenu("&Messenger");

    chatRoomAction = new QAction(QIcon(ChatHelper::renderEmoji(Icons::Plus,16)),"&New Chat Room", this);
    chatRoomAction->setShortcut(QKeySequence::New);
    connect(chatRoomAction, &QAction::triggered, this, &lmFormMain::chatRoomAction_triggered);
    pFileMenu->addAction(chatRoomAction);

    publicChatAction = new QAction(QIcon(ChatHelper::renderEmoji(Icons::Public,16)), "&Public Chat", this);
    connect(publicChatAction, &QAction::triggered, this, &lmFormMain::publicChatAction_triggered);
    pFileMenu->addAction(publicChatAction);

    pFileMenu->addSeparator();

    refreshAction = new QAction(QIcon(ChatHelper::renderEmoji(Icons::Refresh,16)), "&Refresh contacts list", this);
    refreshAction->setShortcut(QKeySequence::Refresh);
    connect(refreshAction, &QAction::triggered, this, &lmFormMain::refreshAction_triggered);
    pFileMenu->addAction(refreshAction);

    pFileMenu->addSeparator();

    exitAction = new QAction(QIcon(ChatHelper::renderEmoji(Icons::Close,16)), "E&xit", this);
    connect(exitAction, &QAction::triggered, this, &lmFormMain::trayExitAction_triggered);
    pFileMenu->addAction(exitAction);

    // Tools menu
    pToolsMenu = pMainMenu->addMenu("&Tools");

    historyAction = new QAction(QIcon(ChatHelper::renderEmoji(Icons::History,16)), "&History", this);
    historyAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_H));
    connect(historyAction, &QAction::triggered, this, &lmFormMain::trayHistoryAction_triggered);
    pToolsMenu->addAction(historyAction);

    transferAction = new QAction(QIcon(ChatHelper::renderEmoji(Icons::Transfer,16)), "File &Transfers", this);
    transferAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_J));
    connect(transferAction, &QAction::triggered, this, &lmFormMain::trayFileAction_triggered);
    pToolsMenu->addAction(transferAction);

    pToolsMenu->addSeparator();

    settingsAction = new QAction(QIcon(ChatHelper::renderEmoji(Icons::Settings,16)), "&Preferences", this);
    settingsAction->setShortcut(QKeySequence::Preferences);
    connect(settingsAction, &QAction::triggered, this, &lmFormMain::traySettingsAction_triggered);
    pToolsMenu->addAction(settingsAction);

    // Help menu
    pHelpMenu = pMainMenu->addMenu("&Help");

    QString text = "%1 &online";
    onlineAction = new QAction(QIcon(ChatHelper::renderEmoji(Icons::Web,16)), text.arg(lmStrings::appName()), this);
    connect(onlineAction, &QAction::triggered, this, &lmFormMain::homePageAction_triggered);
    pHelpMenu->addAction(onlineAction);

    aboutAction = new QAction(QIcon(ChatHelper::renderEmoji(Icons::Info,16)), "&About", this);
    connect(aboutAction, &QAction::triggered, this, &lmFormMain::trayAboutAction_triggered);
    pHelpMenu->addAction(aboutAction);

    layout()->setMenuBar(pMainMenu);
}

void lmFormMain::createTrayMenu(void) {
    pTrayMenu = new QMenu(this);

    QString text = "&Show %1";
    trayShowAction = new QAction(QIcon(QPixmap(IDR_MESSENGER, "PNG")), text.arg(lmStrings::appName()), this);
    connect(trayShowAction, &QAction::triggered, this, &lmFormMain::trayShowAction_triggered);
    pTrayMenu->addAction(trayShowAction);

    pTrayMenu->addSeparator();

    int index = Helper::statusIndexFromCode(pLocalUser->status);
    //	if status is not recognized, default to available
    index = qMax(index, 0);

    trayStatusAction = pTrayMenu->addMenu(pStatusMenu);
    trayStatusAction->setIcon(QIcon(QPixmap(statusPic[index], "PNG")));
    trayStatusAction->setText("&Change Status");

    pTrayMenu->addSeparator();

    trayHistoryAction = new QAction(QIcon(ChatHelper::renderEmoji(Icons::History,16)), "&History", this);
    connect(trayHistoryAction, &QAction::triggered, this, &lmFormMain::trayHistoryAction_triggered);
    pTrayMenu->addAction(trayHistoryAction);

    trayTransferAction = new QAction(QIcon(ChatHelper::renderEmoji(Icons::Transfer,16)), "File &Transfers", this);
    connect(trayTransferAction, &QAction::triggered, this, &lmFormMain::trayFileAction_triggered);
    pTrayMenu->addAction(trayTransferAction);

    pTrayMenu->addSeparator();

    traySettingsAction = new QAction(QIcon(ChatHelper::renderEmoji(Icons::Settings,16)), "&Preferences", this);
    connect(traySettingsAction, &QAction::triggered, this, &lmFormMain::traySettingsAction_triggered);
    pTrayMenu->addAction(traySettingsAction);

    trayAboutAction = new QAction(QIcon(ChatHelper::renderEmoji(Icons::Info,16)), "&About", this);
    connect(trayAboutAction, &QAction::triggered, this, &lmFormMain::trayAboutAction_triggered);
    pTrayMenu->addAction(trayAboutAction);

    pTrayMenu->addSeparator();

    trayExitAction = new QAction(QIcon(ChatHelper::renderEmoji(Icons::Close,16)), "E&xit", this);
    connect(trayExitAction, &QAction::triggered, this, &lmFormMain::trayExitAction_triggered);
    pTrayMenu->addAction(trayExitAction);

    pTrayMenu->setDefaultAction(trayShowAction);
}

void lmFormMain::createTrayIcon(void) {
	pTrayIcon = new QSystemTrayIcon(this);
	pTrayIcon->setIcon(QIcon(IDR_APPICON));
	pTrayIcon->setContextMenu(pTrayMenu);
	
	connect(pTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), 
		this, SLOT(trayIcon_activated(QSystemTrayIcon::ActivationReason)));
	connect(pTrayIcon, SIGNAL(messageClicked()), this, SLOT(trayMessage_clicked()));
}

void lmFormMain::createStatusMenu(void) {
    pStatusMenu = new QMenu(this);
	statusGroup = new QActionGroup(this);

	connect(statusGroup, &QActionGroup::triggered, this, &lmFormMain::statusAction_triggered);

	for(int index = 0; index < ST_COUNT; index++) {
		QAction* pAction = new QAction(QIcon(QPixmap(statusPic[index], "PNG")), lmStrings::statusDesc()[index], this);
		pAction->setData(statusCode[index]);
		pAction->setCheckable(true);
		statusGroup->addAction(pAction);
		pStatusMenu->addAction(pAction);
	}
    btnStatus->setPopupMode(QToolButton::InstantPopup);
    btnStatus->setMinimumWidth(40);
    btnStatus->setMenu(pStatusMenu);
}

void lmFormMain::createAvatarMenu(void) {
	pAvatarMenu = new QMenu(this);

    lmImagePickerAction* pAction = new lmImagePickerAction(this, avtEmoji, AVT_COUNT, 48, 4, &nAvatar);
	connect(pAction, &lmImagePickerAction::triggered, this, &lmFormMain::avatarAction_triggered);
	pAvatarMenu->addAction(pAction);
	pAvatarMenu->addSeparator();
	avatarBrowseAction = new QAction("&Select picture...", this);
    connect(avatarBrowseAction, &QAction::triggered, this, &lmFormMain::avatarBrowseAction_triggered);
    pAvatarMenu->addAction(avatarBrowseAction);

	ui.btnAvatar->setMenu(pAvatarMenu);
}

void lmFormMain::createGroupMenu(void) {
    pGroupMenu = new QMenu(this);

    groupAddAction = new QAction("Add &New Group", this);
    connect(groupAddAction, &QAction::triggered, this, &lmFormMain::groupAddAction_triggered);
    pGroupMenu->addAction(groupAddAction);

    pGroupMenu->addSeparator();

    groupRenameAction = new QAction("&Rename This Group", this);
    connect(groupRenameAction, &QAction::triggered, this, &lmFormMain::groupRenameAction_triggered);
    pGroupMenu->addAction(groupRenameAction);

    groupDeleteAction = new QAction("&Delete This Group", this);
    connect(groupDeleteAction, &QAction::triggered, this, &lmFormMain::groupDeleteAction_triggered);
    pGroupMenu->addAction(groupDeleteAction);
}

void lmFormMain::createUserMenu(void) {
    pUserMenu = new QMenu(this);

    userChatAction = new QAction("&Conversation", this);
    connect(userChatAction, &QAction::triggered, this, &lmFormMain::userConversationAction_triggered);
    pUserMenu->addAction(userChatAction);

    userFileAction = new QAction("Send &File", this);
    connect(userFileAction, &QAction::triggered, this, &lmFormMain::userFileAction_triggered);
    pUserMenu->addAction(userFileAction);

    userFolderAction = new QAction("Send a Fol&der", this);
    connect(userFolderAction, &QAction::triggered, this, &lmFormMain::userFolderAction_triggered);
    pUserMenu->addAction(userFolderAction);

    pUserMenu->addSeparator();

    userBroadcastAction = new QAction("Send &Broadcast Message", this);
    connect(userBroadcastAction, &QAction::triggered, this, &lmFormMain::userBroadcastAction_triggered);
    pUserMenu->addAction(userBroadcastAction);

    pUserMenu->addSeparator();

    userInfoAction = new QAction("Get &Information", this);
    connect(userInfoAction, &QAction::triggered, this, &lmFormMain::userInfoAction_triggered);
    pUserMenu->addAction(userInfoAction);
}

void lmFormMain::createToolBar(void) {
    QToolBar* pStatusBar = new QToolBar(ui.frame);
    pStatusBar->setStyleSheet("QToolBar { border: 0px; padding: 0px; }");
    ui.statusLayout->insertWidget(0, pStatusBar);

    QAction* pStatusAction = new QAction("", this);
    pStatusBar->addAction(pStatusAction);
    btnStatus = qobject_cast<QToolButton*>(pStatusBar->widgetForAction(pStatusAction));
    btnStatus->setPopupMode(QToolButton::MenuButtonPopup);

    QToolBar* pToolBar = new QToolBar(ui.wgtToolBar);
    pToolBar->setIconSize(QSize(40, 20));
    ui.toolBarLayout->addWidget(pToolBar);
    pToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    toolChatAction = new QAction(QIcon(ChatHelper::renderEmoji(Icons::Chat,20)), "&Conversation", this);
    connect(toolChatAction, &QAction::triggered, this, &lmFormMain::userConversationAction_triggered);
    toolChatAction->setEnabled(false);
    pToolBar->addAction(toolChatAction);

    toolFileAction = new QAction(QIcon(ChatHelper::renderEmoji(Icons::File,20)), "Send &File", this);
    connect(toolFileAction, &QAction::triggered, this, &lmFormMain::userFileAction_triggered);
    toolFileAction->setEnabled(false);
    pToolBar->addAction(toolFileAction);

    pToolBar->addSeparator();

    toolBroadcastAction = new QAction(QIcon(ChatHelper::renderEmoji(Icons::Broadcast,20)), "Send &Broadcast Message", this);
    connect(toolBroadcastAction, &QAction::triggered, this, &lmFormMain::userBroadcastAction_triggered);
    pToolBar->addAction(toolBroadcastAction);

    pToolBar->addSeparator();

    toolChatRoomAction = new QAction(QIcon(ChatHelper::renderEmoji(Icons::Plus,20)), "&New Chat Room", this);
    connect(toolChatRoomAction, &QAction::triggered, this, &lmFormMain::chatRoomAction_triggered);
    pToolBar->addAction(toolChatRoomAction);

    toolPublicChatAction = new QAction(QIcon(ChatHelper::renderEmoji(Icons::Public,20)), "&Public Chat", this);
    connect(toolPublicChatAction, &QAction::triggered, this, &lmFormMain::publicChatAction_triggered);
    pToolBar->addAction(toolPublicChatAction);

    // Set auto-raise for all tool buttons
    QList<QAction*> actions = { toolChatAction, toolFileAction, toolBroadcastAction, toolChatRoomAction, toolPublicChatAction };
    for (QAction* action : actions) {
        if (QToolButton* button = qobject_cast<QToolButton*>(pToolBar->widgetForAction(action))) {
            button->setAutoRaise(false);
        }
    }
}

void lmFormMain::setUIText(void) {
	ui.retranslateUi(this);

	setWindowTitle(lmStrings::appName());

	pFileMenu->setTitle(tr("&Messenger"));
	chatRoomAction->setText(tr("&New Chat Room"));
	publicChatAction->setText(tr("&Public Chat"));
	refreshAction->setText(tr("&Refresh Contacts List"));
	exitAction->setText(tr("E&xit"));
	pToolsMenu->setTitle(tr("&Tools"));
	historyAction->setText(tr("&History"));
	transferAction->setText(tr("File &Transfers"));
	settingsAction->setText(tr("&Preferences"));
	pHelpMenu->setTitle(tr("&Help"));
	QString text = tr("%1 &online");
	onlineAction->setText(text.arg(lmStrings::appName()));
	aboutAction->setText(tr("&About"));
	text = tr("&Show %1");
	trayShowAction->setText(text.arg(lmStrings::appName()));
	trayStatusAction->setText(tr("&Change Status"));
	trayHistoryAction->setText(tr("&History"));
	trayTransferAction->setText(tr("File &Transfers"));
	traySettingsAction->setText(tr("&Preferences"));
	trayAboutAction->setText(tr("&About"));
	trayExitAction->setText(tr("E&xit"));
	groupAddAction->setText(tr("Add &New Group"));
	groupRenameAction->setText(tr("&Rename This Group"));
	groupDeleteAction->setText(tr("&Delete This Group"));
	userChatAction->setText(tr("&Conversation"));
	userBroadcastAction->setText(tr("Send &Broadcast Message"));
	userFileAction->setText(tr("Send &File"));
    userFolderAction->setText(tr("Send Fol&der"));
	userInfoAction->setText(tr("Get &Information"));
	avatarBrowseAction->setText(tr("&Browse for more pictures..."));
	toolChatAction->setText(tr("&Conversation"));
	toolFileAction->setText(tr("Send &File"));
	toolBroadcastAction->setText(tr("Send &Broadcast Message"));
	toolChatRoomAction->setText(tr("&New Chat Room"));
	toolPublicChatAction->setText(tr("&Public Chat"));

	for(int index = 0; index < statusGroup->actions().count(); index++)
		statusGroup->actions()[index]->setText(lmStrings::statusDesc()[index]);
	
	ui.lblUserName->setText(pLocalUser->name);	// in case of retranslation
	if(statusGroup->checkedAction())
		ui.lblStatus->setText(statusGroup->checkedAction()->text());

	for(int index = 0; index < ui.tvUserList->topLevelItemCount(); index++) {
		QTreeWidgetItem* item = ui.tvUserList->topLevelItem(index);
		for(int childIndex = 0; childIndex < item->childCount(); childIndex++) {
			QTreeWidgetItem*childItem = item->child(childIndex);
			int statusIndex = childItem->data(0, StatusRole).toInt();
			childItem->setToolTip(0, lmStrings::statusDesc()[statusIndex]);
		}
	}

	setTrayTooltip();
}

void lmFormMain::showMinimizeMessage(void) {
	if(showMinimizeMsg) {
		QString msg = tr("%1 will continue to run in the background. Activate this icon to restore the application window.");
		showTrayMessage(TM_Minimize, msg.arg(lmStrings::appName()));
		showMinimizeMsg = false;
	}
}

void lmFormMain::initGroups(QList<Group>* pGroupList) {
	for(int index = 0; index < pGroupList->count(); index++) {
		lmUserTreeWidgetGroupItem *pItem = new lmUserTreeWidgetGroupItem();
		pItem->setData(0, IdRole, pGroupList->value(index).id);
		pItem->setData(0, TypeRole, "Group");
		pItem->setText(0, pGroupList->value(index).name);
		pItem->setSizeHint(0, QSize(0, 22));
		ui.tvUserList->addTopLevelItem(pItem);
	}

	ui.tvUserList->expandAll();
	// size will be either number of items in group expansion list or number of top level items in
	// treeview control, whichever is less. This is to  eliminate arary out of bounds error.
	int size = qMin(pSettings->beginReadArray(IDS_GROUPEXPHDR), ui.tvUserList->topLevelItemCount());
	for(int index = 0; index < size; index++) {
		pSettings->setArrayIndex(index);
		ui.tvUserList->topLevelItem(index)->setExpanded(pSettings->value(IDS_GROUP).toBool());
	}
	pSettings->endArray();
}

void lmFormMain::updateStatusImage(QTreeWidgetItem* pItem, QString* lpszStatus) {
    int index = Helper::statusIndexFromCode(*lpszStatus);
    if(index != -1)
        pItem->setIcon(0, QIcon(QPixmap(statusPic[index], "PNG")));
}

void lmFormMain::setAvatar(QString fileName) {
	//	create cache folder if it does not exist
	QDir cacheDir(DefinitionsDir::cacheDir());
	if(!cacheDir.exists())
		cacheDir.mkdir(cacheDir.absolutePath());
	QString filePath = DefinitionsDir::avatarFile();

	//	Save the image as a file in the data folder
	QPixmap avatar;
    bool loadFromStdPath = false;
	if(!fileName.isEmpty()) {
		//	save a backup of the image in the cache folder
		avatar = QPixmap(fileName);
		nAvatar = -1;
	} else {
		//	nAvatar = -1 means custom avatar is set, otherwise load from resource
		if(nAvatar < 0) {
			//	load avatar from image file if file exists, else load default
            if(QFile::exists(filePath)) {
				avatar = QPixmap(filePath);
                loadFromStdPath = true;
            }
			else
                avatar = ChatHelper::renderEmoji(avtEmoji[20], 48).pixmap(48, 48); //Hidden Default Avatar
		} else
            avatar = ChatHelper::renderEmoji(avtEmoji[nAvatar], 48).pixmap(48, 48);
	}

    if(!loadFromStdPath) {
        avatar = avatar.scaled(QSize(AVT_WIDTH, AVT_HEIGHT), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        avatar.save(filePath);
    }

	ui.btnAvatar->setIcon(QIcon(QPixmap(filePath, "PNG")));
	pLocalUser->avatar = nAvatar;
	sendAvatar(NULL);
}

QTreeWidgetItem* lmFormMain::getUserItem(QString* lpszUserId) {
	for(int topIndex = 0; topIndex < ui.tvUserList->topLevelItemCount(); topIndex++) {
		for(int index = 0; index < ui.tvUserList->topLevelItem(topIndex)->childCount(); index++) {
			QTreeWidgetItem* pItem = ui.tvUserList->topLevelItem(topIndex)->child(index);
			if(pItem->data(0, IdRole).toString().compare(*lpszUserId) == 0)
				return pItem;
		}
	}

	return NULL;
}

QTreeWidgetItem* lmFormMain::getGroupItem(QString* lpszGroupId) {
	for(int topIndex = 0; topIndex < ui.tvUserList->topLevelItemCount(); topIndex++) {
		QTreeWidgetItem* pItem = ui.tvUserList->topLevelItem(topIndex);
		if(pItem->data(0, IdRole).toString().compare(*lpszGroupId) == 0)
			return pItem;
	}

	return NULL;
}

QTreeWidgetItem* lmFormMain::getGroupItemByName(QString* lpszGroupName) {
	for(int topIndex = 0; topIndex < ui.tvUserList->topLevelItemCount(); topIndex++) {
		QTreeWidgetItem* pItem = ui.tvUserList->topLevelItem(topIndex);
		if(pItem->data(0, Qt::DisplayRole).toString().compare(*lpszGroupName) == 0)
			return pItem;
	}

	return NULL;
}

void lmFormMain::sendMessage(MessageType type, QString* lpszUserId, QString* lpszMessage) {
	MessageXml xmlMessage;
	
	switch(type) {
	case MT_Status:
		xmlMessage.addData(XN_STATUS, *lpszMessage);
		break;
	case MT_Note:
		xmlMessage.addData(XN_NOTE, *lpszMessage);
		break;
	case MT_Refresh:
		break;
	case MT_Group:
		xmlMessage.addData(XN_GROUP, *lpszMessage);
		break;
    case MT_File:
    case MT_Folder:
		xmlMessage.addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
		xmlMessage.addData(XN_FILEOP, FileOpNames[FO_Request]);
		xmlMessage.addData(XN_FILEPATH, *lpszMessage);
		break;
    case MT_Avatar:
        xmlMessage.addData(XN_FILETYPE, FileTypeNames[FT_Avatar]);
        xmlMessage.addData(XN_FILEOP, FileOpNames[FO_Request]);
        xmlMessage.addData(XN_FILEPATH, *lpszMessage);
        break;
	case MT_Query:
		xmlMessage.addData(XN_QUERYOP, QueryOpNames[QO_Get]);
		break;
	default:
		break;
	}

	sendMessage(type, lpszUserId, &xmlMessage);
}

void lmFormMain::sendAvatar(QString* lpszUserId) {
    QString filePath = DefinitionsDir::avatarFile();
	if(!QFile::exists(filePath))
		return;

    sendMessage(MT_Avatar, lpszUserId, &filePath);
}

void lmFormMain::setUserAvatar(QString* lpszUserId, QString *lpszFilePath) {
	QTreeWidgetItem* pUserItem = getUserItem(lpszUserId);
	if(!pUserItem)
		return;

    QPixmap avatar;
    if(!lpszFilePath || !QFile::exists(*lpszFilePath))
        avatar = ChatHelper::renderEmoji(avtEmoji[20], 48).pixmap(48, 48); //Hidden Default Avatar
    else
        avatar.load(*lpszFilePath);
    avatar = avatar.scaled(QSize(32, 32), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	pUserItem->setData(0, AvatarRole, QIcon(avatar));
}

void lmFormMain::processTrayIconTrigger(void) {
	// If system tray minimize is disabled, restore() will be called every time.
	// Otherwise, window is restored or minimized
	if(!allowSysTrayMinimize || isHidden() || isMinimized())
		restore();
	else
		minimize();
}

void lmFormMain::setTrayTooltip(void) {
	if(bConnected)
		pTrayIcon->setToolTip(lmStrings::appName());
	else {
		QString msg = tr("%1 - Not Connected");
		pTrayIcon->setToolTip(msg.arg(lmStrings::appName()));
	}
}
