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


#include <QDir>
#include <QFileInfo>
#include <QDataStream>
#include "zdebuglog.h"
#include "networkstreamer.h"
#include "networkdatagram.h"
#include "audiolevel.h"
#include "definitionssettings.h"
#include <QtEndian>
#include <openssl/rand.h>

lmAudioStream::lmAudioStream(QObject* parent) : QObject(nullptr) {
    Q_UNUSED(parent);   // an object with a parent cannot be moved to another thread
    m_thread = new QThread;
    moveToThread(m_thread);
    m_thread->start(QThread::HighPriority);
}

lmAudioStream::~lmAudioStream() {
    stop();
    m_thread->quit();
    m_thread->wait();
    delete m_thread;
}

//  Called from the main thread. The session key is read here, since the crypto object belongs to it.
void lmAudioStream::start(const QHostAddress& peerAddress, quint16 udpPort, lmOpenSSL* crypto, const QString& peerId) {
    QByteArray key = crypto ? crypto->sessionKey(peerId) : QByteArray();
    QMetaObject::invokeMethod(this, [this, peerAddress, udpPort, key]() {
        doStart(peerAddress, udpPort, key);
    }, Qt::QueuedConnection);
}

//  Blocks until the audio thread has released the devices and sockets.
void lmAudioStream::stop() {
    if(QThread::currentThread() == m_thread)
        doStop();
    else
        QMetaObject::invokeMethod(this, [this]() { doStop(); }, Qt::BlockingQueuedConnection);
}

void lmAudioStream::setMicDevice(const QByteArray& deviceId) {
    QMetaObject::invokeMethod(this, [this, deviceId]() {
        if(deviceId == m_micDeviceId)
            return;
        m_micDeviceId = deviceId;
        if(m_sendSock)          // a call is running, switch now
            startInput();
    }, Qt::QueuedConnection);
}

void lmAudioStream::refreshMicDevice() {
    QMetaObject::invokeMethod(this, [this]() {
        if(!m_sendSock)
            return;
        bool fallback = false;
        const QAudioDevice device = resolveInputDevice(&fallback);
        if(device.isNull()) {
            if(m_source)
                startInput();   // everything was unplugged, this releases the dead source and reports it
            return;
        }
        //  Still the right device and still capturing: nothing to do. Otherwise the device we use is gone,
        //  or the one the user picked is back, or the system default changed.
        if(m_source && device.id() == m_inputId && m_source->state() != QAudio::StoppedState)
            return;
        startInput();
    }, Qt::QueuedConnection);
}

//  The device the user picked, or the default one when that is not available. Null if there is no input at all.
QAudioDevice lmAudioStream::resolveInputDevice(bool* pFallback) const {
    *pFallback = false;
    if(!m_micDeviceId.isEmpty()) {
        const QList<QAudioDevice> inputs = QMediaDevices::audioInputs();
        for(const QAudioDevice& device : inputs)
            if(device.id() == m_micDeviceId)
                return device;
        *pFallback = true;
    }
    return QMediaDevices::defaultAudioInput();
}

void lmAudioStream::stopInput() {
    if(m_source) {
        m_source->disconnect(this);
        m_source->stop();
        delete m_source;
        m_source = nullptr;
    }
    m_input = nullptr;
    m_inputId.clear();
}

//  (Re)starts capturing. Keeps the mute state, so switching microphones mid-call doesn't unmute anyone.
void lmAudioStream::startInput() {
    stopInput();

    bool fallback = false;
    const QAudioDevice device = resolveInputDevice(&fallback);
    if(device.isNull()) {
        lmDebugLog::write("Warning: no audio input device available");
        reportInput(QString(), true);
        return;
    }
    if(!device.isFormatSupported(m_format))
        lmDebugLog::write("Warning: audio input '" + device.description() + "' does not list the voice format as supported");

    m_source = new QAudioSource(device, m_format, this);
    connect(m_source, &QAudioSource::stateChanged, this, &lmAudioStream::onMicStateChanged);
    m_input = m_source->start();
    m_inputId = device.id();
    if(m_input)
        connect(m_input, &QIODevice::readyRead, this, &lmAudioStream::onMicReadyRead);
    if(fallback)
        lmDebugLog::write("Warning: selected microphone not available, using '" + device.description() + "'");
    reportInput(device.description(), fallback);
}

//  Tell the user only once per situation, not on every restart.
void lmAudioStream::reportInput(const QString& name, bool fallback) {
    if(fallback && (!m_fallbackReported || name != m_fallbackName))
        emit micDeviceChanged(name, true);
    m_fallbackReported = fallback;
    m_fallbackName = fallback ? name : QString();
}

