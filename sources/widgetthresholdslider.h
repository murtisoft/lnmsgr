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

#ifndef WIDGETTHRESHOLDSLIDER_H
#define WIDGETTHRESHOLDSLIDER_H

#include <QSlider>
#include <QElapsedTimer>
#include "audiolevel.h"

//	A threshold selector with a level meter built in. The knob is the threshold, the painted bar behind it
//	is the live input level, so you can see at a glance where the knob has to go. Both are in dBFS.
//	It is a QSlider underneath (value, range, keyboard, valueChanged), only drawn and clicked differently.
class lmWidgetThresholdSlider : public QSlider {
	Q_OBJECT

public:
	static constexpr int MinDb = -60;
	static constexpr int MaxDb = 0;
	static constexpr int DefaultDb = -40;

	explicit lmWidgetThresholdSlider(QWidget* parent = nullptr);

	//	Current input level in dBFS. Smoothed here: rises at once, falls gradually.
	void setLevel(float dbfs);
	void resetLevel(void);
	//	Without a threshold (any activation mode but voice activity) the knob is hidden and cannot be moved,
	//	only the level bar remains.
	void setThresholdVisible(bool visible);
	//	Push to talk: the bar is green only while the key is held (setTriggered), grey otherwise, instead of
	//	following the level against a threshold. The level is still drawn.
	void setExternalTrigger(bool used);
	void setTriggered(bool triggered);

	QSize sizeHint(void) const override;
	QSize minimumSizeHint(void) const override;

protected:
	void paintEvent(QPaintEvent* pEvent) override;
	void mousePressEvent(QMouseEvent* pEvent) override;
	void mouseMoveEvent(QMouseEvent* pEvent) override;
	void mouseReleaseEvent(QMouseEvent* pEvent) override;
	void wheelEvent(QWheelEvent* pEvent) override;

private:
	QRectF trackRect(void) const;
	qreal dbToX(double db) const;
	void setValueFromPosition(qreal x);
	void updateToolTip(void);

	float m_level = MinDb;
	bool m_thresholdVisible = true;
	bool m_externalTrigger = false;
	bool m_triggered = false;
	QElapsedTimer m_clock;
};

#endif // WIDGETTHRESHOLDSLIDER_H
