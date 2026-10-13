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

#ifndef VIDEOCAPTURE_H
#define VIDEOCAPTURE_H

#include <QObject>
#include <QImage>
#include <QTimer>
#include <QElapsedTimer>
#include <QScreen>
#include <QCamera>
#include <QCameraDevice>
#include <QScreenCapture>
#include <QMediaCaptureSession>
#include <QVideoSink>
#include <QVideoFrame>

//	The outgoing video of a call: the still image, a desktop or a camera, as chosen in the preferences
//	(IDS_CAMDEVICE). Delivers frames of at most 30 per second. A source that is not connected, or that fails,
//	is replaced by the still image. Lives on the main thread.
class lmVideoCapture : public QObject {
	Q_OBJECT

public:
	explicit lmVideoCapture(QObject* parent = nullptr);
	~lmVideoCapture(void);

	//	The format of the IDS_CAMDEVICE setting: empty is the still image, "screen:<QScreen::name()>" is a desktop
	//	and "camera:<QCameraDevice::id()>" is a camera.
	static QString screenKey(const QScreen* pScreen);
	static QString cameraKey(const QCameraDevice& device);
	static QScreen* screenForKey(const QString& key);            // nullptr if not a desktop or not connected
	static QCameraDevice cameraForKey(const QString& key);       // null if not a camera or not connected
	//	The local avatar centered on a black 720p frame.
	static QImage stillImage(void);

	void start(void);
	void stop(void);
	//	Switches the source, also during a call.
	void setSource(const QString& key);
	//	While muted the camera or desktop is released and the still image is sent instead.
	void setMuted(bool muted);
	//	The lists of cameras or desktops changed: a source in use may be gone, or a missing one may be back.
	void refresh(void);

signals:
	void frameCaptured(const QImage& image);

private:
	void apply(void);
	void release(void);
	void fail(void);
	void onVideoFrame(const QVideoFrame& frame);

	QMediaCaptureSession m_session;
	QVideoSink m_sink;
	QCamera* m_pCamera = nullptr;
	QScreenCapture* m_pScreenCapture = nullptr;
	QTimer m_stillTimer;
	QImage m_still;
	QElapsedTimer m_clock;
	QString m_key;
	bool m_running = false;
	bool m_muted = false;
	bool m_failed = false;      // the source stopped with an error, stay on the still image until it is chosen again
};

#endif // VIDEOCAPTURE_H