//  A capture that stopped by itself with an error usually means the device was unplugged.
void lmAudioStream::onMicStateChanged(QAudio::State state) {
    if(state != QAudio::StoppedState || !m_source || m_source->error() == QAudio::NoError)
        return;
    lmDebugLog::write("Warning: microphone capture stopped with an error");
    if(++m_inputRestarts > 3)       // reset as soon as audio flows again, this only stops a restart loop
        return;
    QMetaObject::invokeMethod(this, [this]() {
        if(m_sendSock)
            startInput();
    }, Qt::QueuedConnection);
}

//  Voice packet: "AUDIOO" | salt(8) | seq(4, big endian) | AES-256-GCM(opus frame) + tag(16)
//  salt+seq are authenticated as additional data.
static const int VOICE_HDR = 6;
static const int VOICE_SALT = 8;
static const int VOICE_SEQ = 4;
static const int VOICE_TAG = 16;

void lmAudioStream::doStart(const QHostAddress& peerAddress, quint16 udpPort, const QByteArray& sessionKey) {
    doStop();

    m_peerAddress = peerAddress;
    m_port = udpPort + AUDIO_PORT_OFFSET;
    m_sessionKey = sessionKey;

    if(m_sessionKey.isEmpty() || !m_opus.init()) {
        lmDebugLog::write("Error: Voice call could not start (no session key or opus unavailable)");
        return;
    }

    m_txSalt.resize(VOICE_SALT);
    RAND_bytes((unsigned char*)m_txSalt.data(), VOICE_SALT);
    m_txKey = lmOpenSSL::deriveVoiceKey(m_sessionKey, m_txSalt);
    if(m_txKey.isEmpty()) {
        lmDebugLog::write("Error: Voice call could not start (no session key)");
        m_opus.shutdown();
        return;
    }
    m_txSeq = 0;
    m_rxSalt.clear();
    m_rxKey.clear();
    m_rxHasSeq = false;

    QAudioFormat fmt;
    fmt.setSampleRate(lmOpusHandler::SampleRate);
    fmt.setChannelCount(lmOpusHandler::Channels);
    fmt.setSampleFormat(QAudioFormat::Int16);

    // Record from the selected mic (default if none picked), encode, encrypt, send over UDP
    m_format = fmt;
    m_inputRestarts = 0;
    m_sendSock = new QUdpSocket(this);
    startInput();

    if(m_callSounds)
        emit micMuted(false);

    // Receive UDP, decrypt, decode, play to default speaker
    m_recvSock = new QUdpSocket(this);
    m_recvSock->bind(QHostAddress::AnyIPv4, m_port,
                     QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);

    m_sink   = new QAudioSink(QMediaDevices::defaultAudioOutput(), fmt, this);
    m_output = m_sink->start();

    connect(m_recvSock, &QUdpSocket::readyRead, this, &lmAudioStream::onNetReadyRead);
}

void lmAudioStream::setMicActivation(int mode, int thresholdDb) {
    QMetaObject::invokeMethod(this, [this, mode, thresholdDb]() {
        const bool changed = (mode != m_activation);
        m_activation = mode;
        m_thresholdDb = thresholdDb;
        if(changed)
            resetGate();
    }, Qt::QueuedConnection);
}

void lmAudioStream::setCallSounds(bool enabled) {
    QMetaObject::invokeMethod(this, [this, enabled]() {
        m_callSounds = enabled;
    }, Qt::QueuedConnection);
}

void lmAudioStream::setPushToTalk(bool down) {
    QMetaObject::invokeMethod(this, [this, down]() {
        m_pttDown = down;
    }, Qt::QueuedConnection);
}

void lmAudioStream::resetGate() {
    m_pttDown = false;
    m_gateOpen = false;
    m_holdFrames = 0;
    m_gateBuf.clear();
    m_preroll.clear();
    m_opus.discardPending();
}

//  Muted talk detection, in 20 ms frames.
static const int MUTED_TALK_FRAMES = 50;            // about a second of speech (pauses count down) before the reminder
static const int MUTED_TALK_COOLDOWN_MS = 5000;     // and not more often than this
static const int MUTED_TALK_DEFAULT_DB = IDS_MICTHRESHOLD_VAL;  // the user's threshold only exists for voice activation

