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

#include <QPainter>
#include <QThread>
#include <vp8cx.h>
#include <vp8dx.h>
#include "zdebuglog.h"
#include "ovpxhandler.h"

namespace {
inline int clampByte(int value) {
    return value < 0 ? 0 : (value > 255 ? 255 : value);
}
}

lmVpxHandler::lmVpxHandler() {
    encoderReady = false;
    decoderReady = false;
    yuvReady = false;
    forceKeyframe = true;
    fps = DefaultFps;
    pts = 0;
}

lmVpxHandler::~lmVpxHandler() {
    shutdown();
}

bool lmVpxHandler::init(int bitrateKbps, int fpsValue) {
    shutdown();
    fps = fpsValue;
    const int threads = qBound(1, QThread::idealThreadCount(), 4);

    vpx_codec_enc_cfg_t cfg;
    if(vpx_codec_enc_config_default(vpx_codec_vp8_cx(), &cfg, 0) != VPX_CODEC_OK) {
        lmDebugLog::write("Error: VP8 encoder configuration failed");
        return false;
    }
    cfg.g_w = Width;
    cfg.g_h = Height;
    cfg.g_timebase.num = 1;
    cfg.g_timebase.den = fps;
    cfg.g_threads = threads;
    cfg.g_lag_in_frames = 0;                // no look ahead, a frame comes out for every frame that goes in
    cfg.g_error_resilient = 0;              // reliable transport
    cfg.rc_end_usage = VPX_CBR;
    cfg.rc_target_bitrate = bitrateKbps;
    cfg.rc_min_quantizer = 4;
    cfg.rc_max_quantizer = 56;
    cfg.rc_undershoot_pct = 100;
    cfg.rc_overshoot_pct = 15;
    cfg.rc_buf_sz = 1000;
    cfg.rc_buf_initial_sz = 600;
    cfg.rc_buf_optimal_sz = 600;
    cfg.kf_mode = VPX_KF_AUTO;
    cfg.kf_min_dist = 0;
    cfg.kf_max_dist = fps * 5;              // a keyframe every 5 seconds, more on request, see requestKeyframe()

    if(vpx_codec_enc_init(&encoder, vpx_codec_vp8_cx(), &cfg, 0) != VPX_CODEC_OK) {
        lmDebugLog::write("Error: VP8 encoder creation failed");
        return false;
    }
    encoderReady = true;
    vpx_codec_control(&encoder, VP8E_SET_CPUUSED, -6);      // real time speed
    vpx_codec_control(&encoder, VP8E_SET_NOISE_SENSITIVITY, 0);
    vpx_codec_control(&encoder, VP8E_SET_STATIC_THRESHOLD, 1);

    if(!vpx_img_alloc(&yuv, VPX_IMG_FMT_I420, Width, Height, 1)) {
        lmDebugLog::write("Error: VP8 frame buffer allocation failed");
        shutdown();
        return false;
    }
    yuvReady = true;

    vpx_codec_dec_cfg_t decCfg = {};
    decCfg.threads = threads;
    decCfg.w = Width;
    decCfg.h = Height;
    if(vpx_codec_dec_init(&decoder, vpx_codec_vp8_dx(), &decCfg, 0) != VPX_CODEC_OK) {
        lmDebugLog::write("Error: VP8 decoder creation failed");
        shutdown();
        return false;
    }
    decoderReady = true;

    forceKeyframe = true;
    pts = 0;
    return true;
}

void lmVpxHandler::shutdown() {
    if(encoderReady) { vpx_codec_destroy(&encoder); encoderReady = false; }
    if(decoderReady) { vpx_codec_destroy(&decoder); decoderReady = false; }
    if(yuvReady) { vpx_img_free(&yuv); yuvReady = false; }
}

bool lmVpxHandler::isReady() const {
    return encoderReady && decoderReady;
}

