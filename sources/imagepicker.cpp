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


#include <QMenu>
#include <QHeaderView>
#include <QPainter>
#include <QMouseEvent>
#include <qmath.h>
#include "definitionsui.h"
#include "imagepicker.h"
#include "chathelper.h"

lmImagePicker::lmImagePicker(QWidget *parent, QList<QString>* source, int picSize, int columns, int* selected, int actionIndex)
	: QTableWidget(parent)
{
	setMouseTracking(true);

	setBackgroundRole(QPalette::Window);
	setIconSize(QSize(picSize, picSize));
	setFrameShape(QFrame::NoFrame);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setSelectionMode(QAbstractItemView::NoSelection);
	setShowGrid(false);
	horizontalHeader()->setVisible(false);

	verticalHeader()->setVisible(false);
	setStyleSheet("QTableWidget { padding: 4px }");	// padding around table

	max_col = columns;
	int max_row = qCeil(source->count() / (qreal)max_col);

	setColumnCount(max_col);
	setRowCount(max_row);

    int cellSize = picSize + 8;
	verticalHeader()->setDefaultSectionSize(cellSize);
	verticalHeader()->setMinimumSectionSize(cellSize);
    horizontalHeader()->setMinimumSectionSize(0);           //QT6 requires this in order for the smileys to fit into the widget.
	horizontalHeader()->setDefaultSectionSize(cellSize);
	horizontalHeader()->setMinimumSectionSize(cellSize);

	//	set min and max size of table, with padding included
    setMinimumSize(max_col * cellSize + 8 , max_row * cellSize + 8);
    setMaximumSize(max_col * cellSize + 8 , max_row * cellSize + 8);

	for(int i = 0; i < max_row; i++) {
		for(int j = 0; j < max_col; j++) {
			int k = (i * max_col) + j;

			QTableWidgetItem* item = new QTableWidgetItem();
			item->setData(TypeRole, 0);
			if(k < source->count()) {
                QString val = source->value(k);
                if (val.startsWith(":/")) {
                    item->setIcon(QIcon(val)); //Legacy Embedded PNG (for custom application icon smiley.)
                } else {
                    item->setIcon(ChatHelper::renderEmoji(val, picSize));
                }
				item->setData(TypeRole, 1);
				item->setSizeHint(QSize(picSize, picSize));
				item->setBackground(this->palette().window());
			}
			setItem(i, j, item);
		}
	}

	this->actionIndex = actionIndex;
	this->selected = selected;
	this->hoverItem = NULL;
}

lmImagePicker::~lmImagePicker() {
}

void lmImagePicker::mouseReleaseEvent(QMouseEvent* e) {
    QTableWidgetItem* it = itemAt(e->pos());

    if (it && it->data(TypeRole).toInt() == 1) {
        int index = it->row() * max_col + it->column();
        *selected = index;
        hoverItem = nullptr;

        QMenu* pMenu = qobject_cast<QMenu*>(this->parent());
        if (pMenu) {
            pMenu->actions()[actionIndex]->trigger();
            pMenu->close();
        }
    }

    QTableWidget::mouseReleaseEvent(e);
}

void lmImagePicker::mouseMoveEvent(QMouseEvent* e) {
    QTableWidget::mouseMoveEvent(e);

    QTableWidgetItem* currentItem = itemAt(e->pos());
    if(currentItem != hoverItem) {
        hoverItem = currentItem;
        if(hoverItem)
            update(visualItemRect(hoverItem));
    }
}

void lmImagePicker::paintEvent(QPaintEvent* e) {
	QTableWidget::paintEvent(e);

	//	If mouse is hovered over an item, draw a border around it
	if(hoverItem) {
		QStyleOptionFrame opt;
		opt.rect = visualItemRect(hoverItem);
        opt.rect.adjust(1, 1, -2, -1);
		QPainter painter(viewport());
		style()->drawPrimitive(QStyle::PE_FrameButtonBevel, &opt, &painter);
	}
}

void lmImagePicker::leaveEvent(QEvent* e) {
	QTableWidget::leaveEvent(e);

	hoverItem = NULL;
}

lmImagePickerAction::lmImagePickerAction(QObject* parent, const QString source[], int sourceCount, int picSize, int columns, int* selected)
    : QWidgetAction(parent) {
    this->source = new QList<QString>();
    for(int index = 0; index < sourceCount; index++)
        this->source->append(source[index]);
    this->picSize = picSize;
    this->columns = columns;
    this->selected = selected;
}

lmImagePickerAction::~lmImagePickerAction(void) {
}

void lmImagePickerAction::releaseWidget(QWidget* widget) {
    widget->deleteLater();
}

QWidget* lmImagePickerAction::createWidget(QWidget* parent) {
    QMenu* menu = (QMenu*)parent;
    int index = 0;
    for(; index < menu->actions().count(); index++)
        if(menu->actions()[index] == this)
            break;
    lmImagePicker* widget = new lmImagePicker(parent, source, picSize, columns, selected, index);
    return widget;
}
