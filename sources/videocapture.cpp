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

#include <climits>
#include <QGuiApplication>
#include <QMediaDevices>
#include <QPainter>
#include "zdebuglog.h"
#include "definitionsdir.h"
#include "ovpxhandler.h"
#include "videocapture.h"

namespace {
const QString kScreenPrefix = "screen:";
const QString kCameraPrefix = "camera:";
constexpr int kStillAvatarSize = 240;
constexpr int kStillIntervalMs = 200;       // the still image is static, this keeps the stream alive
constexpr int kFrameIntervalMs = 33;        // 30 frames per second at most
}

lmVideoCapture::lmVideoCapture(QObject* parent) : QObject(parent) {
	m_session.setVideoSink(&m_sink);
	connect(&m_sink, &QVideoSink::videoFrameChanged, this, &lmVideoCapture::onVideoFrame);
	m_stillTimer.setInterval(kStillIntervalMs);
	connect(&m_stillTimer, &QTimer::timeout, this, [this]() { emit frameCaptured(m_still); });
}

lmVideoCapture::~lmVideoCapture(void) {
	release();
}

QString lmVideoCapture::screenKey(const QScreen* pScreen) {
	return kScreenPrefix + pScreen->name();
}

QString lmVideoCapture::cameraKey(const QCameraDevice& device) {
	return kCameraPrefix + QString::fromUtf8(device.id());
}

QScreen* lmVideoCapture::screenForKey(const QString& key) {
	if(!key.startsWith(kScreenPrefix))
		return nullptr;
	for(QScreen* pScreen : QGuiApplication::screens())
		if(screenKey(pScreen) == key)
			return pScreen;
	return nullptr;
}

QCameraDevice lmVideoCapture::cameraForKey(const QString& key) {
	if(!key.startsWith(kCameraPrefix))
		return QCameraDevice();
	for(const QCameraDevice& device : QMediaDevices::videoInputs())
		if(cameraKey(device) == key)
			return device;
	return QCameraDevice();
}

QImage lmVideoCapture::stillImage(void) {
	QImage image(lmVpxHandler::Width, lmVpxHandler::Height, QImage::Format_ARGB32_Premultiplied);
	image.fill(Qt::black);
	const QImage avatar(DefinitionsDir::avatarFile());
	if(!avatar.isNull()) {
		const QImage scaled = avatar.scaled(kStillAvatarSize, kStillAvatarSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		QPainter painter(&image);
		painter.drawImage((image.width() - scaled.width()) / 2, (image.height() - scaled.height()) / 2, scaled);
	}
	return image;
}

void lmVideoCapture::start(void) {
	m_running = true;
	m_failed = false;
	apply();
}

void lmVideoCapture::stop(void) {
	m_running = false;
	release();
}

void lmVideoCapture::setSource(const QString& key) {
	if(key == m_key)
		return;
	m_key = key;
	m_failed = false;
	apply();
}

void lmVideoCapture::setMuted(bool muted) {
	if(muted == m_muted)
		return;
	m_muted = muted;
	apply();
}

void lmVideoCapture::refresh(void) {
	if(!m_running)
		return;
	const bool live = m_pCamera || m_pScreenCapture;
	const bool wantLive = !m_muted && (screenForKey(m_key) || !cameraForKey(m_key).isNull());
	if(live == wantLive)
		return;
	m_failed = false;
	apply();
}

//	Opens the chosen source, or falls back to the still image.
void lmVideoCapture::apply(void) {
	release();
	if(!m_running)
		return;

	if(!m_muted && !m_failed) {
		if(QScreen* pScreen = screenForKey(m_key)) {
			m_pScreenCapture = new QScreenCapture(this);
			m_pScreenCapture->setScreen(pScreen);
			connect(m_pScreenCapture, &QScreenCapture::errorOccurred, this, [this]() { fail(); });
			m_session.setScreenCapture(m_pScreenCapture);
			m_pScreenCapture->start();
			return;
		}
		const QCameraDevice device = cameraForKey(m_key);
		if(!device.isNull()) {
			m_pCamera = new QCamera(device, this);
			//	The format closest to 720p, a camera's default can be much bigger than what is sent
			QCameraFormat best;
			int bestScore = INT_MAX;
			for(const QCameraFormat& format : device.videoFormats()) {
				const int score = qAbs(format.resolution().width() - lmVpxHandler::Width)
					+ qAbs(format.resolution().height() - lmVpxHandler::Height)
					+ (format.maxFrameRate() >= 30 ? 0 : 100000);
				if(score < bestScore) {
					bestScore = score;
					best = format;
				}
			}
			if(!best.isNull())
				m_pCamera->setCameraFormat(best);
			connect(m_pCamera, &QCamera::errorOccurred, this, [this]() { fail(); });
			m_session.setCamera(m_pCamera);
			m_pCamera->start();
			return;
		}
		if(!m_key.isEmpty())
			lmDebugLog::write("Warning: selected video source not available, sending the still image");
	}

	m_still = stillImage();
	emit frameCaptured(m_still);
	m_stillTimer.start();
}

void lmVideoCapture::release(void) {
	m_stillTimer.stop();
	if(m_pCamera) {
		m_pCamera->disconnect(this);
		m_pCamera->stop();
		m_session.setCamera(nullptr);
		delete m_pCamera;
		m_pCamera = nullptr;
	}
	if(m_pScreenCapture) {
		m_pScreenCapture->disconnect(this);
		m_pScreenCapture->stop();
		m_session.setScreenCapture(nullptr);
		delete m_pScreenCapture;
		m_pScreenCapture = nullptr;
	}
}

//	The source reported an error, usually because it was unplugged. Carry on with the still image.
void lmVideoCapture::fail(void) {
	if(m_failed)
		return;
	lmDebugLog::write("Warning: video source stopped with an error, sending the still image");
	m_failed = true;
	QMetaObject::invokeMethod(this, [this]() { apply(); }, Qt::QueuedConnection);
}

void lmVideoCapture::onVideoFrame(const QVideoFrame& frame) {
	if(!m_pCamera && !m_pScreenCapture)
		return;
	if(m_clock.isValid() && m_clock.elapsed() < kFrameIntervalMs)
		return;
	const QImage image = frame.toImage();
	if(image.isNull())
		return;
	m_clock.restart();
	emit frameCaptured(image);
}
