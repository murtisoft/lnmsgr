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


#ifndef WIDGETUSERTREE_H
#define WIDGETUSERTREE_H

#include <QtWidgets/QApplication>
#include <QtWidgets/QTreeWidget>
#include <QMouseEvent>
#include <QDragMoveEvent>
#include <QString>
#include <QtWidgets/QStyledItemDelegate>
#include <QPainter>
#include <QFontMetrics>
#include "definitionsui.h"

class lmWidgetUserTreeItem : public QTreeWidgetItem {
public:
	lmWidgetUserTreeItem();
	~lmWidgetUserTreeItem() {}

	QRect checkBoxRect(const QRect& itemRect);
};

class lmWidgetUserTreeGroupItem : public lmWidgetUserTreeItem {
public:
	lmWidgetUserTreeGroupItem() : lmWidgetUserTreeItem() {}
	~lmWidgetUserTreeGroupItem() {}

	void addChild(QTreeWidgetItem* child);
};

class lmWidgetUserTreeUserItem : public lmWidgetUserTreeItem {
public:
	lmWidgetUserTreeUserItem() : lmWidgetUserTreeItem() {}
	~lmWidgetUserTreeUserItem() {}

private:
	bool operator < (const QTreeWidgetItem& other) const;
};

class lmWidgetUserTreeDelegate : public QStyledItemDelegate {
public:
	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

private:
	void drawCheckBox(QPainter* painter, const QPalette& palette, const QRect& checkBoxRect, Qt::CheckState checkState) const;
};

class lmWidgetUserTree : public QTreeWidget {
	Q_OBJECT

public:
	lmWidgetUserTree(QWidget* parent);
	~lmWidgetUserTree() {}

	bool checkable(void);
	void setCheckable(bool enable);
	UserListView view(void);
	void setView(UserListView view);

signals:
	void itemDragDropped(QTreeWidgetItem* item);
    void itemContextMenu(QTreeWidgetItem* item, QPoint& pos);

protected:
	void mousePressEvent(QMouseEvent* event);
	void dragMoveEvent(QDragMoveEvent* event);
	void dropEvent(QDropEvent* event);
    void contextMenuEvent(QContextMenuEvent *event);
	void mouseReleaseEvent(QMouseEvent* event);
	void keyPressEvent(QKeyEvent* event);

private:
	lmWidgetUserTreeDelegate* itemDelegate;
	bool dragGroup;
	bool dragUser;
	QString parentId;
	QTreeWidgetItem* dragItem;
	bool expanded;
	bool isCheckable;
	UserListView viewType;
};

#endif // WIDGETUSERTREE_H