void lmAudioStream::processMutedAudio(const QByteArray& pcm) {
    //  Continuous and push to talk have no threshold setting, so they use the default one.
    const int thresholdDb = (m_activation == MA_VoiceActivity) ? m_thresholdDb : MUTED_TALK_DEFAULT_DB;
    m_muteBuf.append(pcm);
    if(m_muteBuf.size() > lmOpusHandler::FrameBytes * 50)
        m_muteBuf.clear();
    while(m_muteBuf.size() >= lmOpusHandler::FrameBytes) {
        const QByteArray frame = m_muteBuf.left(lmOpusHandler::FrameBytes);
        m_muteBuf.remove(0, lmOpusHandler::FrameBytes);

        if(lmAudioLevel::rmsDbfs(frame, QAudioFormat::Int16) >= thresholdDb)
            m_mutedTalkFrames++;
        else if(m_mutedTalkFrames > 0)
            m_mutedTalkFrames--;

        if(m_mutedTalkFrames >= MUTED_TALK_FRAMES) {
            m_mutedTalkFrames = 0;
            if(!m_mutedTalkNotified || m_mutedTalkTimer.hasExpired(MUTED_TALK_COOLDOWN_MS)) {
                m_mutedTalkNotified = true;
                m_mutedTalkTimer.restart();
                emit mutedTalk();
            }
        }
    }
}

void lmAudioStream::onMicReadyRead() {
    if(!m_input || !m_sendSock)
        return;

    QByteArray pcm = m_input->readAll();
    if(pcm.isEmpty())
        return;
    m_inputRestarts = 0;

    //  Muted: the capture keeps running only to notice talking, nothing is encoded or sent.
    if(m_micMuted) {
        processMutedAudio(pcm);
        return;
    }

    if(m_activation == MA_Continuous) {
        sendPackets(m_opus.encode(pcm));
        return;
    }

    //  Push to talk and voice activation: decide per 20 ms frame whether it is sent.
    m_gateBuf.append(pcm);
    if(m_gateBuf.size() > lmOpusHandler::FrameBytes * 50)     // capture stalled and piled up, drop it
        m_gateBuf.clear();
    while(m_gateBuf.size() >= lmOpusHandler::FrameBytes) {
        const QByteArray frame = m_gateBuf.left(lmOpusHandler::FrameBytes);
        m_gateBuf.remove(0, lmOpusHandler::FrameBytes);
        processGatedFrame(frame);
    }
}

//  Gate timing, in 20 ms frames.
static const int VAD_HOLD_FRAMES = 12;      // keep sending 240 ms after the voice dropped below the threshold
static const int PTT_RELEASE_FRAMES = MIC_PTT_RELEASE_MS / 20;     // and a little after the key was let go
static const int PREROLL_FRAMES = 5;        // 100 ms of lead-in for voice activation, the first syllable is quieter than the threshold

void lmAudioStream::processGatedFrame(const QByteArray& frame) {
    bool open = false;
    if(m_activation == MA_PushToTalk) {
        if(m_pttDown) {
            m_holdFrames = PTT_RELEASE_FRAMES;
            open = true;
        } else if(m_holdFrames > 0) {
            m_holdFrames--;
            open = true;
        }
    } else {    // MA_VoiceActivity
        if(lmAudioLevel::rmsDbfs(frame, QAudioFormat::Int16) >= m_thresholdDb) {
            m_holdFrames = VAD_HOLD_FRAMES;
            open = true;
        } else if(m_holdFrames > 0) {
            m_holdFrames--;
            open = true;
        }
    }

    if(!open) {
        m_gateOpen = false;
        if(m_activation == MA_VoiceActivity) {
            m_preroll.append(frame);
            while(m_preroll.size() > PREROLL_FRAMES)
                m_preroll.removeFirst();
        }
        return;
    }

    QList<QByteArray> packets;
    if(!m_gateOpen) {
        //  Speech starts: begin with the lead-in. Push to talk has none, you only transmit what you say after pressing.
        for(const QByteArray& quiet : std::as_const(m_preroll))
            packets += m_opus.encode(quiet);
        m_preroll.clear();
        m_gateOpen = true;
    }
    packets += m_opus.encode(frame);
    sendPackets(packets);
}

void lmAudioStream::sendPackets(const QList<QByteArray>& frames) {
    for(const QByteArray& frame : frames) {
        quint32 seq = m_txSeq++;
        QByteArray seqBytes(VOICE_SEQ, 0);
        qToBigEndian<quint32>(seq, seqBytes.data());

        QByteArray aad = m_txSalt + seqBytes;
        QByteArray cipher = lmOpenSSL::sealPacket(m_txKey, seq, aad, frame);
        if(cipher.isEmpty())
            continue;

        QByteArray dg = DatagramTypeNames[DT_Audio].toLocal8Bit() + aad + cipher;
        m_sendSock->writeDatagram(dg, m_peerAddress, m_port);
    }
}

