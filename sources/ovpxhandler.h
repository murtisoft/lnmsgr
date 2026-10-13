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

#ifndef OVPXHANDLER_H
#define OVPXHANDLER_H

#include <QByteArray>
#include <QImage>
#include <vpx_encoder.h>
#include <vpx_decoder.h>

//  VP8 codec wrapper: fixed 1280x720, constant bitrate, real time, no lag. Meant for a reliable (TCP) stream on
//  a LAN, so there is no loss recovery: a keyframe every 5 seconds, more on request.
//  Images go in and come out as QImage, the I420 conversion is done in here.
class lmVpxHandler {
public:
    static const int Width        = 1280;
    static const int Height       = 720;
    static const int DefaultFps   = 30;
    static const int DefaultKbps  = 2000;

    lmVpxHandler();
    ~lmVpxHandler();

    bool init(int bitrateKbps = DefaultKbps, int fps = DefaultFps);
    void shutdown();
    bool isReady() const;
    //  The next encoded frame will be a keyframe, e.g. after the video source was switched.
    void requestKeyframe() { forceKeyframe = true; }

    //  Encodes one image. Anything but 1280x720 is scaled to fit and centered on black. Empty on error.
    QByteArray encode(const QImage& image);
    //  Decodes one encoded frame. A null image on error.
    QImage decode(const QByteArray& frame);

private:
    vpx_codec_ctx_t encoder;
    vpx_codec_ctx_t decoder;
    vpx_image_t     yuv;
    bool encoderReady;
    bool decoderReady;
    bool yuvReady;
    bool forceKeyframe;
    int  fps;
    vpx_codec_pts_t pts;
};

#endif // OVPXHANDLER_H
