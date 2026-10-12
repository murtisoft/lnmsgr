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

#include <QtMath>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QToolTip>
#include "widgetthresholdslider.h"

namespace {
constexpr qreal kMarginX = 7;           // room for half a knob at either end
constexpr qreal kTrackHeight = 6;
constexpr qreal kKnobWidth = 8;
constexpr qreal kKnobOverhang = 5;      // how far the knob sticks out above and below the track
constexpr int   kPageStepDb = 6;
constexpr float kFallDbPerSecond = 40.0f;
const QColor kActiveColor(0x43, 0xa0, 0x47);
const QColor kQuietColor(127, 127, 127);
}

lmWidgetThresholdSlider::lmWidgetThresholdSlider(QWidget* parent) : QSlider(Qt::Horizontal, parent) {
	setRange(MinDb, MaxDb);
	setValue(DefaultDb);
	setSingleStep(1);
	setPageStep(kPageStepDb);
	setMinimumHeight(int(kTrackHeight + 2 * kKnobOverhang + 6));
	connect(this, &QAbstractSlider::valueChanged, this, &lmWidgetThresholdSlider::updateToolTip);
	updateToolTip();
}

void lmWidgetThresholdSlider::setLevel(float dbfs) {
	float elapsed = 0;
	if(m_clock.isValid())
		elapsed = qMin(0.25f, m_clock.restart() / 1000.0f);
	else
		m_clock.start();

	m_level = qMax(dbfs, m_level - kFallDbPerSecond * elapsed);
	m_level = qBound(float(MinDb), m_level, float(MaxDb));
	update();
}

void lmWidgetThresholdSlider::resetLevel(void) {
	m_level = MinDb;
	m_clock.invalidate();
	update();
}

QSize lmWidgetThresholdSlider::sizeHint(void) const {
	return QSize(200, minimumHeight());
}

QSize lmWidgetThresholdSlider::minimumSizeHint(void) const {
	return QSize(120, minimumHeight());
}

QRectF lmWidgetThresholdSlider::trackRect(void) const {
	const qreal top = (height() - kTrackHeight) / 2.0;
	return QRectF(kMarginX, top, width() - 2 * kMarginX, kTrackHeight);
}

qreal lmWidgetThresholdSlider::dbToX(double db) const {
	const QRectF track = trackRect();
	const double fraction = (db - MinDb) / double(MaxDb - MinDb);
	return track.left() + qBound(0.0, fraction, 1.0) * track.width();
}

void lmWidgetThresholdSlider::paintEvent(QPaintEvent*) {
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	const QPalette::ColorGroup group = isEnabled() ? QPalette::Active : QPalette::Disabled;
	auto color = [&](QPalette::ColorRole role) { return palette().color(group, role); };

	const QRectF track = trackRect();
	const qreal radius = track.height() / 2.0;

	//	groove
	painter.setPen(QPen(color(QPalette::Mid), 1));
	painter.setBrush(color(QPalette::Base));
	painter.drawRoundedRect(track, radius, radius);

	//	level bar, green while it is loud enough to pass the threshold, grey otherwise
	const qreal levelX = dbToX(m_level);
	if(levelX > track.left() + 1) {
		QPainterPath clip;
		clip.addRoundedRect(track.adjusted(1, 1, -1, -1), radius - 1, radius - 1);
		painter.save();
		painter.setClipPath(clip);
		painter.setPen(Qt::NoPen);
		const bool passing = m_level >= value();
		painter.setBrush(!isEnabled() ? color(QPalette::Mid) : (passing ? kActiveColor : kQuietColor));
		painter.drawRect(QRectF(track.left(), track.top(), levelX - track.left(), track.height()));
		painter.restore();
	}

	//	knob
	const qreal knobX = dbToX(value());
	const QRectF knob(knobX - kKnobWidth / 2.0, track.top() - kKnobOverhang, kKnobWidth, track.height() + 2 * kKnobOverhang);
	painter.setPen(QPen(hasFocus() && isEnabled() ? color(QPalette::Highlight) : color(QPalette::Window), hasFocus() ? 2 : 1));
	painter.setBrush(color(QPalette::WindowText));
	painter.drawRoundedRect(knob, 3, 3);
}

void lmWidgetThresholdSlider::mousePressEvent(QMouseEvent* pEvent) {
	if(pEvent->button() != Qt::LeftButton) {
		pEvent->ignore();
		return;
	}
	setFocus(Qt::MouseFocusReason);
	setSliderDown(true);
	setValueFromPosition(pEvent->position().x());
	pEvent->accept();
}

void lmWidgetThresholdSlider::mouseMoveEvent(QMouseEvent* pEvent) {
	if(!isSliderDown()) {
		pEvent->ignore();
		return;
	}
	setValueFromPosition(pEvent->position().x());
	pEvent->accept();
}

void lmWidgetThresholdSlider::mouseReleaseEvent(QMouseEvent* pEvent) {
	if(pEvent->button() != Qt::LeftButton || !isSliderDown()) {
		pEvent->ignore();
		return;
	}
	setSliderDown(false);
	pEvent->accept();
}

//	Anywhere on the bar is a valid target, the knob jumps there.
void lmWidgetThresholdSlider::setValueFromPosition(qreal x) {
	const QRectF track = trackRect();
	const double fraction = qBound(0.0, (x - track.left()) / track.width(), 1.0);
	setValue(qRound(MinDb + fraction * (MaxDb - MinDb)));
	if(isSliderDown())
		QToolTip::showText(QCursor::pos(), toolTip(), this);
}

void lmWidgetThresholdSlider::updateToolTip(void) {
	setToolTip(tr("%1 dB").arg(value()));
}
