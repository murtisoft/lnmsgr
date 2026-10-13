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

#ifndef WIDGETVIDEOVIEW_H
#define WIDGETVIDEOVIEW_H

#include <QVideoWidget>
#include <QVideoSink>
#include <QVideoFrame>
#include <QVideoFrameFormat>
#include <QImage>
#include <cstring>

//	A QVideoWidget that fills the width it is given and takes its height from the 16:9 (720p) aspect ratio.
class lmWidgetVideoView : public QVideoWidget {
	Q_OBJECT

public:
	explicit lmWidgetVideoView(QWidget* parent = nullptr) : QVideoWidget(parent) {
		QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		policy.setHeightForWidth(true);
		setSizePolicy(policy);
	}

	bool hasHeightForWidth(void) const override { return true; }
	int heightForWidth(int width) const override { return width * 9 / 16; }
	QSize sizeHint(void) const override { return QSize(320, 180); }
	QSize minimumSizeHint(void) const override { return QSize(0, 0); }

	//	Shows one image, QVideoWidget has no still image of its own. A null image clears the view.
	void showImage(const QImage& image) {
		if(image.isNull()) {
			videoSink()->setVideoFrame(QVideoFrame());
			return;
		}
		QImage source = image;
		auto pixelFormat = QVideoFrameFormat::pixelFormatFromImageFormat(source.format());
		if(pixelFormat == QVideoFrameFormat::Format_Invalid) {
			source = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
			pixelFormat = QVideoFrameFormat::pixelFormatFromImageFormat(source.format());
		}
		QVideoFrame frame(QVideoFrameFormat(source.size(), pixelFormat));
		if(!frame.map(QVideoFrame::WriteOnly))
			return;
		const int bytes = qMin(frame.bytesPerLine(0), int(source.bytesPerLine()));
		for(int row = 0; row < source.height(); row++)
			std::memcpy(frame.bits(0) + row * frame.bytesPerLine(0), source.constScanLine(row), bytes);
		frame.unmap();
		videoSink()->setVideoFrame(frame);
	}
	void clear(void) { showImage(QImage()); }
};

#endif // WIDGETVIDEOVIEW_H
