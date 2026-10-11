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

#include "zdebuglog.h"
#include "opushandler.h"

lmOpusHandler::lmOpusHandler() {
    pEncoder = nullptr;
    pDecoder = nullptr;
}

lmOpusHandler::~lmOpusHandler() {
    shutdown();
}

bool lmOpusHandler::init(int bitrate) {
    shutdown();

    int err = OPUS_OK;
    pEncoder = opus_encoder_create(SampleRate, Channels, OPUS_APPLICATION_VOIP, &err);
    if(err != OPUS_OK || !pEncoder) {
        lmDebugLog::write("Error: Opus encoder creation failed");
        pEncoder = nullptr;
        return false;
    }
    opus_encoder_ctl(pEncoder, OPUS_SET_BITRATE(bitrate));
    opus_encoder_ctl(pEncoder, OPUS_SET_SIGNAL(OPUS_AUTO));
    opus_encoder_ctl(pEncoder, OPUS_SET_INBAND_FEC(1));
    opus_encoder_ctl(pEncoder, OPUS_SET_PACKET_LOSS_PERC(10));

    pDecoder = opus_decoder_create(SampleRate, Channels, &err);
    if(err != OPUS_OK || !pDecoder) {
        lmDebugLog::write("Error: Opus decoder creation failed");
        pDecoder = nullptr;
        shutdown();
        return false;
    }
    pending.clear();
    return true;
}

void lmOpusHandler::shutdown() {
    if(pEncoder) { opus_encoder_destroy(pEncoder); pEncoder = nullptr; }
    if(pDecoder) { opus_decoder_destroy(pDecoder); pDecoder = nullptr; }
    pending.clear();
}

bool lmOpusHandler::isReady() const {
    return pEncoder && pDecoder;
}

QList<QByteArray> lmOpusHandler::encode(const QByteArray& pcm) {
    QList<QByteArray> packets;
    if(!pEncoder)
        return packets;

    pending.append(pcm);
    // if the capture device stalled and a backlog built up, drop it to keep latency low
    if(pending.size() > FrameBytes * 50)
        pending.clear();

    unsigned char out[MaxPacket];
    while(pending.size() >= FrameBytes) {
        int len = opus_encode(pEncoder, reinterpret_cast<const opus_int16*>(pending.constData()),
                              FrameSamples, out, MaxPacket);
        pending.remove(0, FrameBytes);
        if(len > 0)
            packets.append(QByteArray(reinterpret_cast<char*>(out), len));
        else
            lmDebugLog::write("Warning: Opus encode failed");
    }
    return packets;
}

QByteArray lmOpusHandler::decode(const QByteArray& packet) {
    if(!pDecoder || packet.isEmpty())
        return QByteArray();

    QByteArray pcm(FrameBytes, 0);
    int samples = opus_decode(pDecoder, reinterpret_cast<const unsigned char*>(packet.constData()),
                              packet.size(), reinterpret_cast<opus_int16*>(pcm.data()), FrameSamples, 0);
    if(samples <= 0) {
        lmDebugLog::write("Warning: Opus decode failed");
        return QByteArray();
    }
    pcm.resize(samples * Channels * 2);
    return pcm;
}

QByteArray lmOpusHandler::decodeLost() {
    if(!pDecoder)
        return QByteArray();

    QByteArray pcm(FrameBytes, 0);
    int samples = opus_decode(pDecoder, nullptr, 0, reinterpret_cast<opus_int16*>(pcm.data()), FrameSamples, 0);
    if(samples <= 0)
        return QByteArray();
    pcm.resize(samples * Channels * 2);
    return pcm;
}
