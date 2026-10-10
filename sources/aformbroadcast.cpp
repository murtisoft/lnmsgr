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

#include <QActionGroup>
#include <QSet>
#include "aformbroadcast.h"
#include "widgetimagepicker.h"
#include "definitionsui.h"
#include "sharedchatfunctions.h"
#include "translatabletext.h"
#include "shareduifunctions.h"

//	constructor
lmFormBroadcast::lmFormBroadcast(QWidget *parent) : QWidget(parent) {
	ui.setupUi(this);

	//	Destroy the window when it closes
	setAttribute(Qt::WA_DeleteOnClose, true);
	
	//	set up the initial default size of splitter panels
	//	left panel takes up 60% of total width, right panel the rest
	QList<int> sizes;
	sizes.append(width() * 0.6);
	sizes.append(width() - width() * 0.6 - ui.splitter->handleWidth());
	ui.splitter->setSizes(sizes);

	connect(ui.btnSelectAll, SIGNAL(clicked()), this, SLOT(btnSelectAll_clicked()));
	connect(ui.btnSelectNone, SIGNAL(clicked()), this, SLOT(btnSelectNone_clicked()));
    connect(ui.tvUserList, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
        this, SLOT(tvUserList_itemChanged(QTreeWidgetItem*,int)));
	connect(ui.btnSend, SIGNAL(clicked()), this, SLOT(btnSend_clicked()));
	connect(ui.txtMessage, SIGNAL(textChanged()), this, SLOT(txtMessage_textChanged()));

	//	event filters for handling keyboard input
	ui.txtMessage->installEventFilter(this);
	ui.tvUserList->installEventFilter(this);
    ui.btnSend->installEventFilter(this);
    ui.btnCancel->installEventFilter(this);
    ui.btnSelectAll->installEventFilter(this);
    ui.btnSelectNone->installEventFilter(this);

	ui.lblInfo->setVisible(false);
	infoFlag = IT_Ok;

	parentToggling = false;
	childToggling = false;
	bConnected = false;
}

lmFormBroadcast::~lmFormBroadcast() {
}

void lmFormBroadcast::init(bool connected) {
	createToolBar();

	setWindowIcon(QIcon(IDR_APPICON));

	ui.tvUserList->setIconSize(QSize(16, 16));
    ui.tvUserList->header()->setSectionsMovable(false);
	ui.tvUserList->header()->setStretchLastSection(false);
    ui.tvUserList->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	ui.tvUserList->setCheckable(true);

	//	load settings
	pSettings = new lmSettings();
	restoreGeometry(pSettings->value(IDS_WINDOWBROADCAST).toByteArray());
	ui.splitter->restoreState(pSettings->value(IDS_SPLITTERBROADCAST).toByteArray());
	showSmiley = pSettings->value(IDS_EMOTICON, IDS_EMOTICON_VAL).toBool();
	sendKeyMod = pSettings->value(IDS_SENDKEYMOD, IDS_SENDKEYMOD_VAL).toBool();
	fontSizeVal = pSettings->value(IDS_FONTSIZE, IDS_FONTSIZE_VAL).toInt();
    pFontGroup->actions().at(fontSizeVal)->setChecked(true);
	int viewType = pSettings->value(IDS_USERLISTVIEW, IDS_USERLISTVIEW_VAL).toInt();
	ui.tvUserList->setView((UserListView)viewType);

	//	show a message if not connected
	bConnected = connected;
	if(!bConnected)
		showStatus(IT_Disconnected, true);

	setUIText();

	ui.txtMessage->setStyleSheet("QTextEdit { " + fontStyle[fontSizeVal] + " }");
	ui.txtMessage->setFocus();
}

void lmFormBroadcast::stop(void) {
	//	save window geometry and splitter panel sizes
	pSettings->setValue(IDS_WINDOWBROADCAST, saveGeometry());
	pSettings->setValue(IDS_SPLITTERBROADCAST, ui.splitter->saveState());
}

void lmFormBroadcast::show(QList<QTreeWidgetItem*>* pGroupList) {

	
	if(!pGroupList)
		return;

	//	populate the user tree
	ui.tvUserList->clear();
	for(int index = 0; index < pGroupList->count(); index++) {
		QTreeWidgetItem* pItem = pGroupList->value(index);
		pItem->setCheckState(0, Qt::Unchecked);
		for(int childIndex = 0; childIndex < pItem->childCount(); childIndex++)
			pItem->child(childIndex)->setCheckState(0, Qt::Unchecked);
		ui.tvUserList->addTopLevelItem(pItem);
	}
	ui.tvUserList->expandAll();

	btnSelectAll_clicked();
	updateSendState();
    QWidget::show();
}