void lmAudioStream::onNetReadyRead() {
    if(!m_recvSock)
        return;

    const QByteArray hdr = DatagramTypeNames[DT_Audio].toLocal8Bit();

    while(m_recvSock->hasPendingDatagrams()) {
        QByteArray dg;
        dg.resize(m_recvSock->pendingDatagramSize());
        m_recvSock->readDatagram(dg.data(), dg.size());

        if(dg.size() <= VOICE_HDR + VOICE_SALT + VOICE_SEQ + VOICE_TAG || !dg.startsWith(hdr))
            continue;

        QByteArray salt = dg.mid(VOICE_HDR, VOICE_SALT);
        quint32 seq = qFromBigEndian<quint32>(dg.constData() + VOICE_HDR + VOICE_SALT);
        QByteArray aad = dg.mid(VOICE_HDR, VOICE_SALT + VOICE_SEQ);
        QByteArray cipher = dg.mid(VOICE_HDR + VOICE_SALT + VOICE_SEQ);

        bool newStream = (salt != m_rxSalt);
        if(!newStream && m_rxHasSeq && seq <= m_rxLastSeq)
            continue;   // duplicate, replayed or late packet

        QByteArray key = newStream ? lmOpenSSL::deriveVoiceKey(m_sessionKey, salt) : m_rxKey;
        if(key.isEmpty())
            continue;

        QByteArray frame = lmOpenSSL::openPacket(key, seq, aad, cipher);
        if(frame.isEmpty())
            continue;   // failed authentication, drop

        if(newStream) {
            m_rxSalt = salt;
            m_rxKey = key;
            m_rxHasSeq = false;
        }

        // conceal a few lost frames to keep timing smooth
        if(m_rxHasSeq) {
            quint32 lost = seq - m_rxLastSeq - 1;
            for(quint32 i = 0; i < lost && i < 3; i++) {
                QByteArray plc = m_opus.decodeLost();
                if(m_output && !plc.isEmpty())
                    m_output->write(plc);
            }
        }
        m_rxLastSeq = seq;
        m_rxHasSeq = true;

        QByteArray pcm = m_opus.decode(frame);
        if(m_output && !pcm.isEmpty())
            m_output->write(pcm);
    }
}

void lmAudioStream::doStop() {
    if(m_callSounds && (m_source || m_sink))
        emit micMuted(true);
    stopInput();
    if (m_sink)   { m_sink->stop();   delete m_sink;   m_sink   = nullptr; }
    if (m_sendSock) { m_sendSock->close(); delete m_sendSock; m_sendSock = nullptr; }
    if (m_recvSock) { m_recvSock->close(); delete m_recvSock; m_recvSock = nullptr; }
    m_output = nullptr;
    resetGate();
    m_fallbackReported = false;
    m_fallbackName.clear();
    m_muteBuf.clear();
    m_mutedTalkFrames = 0;
    m_mutedTalkNotified = false;
    m_opus.shutdown();
    m_txKey.clear();
    m_rxKey.clear();
    m_sessionKey.clear();
}

void lmAudioStream::setMicMuted(bool muted) {
    m_micMuted = muted;     // remembered even without a microphone, so it applies once there is one
    //  The capture is not suspended, it keeps running while muted to detect talking. Its audio is dropped.
    resetGate();
    m_muteBuf.clear();
    m_mutedTalkFrames = 0;
    if (!m_source) return;
    emit micMuted(muted);
}

void lmAudioStream::setSpeakerMuted(bool muted) {
    if (!m_sink) return;
    m_speakerMuted = muted;
    muted ? m_sink->suspend() : m_sink->resume();
    emit speakerMuted(muted);
}

void lmAudioStream::setCamMuted(bool muted) {
}

/****************************************************************************
** Class: lmVideoStream
** Description: Video of a call, see the header.
**
** Stream: "VIDEOO" | salt(8), once, when the connection opens, then for every frame
**         length(4, big endian, counts what follows) | seq(4, big endian) | AES-256-GCM(VP8 frame) + tag(16)
**         salt+seq are authenticated as additional data.
****************************************************************************/
static const int VIDEO_HDR = 6;
static const int VIDEO_SALT = 8;
static const int VIDEO_LEN = 4;
static const int VIDEO_SEQ = 4;
static const int VIDEO_TAG = 16;
static const int VIDEO_MAX_FRAME = 2 * 1024 * 1024;     // anything bigger is not from us
static const qint64 VIDEO_MAX_BACKLOG = 512 * 1024;     // unsent bytes, about two seconds of video
static const int VIDEO_RETRY_MS = 500;

lmVideoStream::lmVideoStream(QObject* parent) : QObject(nullptr) {
    Q_UNUSED(parent);   // an object with a parent cannot be moved to another thread
    m_thread = new QThread;
    moveToThread(m_thread);
    m_thread->start();
}

