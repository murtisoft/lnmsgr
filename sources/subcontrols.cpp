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


#include <QApplication>
#include "strings.h"
#include "subcontrols.h"

lmToolButton::lmToolButton(QWidget* parent) : QToolButton(parent) {
}

void lmToolButton::paintEvent(QPaintEvent*) {
	QStylePainter p(this);
	QStyleOptionToolButton opt;
	initStyleOption(&opt);
	opt.features &= (~QStyleOptionToolButton::HasMenu);
	p.drawComplexControl(QStyle::CC_ToolButton, opt);
}

lmLabel::lmLabel(QWidget* parent) : QLabel(parent) {
	actualText = elidedText = QString();
}

QString lmLabel::text(void) const {
	return actualText;
}

void lmLabel::setText(const QString& text) {
	actualText = text;
	setElidedText();
}

void lmLabel::resizeEvent(QResizeEvent*) {
	setElidedText();
}

void lmLabel::paintEvent(QPaintEvent*) {
	QStylePainter p(this);
	p.drawText(rect(), alignment(), elidedText);
}

void lmLabel::setElidedText(void) {
	elidedText = fontMetrics().elidedText(actualText, Qt::ElideRight, width());
}

lmLineEdit::lmLineEdit(QWidget* parent) : QLineEdit(parent) {
	QPalette p = QApplication::palette();
	p.setColor(QPalette::Base, p.color(QPalette::Window));
    p.setColor(QPalette::Text, GRAY_TEXT_COLOR);
	setPalette(p);
}

void lmLineEdit::focusInEvent(QFocusEvent* event) {
	QLineEdit::focusInEvent(event);

	setSelection(0, text().length());

	QPalette p = QApplication::palette();
	p.setColor(QPalette::Base, p.color(QPalette::Base));
	p.setColor(QPalette::Text, p.color(QPalette::Text));
	setPalette(p);
}

void lmLineEdit::focusOutEvent(QFocusEvent* event) {
	QLineEdit::focusOutEvent(event);

	QPalette p = QApplication::palette();
	p.setColor(QPalette::Base, p.color(QPalette::Window));
    p.setColor(QPalette::Text, GRAY_TEXT_COLOR);
	setPalette(p);

	emit lostFocus();
}