//	rebuild the user tree while keeping the current selection
//	users that were not in the list before are left unchecked
void lmFormBroadcast::refreshUsers(QList<QTreeWidgetItem*>* pGroupList) {
	if(!pGroupList)
		return;

	//	remember who is currently checked
	QSet<QString> checkedIds;
	for(int index = 0; index < ui.tvUserList->topLevelItemCount(); index++) {
		QTreeWidgetItem* pGroup = ui.tvUserList->topLevelItem(index);
		for(int childIndex = 0; childIndex < pGroup->childCount(); childIndex++) {
			QTreeWidgetItem* pChild = pGroup->child(childIndex);
			if(pChild->checkState(0) == Qt::Checked)
				checkedIds.insert(pChild->data(0, IdRole).toString());
		}
	}

	//	populate the tree, check states are set before the items join the tree
	//	so that itemChanged is not triggered
	ui.tvUserList->clear();
	for(int index = 0; index < pGroupList->count(); index++) {
		QTreeWidgetItem* pItem = pGroupList->value(index);
		int nChecked = 0;
		for(int childIndex = 0; childIndex < pItem->childCount(); childIndex++) {
			QTreeWidgetItem* pChild = pItem->child(childIndex);
			bool checked = checkedIds.contains(pChild->data(0, IdRole).toString());
			pChild->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
			if(checked)
				nChecked++;
		}
		bool allChecked = (pItem->childCount() > 0 && nChecked == pItem->childCount());
		pItem->setCheckState(0, allChecked ? Qt::Checked : Qt::Unchecked);
		ui.tvUserList->addTopLevelItem(pItem);
	}
	ui.tvUserList->expandAll();

	updateSendState();
}

void lmFormBroadcast::connectionStateChanged(bool connected) {
	bConnected = connected;
	bConnected ? showStatus(IT_Disconnected, false) : showStatus(IT_Disconnected, true);
	updateSendState();
}

void lmFormBroadcast::settingsChanged(void) {
	showSmiley = pSettings->value(IDS_EMOTICON, IDS_EMOTICON_VAL).toBool();
	sendKeyMod = pSettings->value(IDS_SENDKEYMOD, IDS_SENDKEYMOD_VAL).toBool();
	int viewType = pSettings->value(IDS_USERLISTVIEW, IDS_USERLISTVIEW_VAL).toInt();
	ui.tvUserList->setView((UserListView)viewType);
}

//	this method receives keyboard events and check if Enter key or Escape key were pressed
//	if so, corresponding functions are called
bool lmFormBroadcast::eventFilter(QObject* pObject, QEvent* pEvent) {
	if(pEvent->type() == QEvent::KeyPress) {
		QKeyEvent* pKeyEvent = static_cast<QKeyEvent*>(pEvent);
		if(pKeyEvent->key() == Qt::Key_Escape) {
			close();
			return true;
		} else if((pKeyEvent->key() == Qt::Key_Return || pKeyEvent->key() == Qt::Key_Enter) && pObject == ui.txtMessage) {
            if (pKeyEvent->modifiers() & Qt::ShiftModifier) {  //Shift Enter always adds a newline regardless of setting.
                ui.txtMessage->insertPlainText("\n");
                return true;
            }
			bool keyMod = ((pKeyEvent->modifiers() & Qt::ControlModifier) == Qt::ControlModifier);
			if(keyMod == sendKeyMod) {
				sendMessage();
				return true;
			}
			// The TextEdit widget does not insert new line when Ctrl+Enter is pressed
			// So we insert a new line manually
			if(keyMod)
				ui.txtMessage->insertPlainText("\n");
		}
	}

	return false;
}

void lmFormBroadcast::changeEvent(QEvent* pEvent) {
	switch(pEvent->type()) {
	case QEvent::LanguageChange:
		setUIText();
		break;
    default:
        break;
	}

	QWidget::changeEvent(pEvent);
}