lmVideoStream::~lmVideoStream() {
    stop();
    m_thread->quit();
    m_thread->wait();
    delete m_thread;
}

//  Called from the main thread. The session key is read here, since the crypto object belongs to it.
void lmVideoStream::start(const QHostAddress& peerAddress, quint16 udpPort, lmOpenSSL* crypto, const QString& peerId) {
    QByteArray key = crypto ? crypto->sessionKey(peerId) : QByteArray();
    QMetaObject::invokeMethod(this, [this, peerAddress, udpPort, key]() {
        doStart(peerAddress, udpPort, key);
    }, Qt::QueuedConnection);
}

void lmVideoStream::stop() {
    if(QThread::currentThread() == m_thread)
        doStop();
    else
        QMetaObject::invokeMethod(this, [this]() { doStop(); }, Qt::BlockingQueuedConnection);
}

void lmVideoStream::sendFrame(const QImage& image) {
    if(image.isNull() || !m_pending.testAndSetRelaxed(0, 1))
        return;
    QMetaObject::invokeMethod(this, [this, image]() { doSendFrame(image); }, Qt::QueuedConnection);
}

void lmVideoStream::requestKeyframe() {
    QMetaObject::invokeMethod(this, [this]() { m_vpx.requestKeyframe(); }, Qt::QueuedConnection);
}

void lmVideoStream::doStart(const QHostAddress& peerAddress, quint16 udpPort, const QByteArray& sessionKey) {
    doStop();

    m_peerAddress = peerAddress;
    m_port = udpPort + VIDEO_PORT_OFFSET;
    m_sessionKey = sessionKey;

    if(m_sessionKey.isEmpty() || !m_vpx.init()) {
        lmDebugLog::write("Error: Video call could not start (no session key or VP8 unavailable)");
        return;
    }

    m_txSalt.resize(VIDEO_SALT);
    RAND_bytes((unsigned char*)m_txSalt.data(), VIDEO_SALT);
    m_txKey = lmOpenSSL::deriveVideoKey(m_sessionKey, m_txSalt);
    if(m_txKey.isEmpty()) {
        lmDebugLog::write("Error: Video call could not start (no session key)");
        m_vpx.shutdown();
        return;
    }
    m_txSeq = 0;
    m_txReady = false;

    //  Receive: the peer connects to us
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &lmVideoStream::onNewConnection);
    if(!m_server->listen(QHostAddress::Any, m_port))
        lmDebugLog::write("Error: Video could not listen on port " + QString::number(m_port));

    //  Send: we connect to the peer. It may not be listening yet, so keep trying until it does.
    m_txSock = new QTcpSocket(this);
    m_txSock->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    connect(m_txSock, &QTcpSocket::connected, this, &lmVideoStream::onTxConnected);
    connect(m_txSock, &QTcpSocket::disconnected, this, &lmVideoStream::onTxDisconnected);
    m_connectTimer = new QTimer(this);
    m_connectTimer->setInterval(VIDEO_RETRY_MS);
    connect(m_connectTimer, &QTimer::timeout, this, &lmVideoStream::tryConnect);
    m_connectTimer->start();
    tryConnect();
}

void lmVideoStream::doStop() {
    if(m_connectTimer) { m_connectTimer->stop(); delete m_connectTimer; m_connectTimer = nullptr; }
    if(m_txSock) { m_txSock->disconnect(this); m_txSock->abort(); delete m_txSock; m_txSock = nullptr; }
    closeRx();
    if(m_server) { m_server->close(); delete m_server; m_server = nullptr; }
    m_txReady = false;
    m_vpx.shutdown();
    m_txKey.clear();
    m_sessionKey.clear();
    m_pending.storeRelaxed(0);
}

void lmVideoStream::closeRx() {
    if(m_rxSock) { m_rxSock->disconnect(this); m_rxSock->abort(); delete m_rxSock; m_rxSock = nullptr; }
    m_rxBuf.clear();
    m_rxHandshaken = false;
    m_rxKey.clear();
    m_rxSalt.clear();
    m_rxHasSeq = false;
}

void lmVideoStream::tryConnect() {
    if(m_txSock && m_txSock->state() == QAbstractSocket::UnconnectedState)
        m_txSock->connectToHost(m_peerAddress, m_port);
}

void lmVideoStream::onTxConnected() {
    m_txSock->write(DatagramTypeNames[DT_Video].toLocal8Bit() + m_txSalt);
    m_txReady = true;
    m_vpx.requestKeyframe();    // the peer has to start on a keyframe
}

void lmVideoStream::onTxDisconnected() {
    m_txReady = false;          // the timer connects again
}

