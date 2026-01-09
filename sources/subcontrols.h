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


//	This file contains definitions for subclassed controls

#ifndef SUBCONTROLS_H
#define SUBCONTROLS_H

#include <QStylePainter>
#include <QStyleOption>
#include <QToolButton>
#include <QLabel>
#include <QLineEdit>
#include "definitionsui.h"

class lmToolButton : public QToolButton {
public:
	lmToolButton(QWidget* parent = 0);

protected:
	void paintEvent(QPaintEvent*);
};

class lmLabel : public QLabel {
public:
	lmLabel(QWidget* parent = 0);

	QString text(void) const;
	void setText(const QString& text);

protected:
	void resizeEvent(QResizeEvent*);
	void paintEvent(QPaintEvent*);

private:
	void setElidedText(void);

	QString actualText;
	QString elidedText;
};

class lmLineEdit : public QLineEdit {
	Q_OBJECT

public:
	lmLineEdit(QWidget* parent = 0);

signals:
	void lostFocus(void);

protected:
	void focusInEvent(QFocusEvent* event);
	void focusOutEvent(QFocusEvent* event);
};

#endif //SUBCONTROLS_H