void lmFormBroadcast::closeEvent(QCloseEvent* pEvent) {
	ui.txtMessage->clear();
	btnSelectNone_clicked();

	QWidget::closeEvent(pEvent);
}

//	change the font size of text box through menu
void lmFormBroadcast::fontAction_triggered(QAction* action) {
	fontSizeVal = action->data().toInt();
	ui.txtMessage->setStyleSheet("QTextEdit { " + fontStyle[fontSizeVal] + " }");
}

//	insert a smiley into the text box
void lmFormBroadcast::smileyAction_triggered(void) {
	//	if smileys are enabled, insert an image else insert a text smiley
	//	nSmiley contains index of smiley
	if(showSmiley) {
        QString htmlPic;
        if (smileyEmoji[nSmiley].startsWith(":/")) {
            htmlPic = ("<img src='" + smileyEmoji[nSmiley] + "' />");
        } else {
            htmlPic = ("&#8203;<span style='font-size:18px; vertical-align: middle;'>" + smileyEmoji[nSmiley] + "</span>&#8203;");
        }
        ui.txtMessage->insertHtml(htmlPic);
	}
	else
		ui.txtMessage->insertPlainText(smileyCode[nSmiley]);
}

//	select all users in the user tree
void lmFormBroadcast::btnSelectAll_clicked(void) {
	for(int index = 0; index < ui.tvUserList->topLevelItemCount(); index++)
		ui.tvUserList->topLevelItem(index)->setCheckState(0, Qt::Checked);
}

//	deselect all users in the user tree
void lmFormBroadcast::btnSelectNone_clicked(void) {
	for(int index = 0; index < ui.tvUserList->topLevelItemCount(); index++)
		ui.tvUserList->topLevelItem(index)->setCheckState(0, Qt::Unchecked);
}

//	event called when the user checks/unchecks a tree item
void lmFormBroadcast::tvUserList_itemChanged(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);

	//	if parent tree item was toggled, update all its children to the same state
	//	if a child tree item was toggled, two cases arise:
	//		if all its siblings and it are checked, update its parent to checked
	//		if all its siblings and it are not checked, update its parent to unchecked
	if(item->data(0, TypeRole).toString().compare("Group") == 0 && !childToggling) {
		parentToggling = true;
		for(int index = 0; index < item->childCount(); index++)
			item->child(index)->setCheckState(0, item->checkState(0));
		parentToggling = false;
	} else if(item->data(0, TypeRole).toString().compare("User") == 0 && !parentToggling) {
		childToggling = true;
		int nChecked = 0;
		QTreeWidgetItem* parent = item->parent();
		for(int index = 0; index < parent->childCount(); index++)
			if(parent->child(index)->checkState(0))
				nChecked++;
		Qt::CheckState check = (nChecked == parent->childCount()) ? Qt::Checked : Qt::Unchecked;
		parent->setCheckState(0, check);
		childToggling = false;
	}

	updateSendState();
}

void lmFormBroadcast::btnSend_clicked(void) {
	sendMessage();
}

void lmFormBroadcast::txtMessage_textChanged(void) {
	updateSendState();
}

//	create toolbar and add buttons
void lmFormBroadcast::createToolBar(void) {
	//	create the toolbar
	pToolBar = new QToolBar(ui.toolBarWidget);
    ui.toolBarWidget->setStyleSheet("QToolBar::separator { background: transparent; margin: 0; }");
	pToolBar->setStyleSheet("QToolBar { border: 0px }");
	pToolBar->setIconSize(QSize(16, 16));
	ui.toolBarLayout->addWidget(pToolBar);

	//	create the font menu
	QMenu* pFontMenu = new QMenu(this);
	pFontGroup = new QActionGroup(this);
	connect(pFontGroup, SIGNAL(triggered(QAction*)), this, SLOT(fontAction_triggered(QAction*)));

	for(int index = 0; index < FS_COUNT; index++) {
		QAction* pAction = new QAction(lmStrings::fontSize()[index], this);
		pAction->setCheckable(true);
		pAction->setData(index);
		pFontGroup->addAction(pAction);
		pFontMenu->addAction(pAction);
	}

	//	create the smiley menu
    lmImagePickerAction* pSmileyAction = new lmImagePickerAction(this, smileyEmoji, SM_COUNT, 19, 10, &nSmiley);
	connect(pSmileyAction, SIGNAL(triggered()), this, SLOT(smileyAction_triggered()));

    pToolBar->addSeparator();
	QMenu* pSmileyMenu = new QMenu(this);
	pSmileyMenu->addAction(pSmileyAction);

	//	create the smiley tool button
    pbtnSmiley = new QToolButton(pToolBar);
    pbtnSmiley->setIcon(Helper::renderEmoji(Icons::Smiley,16));
	pbtnSmiley->setPopupMode(QToolButton::InstantPopup);
    pbtnSmiley->setStyleSheet("QToolButton::menu-indicator { image: none; }");
	pbtnSmiley->setMenu(pSmileyMenu);
	pToolBar->addWidget(pbtnSmiley);
}