void lmVideoStream::doSendFrame(const QImage& image) {
    m_pending.storeRelaxed(0);
    //  Not encoding at all while the connection is down or behind keeps the encoder and the peer's decoder in step
    if(!m_txReady || !m_vpx.isReady() || m_txSock->bytesToWrite() > VIDEO_MAX_BACKLOG)
        return;

    const QByteArray frame = m_vpx.encode(image);
    if(frame.isEmpty())
        return;

    quint32 seq = m_txSeq++;
    QByteArray seqBytes(VIDEO_SEQ, 0);
    qToBigEndian<quint32>(seq, seqBytes.data());
    QByteArray cipher = lmOpenSSL::sealPacket(m_txKey, seq, m_txSalt + seqBytes, frame);
    if(cipher.isEmpty())
        return;

    QByteArray lenBytes(VIDEO_LEN, 0);
    qToBigEndian<quint32>(quint32(VIDEO_SEQ + cipher.size()), lenBytes.data());
    m_txSock->write(lenBytes + seqBytes + cipher);
}

void lmVideoStream::onNewConnection() {
    while(QTcpSocket* socket = m_server->nextPendingConnection()) {
        //  Only the peer of the call may send us video, and one connection at a time
        if(!socket->peerAddress().isEqual(m_peerAddress, QHostAddress::ConvertV4MappedToIPv4)) {
            socket->abort();
            socket->deleteLater();
            continue;
        }
        closeRx();
        m_rxSock = socket;
        m_rxSock->setParent(this);
        connect(m_rxSock, &QTcpSocket::readyRead, this, &lmVideoStream::onRxReadyRead);
    }
}

void lmVideoStream::onRxReadyRead() {
    if(!m_rxSock)
        return;
    m_rxBuf.append(m_rxSock->readAll());

    if(!m_rxHandshaken) {
        if(m_rxBuf.size() < VIDEO_HDR + VIDEO_SALT)
            return;
        if(!m_rxBuf.startsWith(DatagramTypeNames[DT_Video].toLocal8Bit())) {
            closeRx();
            return;
        }
        m_rxSalt = m_rxBuf.mid(VIDEO_HDR, VIDEO_SALT);
        m_rxKey = lmOpenSSL::deriveVideoKey(m_sessionKey, m_rxSalt);
        if(m_rxKey.isEmpty()) {
            closeRx();
            return;
        }
        m_rxBuf.remove(0, VIDEO_HDR + VIDEO_SALT);
        m_rxHandshaken = true;
    }

    while(true) {
        if(m_rxBuf.size() < VIDEO_LEN)
            break;
        const quint32 length = qFromBigEndian<quint32>(m_rxBuf.constData());
        if(length <= quint32(VIDEO_SEQ + VIDEO_TAG) || length > quint32(VIDEO_MAX_FRAME)) {
            lmDebugLog::write("Warning: Video stream is not valid, closing it");
            closeRx();
            return;
        }
        if(m_rxBuf.size() < VIDEO_LEN + int(length))
            break;
        const quint32 seq = qFromBigEndian<quint32>(m_rxBuf.constData() + VIDEO_LEN);
        const QByteArray seqBytes = m_rxBuf.mid(VIDEO_LEN, VIDEO_SEQ);
        const QByteArray cipher = m_rxBuf.mid(VIDEO_LEN + VIDEO_SEQ, int(length) - VIDEO_SEQ);
        m_rxBuf.remove(0, VIDEO_LEN + int(length));

        if(m_rxHasSeq && seq <= m_rxLastSeq)
            continue;   // replayed
        const QByteArray frame = lmOpenSSL::openPacket(m_rxKey, seq, m_rxSalt + seqBytes, cipher);
        if(frame.isEmpty()) {
            lmDebugLog::write("Warning: Video frame failed authentication, closing the stream");
            closeRx();
            return;
        }
        m_rxLastSeq = seq;
        m_rxHasSeq = true;

        const QImage image = m_vpx.decode(frame);
        if(!image.isNull())
            emit frameReceived(image);
    }
}

/****************************************************************************
** Class: FileSender
** Description: Handles sending files.
****************************************************************************/
const qint64 bufferSize = 65535;

FileSender::FileSender(void) {
}

FileSender::FileSender(QString szId, QString szLocalId, QString szPeerId, QString szFilePath,
    QString szFileName, qint64 nFileSize, QString szAddress, int nPort, FileType nType) {

		id = szId;
        localId = szLocalId;
		peerId = szPeerId;
		filePath = szFilePath;
		fileName = szFileName;
		fileSize = nFileSize;
		address = szAddress;
		port = nPort;
		active = false;
		mile = fileSize / 36;
		milestone = mile;
		file = NULL;
		socket = NULL;
		timer = NULL;
		type = nType;
}

FileSender::~FileSender(void) {
}

