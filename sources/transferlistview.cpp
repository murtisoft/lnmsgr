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


#include "transferlistview.h"

lmTransferListView::lmTransferListView(QWidget* parent) : QListView(parent) {
	pModel = new FileModel();
	setModel(pModel);
	setItemDelegate(new FileDelegate);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void lmTransferListView::insertItem(int row, FileView* fileTransfer) {
	pModel->insertItem(row, fileTransfer);
}

void lmTransferListView::removeItem(int row) {
	pModel->removeItem(row);
}

FileView* lmTransferListView::item(int row) {
	return pModel->item(row);
}

FileView* lmTransferListView::item(QString id) {
	return pModel->item(id);
}

FileView* lmTransferListView::item(QString id, FileView::TransferMode mode) {
	return pModel->item(id, mode);
}

int lmTransferListView::itemIndex(QString id, FileView::TransferMode mode) {
	return pModel->itemIndex(id, mode);
}

FileView* lmTransferListView::currentItem(void) {
	return pModel->item(currentRow());
}

int lmTransferListView::currentRow(void) {
	QModelIndex index = currentIndex();
	if(!index.isValid())
		return -1;
	if(index.row() >= pModel->rowCount())
		return -1;

	return index.row();
}

int lmTransferListView::count(void) {
	return pModel->rowCount();
}

void lmTransferListView::setCurrentRow(int row) {
	selectionModel()->setCurrentIndex(pModel->index(row), QItemSelectionModel::ClearAndSelect);
}

void lmTransferListView::itemChanged(int row) {
	pModel->itemChanged(row);
}

void lmTransferListView::currentChanged(const QModelIndex& current, const QModelIndex& previous) {
	QListView::currentChanged(current, previous);
	emit currentRowChanged(current.row());
}

void lmTransferListView::loadData(QString filePath) {
	pModel->loadData(filePath);
}

void lmTransferListView::saveData(QString filePath) {
	pModel->saveData(filePath);
}