QByteArray lmVpxHandler::encode(const QImage& image) {
    if(!encoderReady || image.isNull())
        return QByteArray();

    //  Make it a 1280x720 RGB32 image, letterboxed when the source has another shape
    QImage frame;
    if(image.size() == QSize(Width, Height)) {
        frame = image.convertToFormat(QImage::Format_RGB32);
    } else {
        frame = QImage(Width, Height, QImage::Format_RGB32);
        frame.fill(Qt::black);
        const QImage scaled = image.scaled(Width, Height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QPainter painter(&frame);
        painter.drawImage((Width - scaled.width()) / 2, (Height - scaled.height()) / 2, scaled);
    }

    //  RGB to I420 (BT.601, limited range), chroma averaged over 2x2 pixels
    for(int y = 0; y < Height; y += 2) {
        const QRgb* row0 = reinterpret_cast<const QRgb*>(frame.constScanLine(y));
        const QRgb* row1 = reinterpret_cast<const QRgb*>(frame.constScanLine(y + 1));
        uint8_t* yRow0 = yuv.planes[VPX_PLANE_Y] + y * yuv.stride[VPX_PLANE_Y];
        uint8_t* yRow1 = yRow0 + yuv.stride[VPX_PLANE_Y];
        uint8_t* uRow = yuv.planes[VPX_PLANE_U] + (y / 2) * yuv.stride[VPX_PLANE_U];
        uint8_t* vRow = yuv.planes[VPX_PLANE_V] + (y / 2) * yuv.stride[VPX_PLANE_V];
        for(int x = 0; x < Width; x += 2) {
            const QRgb p[4] = { row0[x], row0[x + 1], row1[x], row1[x + 1] };
            int sumR = 0, sumG = 0, sumB = 0;
            for(int i = 0; i < 4; i++) {
                const int r = qRed(p[i]), g = qGreen(p[i]), b = qBlue(p[i]);
                sumR += r; sumG += g; sumB += b;
                const uint8_t luma = ((66 * r + 129 * g + 25 * b + 128) >> 8) + 16;
                if(i == 0) yRow0[x] = luma;
                else if(i == 1) yRow0[x + 1] = luma;
                else if(i == 2) yRow1[x] = luma;
                else yRow1[x + 1] = luma;
            }
            const int r = sumR / 4, g = sumG / 4, b = sumB / 4;
            uRow[x / 2] = ((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128;
            vRow[x / 2] = ((112 * r - 94 * g - 18 * b + 128) >> 8) + 128;
        }
    }

    vpx_codec_flags_t flags = 0;
    if(forceKeyframe) {
        flags |= VPX_EFLAG_FORCE_KF;
        forceKeyframe = false;
    }
    if(vpx_codec_encode(&encoder, &yuv, pts++, 1, flags, VPX_DL_REALTIME) != VPX_CODEC_OK) {
        lmDebugLog::write("Warning: VP8 encode failed");
        forceKeyframe = true;
        return QByteArray();
    }

    QByteArray out;
    vpx_codec_iter_t iter = nullptr;
    while(const vpx_codec_cx_pkt_t* packet = vpx_codec_get_cx_data(&encoder, &iter)) {
        if(packet->kind == VPX_CODEC_CX_FRAME_PKT)
            out.append(static_cast<const char*>(packet->data.frame.buf), int(packet->data.frame.sz));
    }
    return out;
}

QImage lmVpxHandler::decode(const QByteArray& data) {
    if(!decoderReady || data.isEmpty())
        return QImage();

    if(vpx_codec_decode(&decoder, reinterpret_cast<const uint8_t*>(data.constData()), data.size(), nullptr, 0) != VPX_CODEC_OK) {
        lmDebugLog::write("Warning: VP8 decode failed");
        return QImage();
    }
    vpx_codec_iter_t iter = nullptr;
    const vpx_image_t* img = vpx_codec_get_frame(&decoder, &iter);
    if(!img || img->fmt != VPX_IMG_FMT_I420)
        return QImage();

    //  I420 to RGB (BT.601, limited range)
    QImage image(img->d_w, img->d_h, QImage::Format_RGB32);
    for(unsigned int y = 0; y < img->d_h; y++) {
        const uint8_t* yRow = img->planes[VPX_PLANE_Y] + y * img->stride[VPX_PLANE_Y];
        const uint8_t* uRow = img->planes[VPX_PLANE_U] + (y / 2) * img->stride[VPX_PLANE_U];
        const uint8_t* vRow = img->planes[VPX_PLANE_V] + (y / 2) * img->stride[VPX_PLANE_V];
        QRgb* out = reinterpret_cast<QRgb*>(image.scanLine(y));
        for(unsigned int x = 0; x < img->d_w; x++) {
            const int c = 298 * (yRow[x] - 16) + 128;
            const int d = uRow[x / 2] - 128;
            const int e = vRow[x / 2] - 128;
            out[x] = qRgb(clampByte((c + 409 * e) >> 8),
                          clampByte((c - 100 * d - 208 * e) >> 8),
                          clampByte((c + 516 * d) >> 8));
        }
    }
    return image;
}