void FileSender::init(void) {
	socket = new QTcpSocket(this);
	connect(socket, SIGNAL(connected()), this, SLOT(connected()));
	connect(socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
	connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
	connect(socket, SIGNAL(bytesWritten(qint64)), this, SLOT(bytesWritten(qint64)));

	QHostAddress hostAddress(address);
	socket->connectToHost(hostAddress, port);
}

void FileSender::stop(void) {
	active = false;

	if(timer)
		timer->stop();
	if(file && file->isOpen())
		file->close();
	if(socket && socket->isOpen())
		socket->close();
}

void FileSender::connected(void) {
    QByteArray data = id.toLocal8Bit();
    data.append(localId.toLocal8Bit());
	data.insert(0, "FILE");	// insert indicator that this socket handles file transfer
	//	send an id message and then wait for a START message 
	//	from receiver, which will trigger readyRead signal
	socket->write(data);
}

void FileSender::disconnected(void) {
	if(active) {
		QString data;
		emit progressUpdated(FM_Send, FO_Error, type, &id, &peerId, &data);
	}
}

void FileSender::readyRead(void) {
	//	message received from receiver, start sending the file
	sendFile();
}

void FileSender::timer_timeout(void) {
	if(!active)
		return;
	
	QString transferred = QString::number(file->pos());
	emit progressUpdated(FM_Send, FO_Progress, type, &id, &peerId, &transferred);
}

void FileSender::bytesWritten(qint64 bytes) {
    Q_UNUSED(bytes);

	if(!active)
		return;

	qint64 unsentBytes = fileSize - file->pos();

	if(unsentBytes == 0) {
		active = false;
		file->close();
		socket->close();
        emit progressUpdated(FM_Send, FO_Complete, type, &id, &peerId, &filePath);
		return;
	}

	qint64 bytesToSend = (bufferSize < unsentBytes) ? bufferSize : unsentBytes;
	qint64 bytesRead = file->read(buffer, bytesToSend);
	socket->write(buffer, bytesRead);

//	if(file->pos() > milestone) {
//		QString transferred = QString::number(file->pos());
//		emit progressUpdated(FM_Send, FO_Progress, type, &id, &peerId, &transferred);
//		milestone += mile;
//	}
}

void FileSender::sendFile(void) {
	file = new QFile(filePath);

	if(file->open(QIODevice::ReadOnly)) {
		buffer = new char[bufferSize];
		active = true;

		timer = new QTimer(this);
		connect(timer, SIGNAL(timeout()), this, SLOT(timer_timeout()));
        timer->start(PROGRESS_TIMEOUT);

		qint64 unsentBytes = fileSize - file->pos();
		qint64 bytesToSend = (bufferSize < unsentBytes) ? bufferSize : unsentBytes;
		qint64 bytesRead = file->read(buffer, bytesToSend);
		socket->write(buffer, bytesRead);
	} else {
		socket->close();
        QString data;
		emit progressUpdated(FM_Send, FO_Error, type, &id, &peerId, &data);
	}
}


/****************************************************************************
** Class: FileReceiver
** Description: Handles receiving files.
****************************************************************************/
FileReceiver::FileReceiver(void) {
}

FileReceiver::FileReceiver(QString szId, QString szPeerId, QString szFilePath, QString szFileName, 
	qint64 nFileSize, QString szAddress, int nPort, FileType nType) {

		id = szId;
		peerId = szPeerId;
		filePath = szFilePath;
		fileName = szFileName;
		fileSize = nFileSize;
		address = szAddress;
		port = nPort;
		active = false;
		mile = fileSize / 36;
		milestone = mile;
		file = NULL;
		socket = NULL;
		timer = NULL;
		type = nType;
        lastPosition = 0;
        numTimeOuts = 0;
}

FileReceiver::~FileReceiver(void) {
}

void FileReceiver::init(QTcpSocket* socket) {
	this->socket = socket;
	connect(socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
	connect(this->socket, SIGNAL(readyRead()), this, SLOT(readyRead()));

	receiveFile();
	//	now send a START message to sender
	socket->write("START");
}

void FileReceiver::stop(void) {
	bool deleteFile = false;

	active = false;

	if(timer)
		timer->stop();
	if(file && file->isOpen()) {
		deleteFile = (file->pos() < fileSize);
		file->close();
	}
	if(socket && socket->isOpen())
		socket->close();

	if(deleteFile)
		QFile::remove(filePath);
}

void FileReceiver::disconnected(void) {
	if(active) {
		QString data;
		emit progressUpdated(FM_Receive, FO_Error, type, &id, &peerId, &data);
	}
}

void FileReceiver::readyRead(void) {
	if(!active)
		return;

	qint64 bytesReceived = socket->read(buffer, bufferSize);
	file->write(buffer, bytesReceived);

	qint64 unreceivedBytes = fileSize - file->pos();
	if(unreceivedBytes == 0) {
		active = false;
		file->close();
		socket->close();
		emit progressUpdated(FM_Receive, FO_Complete, type, &id, &peerId, &filePath);
		return;
	}

//	if(file->pos() > milestone) {
//		QString transferred = QString::number(file->pos());
//		emit progressUpdated(FM_Receive, FO_Progress, type, &id, &peerId, &transferred);
//		milestone += mile;
//	}
}

void FileReceiver::timer_timeout(void) {
	if(!active)
		return;

    if(lastPosition < file->pos()) {
        lastPosition = file->pos();
        numTimeOuts = 0;
    } else {
        numTimeOuts++;
        if(numTimeOuts > 20) {
            QString data;
            emit progressUpdated(FM_Receive, FO_Error, type, &id, &peerId, &data);
            stop();
            return;
        }
    }

	QString transferred = QString::number(file->pos());
	emit progressUpdated(FM_Receive, FO_Progress, type, &id, &peerId, &transferred);
}

void FileReceiver::receiveFile(void) {
	QDir dir = QFileInfo(filePath).dir();
	if(!dir.exists())
		dir.mkpath(dir.absolutePath());

	file = new QFile(filePath);

	if(file->open(QIODevice::WriteOnly)) {
		buffer = new char[bufferSize];
		active = true;

		timer = new QTimer(this);
		connect(timer, SIGNAL(timeout()), this, SLOT(timer_timeout()));
        timer->start(PROGRESS_TIMEOUT);
	} else {
		socket->close();
		emit progressUpdated(FM_Receive, FO_Error, type, &id, &peerId, &filePath);
	}
}


/****************************************************************************
** Class: MsgStream
** Description: Handles transmission and reception of TCP streaming messages.
****************************************************************************/
MsgStream::MsgStream(void) {
	socket = NULL;
	reading = false;
}

MsgStream::MsgStream(QString szLocalId, QString szPeerId, QString szPeerAddress, int nPort) {
	localId = szLocalId;
	peerId = szPeerId;
	peerAddress = szPeerAddress;
	port = nPort;
	socket = NULL;
	reading = false;
	inDataLen = 0;
}

MsgStream::~MsgStream(void) {
}

void MsgStream::init(void) {
	socket = new QTcpSocket(this);
	connect(socket, SIGNAL(connected()), this, SLOT(connected()));
	connect(socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
	connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));

	QHostAddress hostAddress(peerAddress);
	socket->connectToHost(hostAddress, port);
}

void MsgStream::init(QTcpSocket* socket) {
	this->socket = socket;
	connect(socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
	connect(this->socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
}

void MsgStream::stop(void) {
	if(socket && socket->isOpen())
		socket->close();
}

void MsgStream::sendMessage(QByteArray& data) {
	qint32 dataLen = sizeof(quint32) + data.length();
	outData.resize(dataLen);

	QDataStream stream(&outData, QIODevice::WriteOnly);
	stream << (quint32)data.length();
	stream.writeRawData(data.data(), data.length());

	qint64 numBytesWritten = socket->write(outData);
	if(numBytesWritten < 0)
		lmDebugLog::write("Error: Socket write failed");
}

void MsgStream::connected(void) {
	outData = localId.toLocal8Bit();
	outData.insert(0, "MSG");	// insert indicator that this socket handles messages

	//	send an id message and then wait for public key message 
	//	from receiver, which will trigger readyRead signal
	qint64 numBytesWritten = socket->write(outData);
	if(numBytesWritten < 0)
		lmDebugLog::write("Error: Socket write failed");
}

void MsgStream::disconnected(void) {
	emit connectionLost(&peerId);
}

void MsgStream::readyRead(void) {
	qint64 available = socket->bytesAvailable();
	while(available > 0) {
		if(!reading) {
			reading = true;
			QByteArray len = socket->read(4);
			QDataStream stream(len);
			stream >> inDataLen;
			inData.clear();
			QByteArray data = socket->read(inDataLen);
			inData.append(data);
			inDataLen -= data.length();
			available -= (sizeof(quint32) +  data.length());
			if(inDataLen == 0) {
				reading = false;
				emit messageReceived(&peerId, &peerAddress, inData);
			}
		} else {
			QByteArray data = socket->read(inDataLen);
			inData.append(data);
			inDataLen -= data.length();
			available -= data.length();
			if(inDataLen == 0) {
				reading = false;
				emit messageReceived(&peerId, &peerAddress, inData);
			}
		}
	}
}
