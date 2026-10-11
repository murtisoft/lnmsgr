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

#ifndef OPUSHANDLER_H
#define OPUSHANDLER_H

#include <QByteArray>
#include <QList>
#include <opus.h>

//  Opus codec wrapper: 48 kHz, mono, 16-bit PCM, 20 ms frames (VoIP profile).
class lmOpusHandler {
public:
    static const int SampleRate   = 48000;
    static const int Channels     = 1;
    static const int FrameSamples = 960;                            // 20 ms
    static const int FrameBytes   = FrameSamples * Channels * 2;    // 16-bit samples
    static const int MaxPacket    = 1000;

    lmOpusHandler();
    ~lmOpusHandler();

    bool init(int bitrate = 96000);
    void shutdown();
    bool isReady() const;

    //  Buffers raw PCM and returns one Opus packet per complete 20 ms frame.
    QList<QByteArray> encode(const QByteArray& pcm);
    //  Decodes one Opus packet to PCM (FrameBytes). Empty on error.
    QByteArray decode(const QByteArray& packet);
    //  Packet loss concealment for one missing frame.
    QByteArray decodeLost();

private:
    OpusEncoder* pEncoder;
    OpusDecoder* pDecoder;
    QByteArray pending;
};

#endif // OPUSHANDLER_H