void lmFormBroadcast::setUIText(void) {
	ui.retranslateUi(this);

	setWindowTitle(tr("Send Broadcast Message"));

	pbtnSmiley->setToolTip(tr("Insert Smiley"));

	for(int index = 0; index < pFontGroup->actions().count(); index++)
        pFontGroup->actions().at(index)->setText(lmStrings::fontSize()[index]);

	updateSendState();
}

//	send the broadcast message to all selected users
void lmFormBroadcast::sendMessage(void) {
	//	do nothing if sending is currently blocked (the send button explains why)
	if(!sendBlockReason().isEmpty())
		return;

	{
		QString szHtmlMessage(ui.txtMessage->toHtml());
		encodeMessage(&szHtmlMessage);
		QTextDocument docMessage;
		docMessage.setHtml(szHtmlMessage);
		QString szMessage(docMessage.toPlainText());
		
		//	send broadcast
		MessageXml xmlMessage;
		xmlMessage.addData(XN_BROADCAST, szMessage);
		for(int index = 0; index < ui.tvUserList->topLevelItemCount(); index++) {
			for(int childIndex = 0; childIndex < ui.tvUserList->topLevelItem(index)->childCount(); childIndex++) {
				QTreeWidgetItem* item = ui.tvUserList->topLevelItem(index)->child(childIndex);
				if(item->checkState(0) == Qt::Checked) {
                    QString szUserId = item->data(0, IdRole).toString();
					emit messageSent(MT_Broadcast, &szUserId, &xmlMessage);
				}
			}
		}

		ui.txtMessage->clear();
		close();
	}
}

//	Called before sending message
void lmFormBroadcast::encodeMessage(QString* lpszMessage) {
	//	replace all emoticon images with corresponding text code
	ChatHelper::encodeSmileys(lpszMessage);
}

//	count the checked users in the tree
int lmFormBroadcast::countSelectedUsers(void) {
	int count = 0;
	for(int index = 0; index < ui.tvUserList->topLevelItemCount(); index++) {
		QTreeWidgetItem* pGroup = ui.tvUserList->topLevelItem(index);
		for(int childIndex = 0; childIndex < pGroup->childCount(); childIndex++)
			if(pGroup->child(childIndex)->checkState(0) == Qt::Checked)
				count++;
	}
	return count;
}

//	returns why the broadcast cannot be sent right now, or an empty string if it can
QString lmFormBroadcast::sendBlockReason(void) {
	if(!bConnected)
		return tr("You are not connected. Broadcast message cannot be sent.");
	if(countSelectedUsers() == 0)
		return tr("Select at least one recipient to send a broadcast.");
	if(ui.txtMessage->document()->isEmpty())
		return tr("Type a message to send.");
	return QString();
}

//	enable or disable the send button and explain the reason in its tooltip
void lmFormBroadcast::updateSendState(void) {
	QString reason = sendBlockReason();
	ui.btnSend->setEnabled(reason.isEmpty());
	ui.btnSend->setToolTip(reason);
}

//	show a message depending on the connection state
void lmFormBroadcast::showStatus(int flag, bool add) {
	infoFlag = add ? infoFlag | flag : infoFlag & ~flag;

	ui.lblInfo->setStyleSheet("QLabel { background-color:white;font-size:9pt; }");
	if(infoFlag & IT_Disconnected) {
		ui.lblInfo->setText("<span style='color:rgb(192, 0, 0);'>" + 
			tr("You are no longer connected. Broadcast message cannot be sent.") + "</span>");
		ui.lblInfo->setVisible(true);
	} else {
		ui.lblInfo->setText(QString());
		ui.lblInfo->setVisible(false);
	}
}
