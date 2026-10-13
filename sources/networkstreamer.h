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


#ifndef NETWORKSTREAMER_H
#define NETWORKSTREAMER_H

#include "definitions.h"
#include <QTcpSocket>
#include <QTcpServer>
#include <QTimer>
#include <QFile>
#include <QObject>
#include <QUdpSocket>
#include <QThread>
#include <QElapsedTimer>
#include <QAudioSource>
#include <QAudioSink>
#include <QMediaDevices>
#include <QAudioFormat>
#include <QAudioDevice>
#include "opushandler.h"
#include "ovpxhandler.h"
#include <QImage>
#include <QAtomicInt>
#include "opensslhandler.h"


class lmAudioStream : public QObject {
    Q_OBJECT

signals:
    void micMuted(bool muted);
    void speakerMuted(bool muted);
    void camMuted(bool muted);
    //  The microphone in use changed, or could not be the one the user picked. fallback is true when the picked
    //  device is unavailable. An empty name means there is no microphone at all.
    void micDeviceChanged(const QString& deviceName, bool fallback);
    //  The user keeps talking while the microphone is muted. Emitted at most once per cooldown.
    void mutedTalk();

public:
    //  The stream runs on its own thread (capture, encode, encrypt, network, decode, playback),
    //  so UI work on the main thread cannot stall the audio. The object must have no parent.
    explicit lmAudioStream(QObject* parent = nullptr);
    ~lmAudioStream();

    void start(const QHostAddress& peerAddress, quint16 port, lmOpenSSL* crypto, const QString& peerId);
    void stop();

    //  Microphone to capture from, QAudioDevice::id(). Empty means the system default. Can be called before
    //  start() and during a call, the capture is switched on the fly. A missing device falls back to the default.
    void setMicDevice(const QByteArray& deviceId);
    //  Re-check the microphone. Call it when the list of audio devices changed (unplugged / plugged in).
    void refreshMicDevice();
    //  When the microphone transmits, MicActivation, and the voice activation threshold in dBFS. Safe at any time.
    void setMicActivation(int mode, int thresholdDb);
    //  Push to talk: key down / up. Only matters in push to talk mode.
    void setPushToTalk(bool down);
    //  Whether starting and ending the call announce themselves with the microphone on / off sounds (micMuted).
    //  On by default. A video call turns it off, it has its own camera sounds. Call it before start().
    void setCallSounds(bool enabled);

public slots:
    void setMicMuted(bool muted);
    void setSpeakerMuted(bool muted);
    void setCamMuted(bool muted);

private slots:
    void onMicReadyRead();
    void onNetReadyRead();
    void onMicStateChanged(QAudio::State state);

private:
    void doStart(const QHostAddress& peerAddress, quint16 udpPort, const QByteArray& sessionKey);
    void doStop();
    void startInput();
    void stopInput();
    QAudioDevice resolveInputDevice(bool* pFallback) const;
    void reportInput(const QString& name, bool fallback);
    void sendPackets(const QList<QByteArray>& packets);
    void processGatedFrame(const QByteArray& frame);
    void resetGate();
    void processMutedAudio(const QByteArray& pcm);

    QThread*       m_thread  = nullptr;
    QByteArray     m_sessionKey;
    QAudioSource*  m_source  = nullptr;
    QAudioSink*    m_sink    = nullptr;
    QAudioFormat   m_format;
    QByteArray     m_micDeviceId;           // what the user asked for, empty = default
    QByteArray     m_inputId;               // device actually in use
    int            m_inputRestarts = 0;     // consecutive restarts after a capture error
    bool           m_fallbackReported = false;
    QString        m_fallbackName;

    // transmit gate for push to talk / voice activation, works on whole 20 ms frames
    int            m_activation = MA_Continuous;
    int            m_thresholdDb = -40;
    bool           m_pttDown = false;
    bool           m_gateOpen = false;
    int            m_holdFrames = 0;        // frames left to keep transmitting after the trigger ended
    QByteArray     m_gateBuf;               // captured audio that is not a whole frame yet
    QList<QByteArray> m_preroll;            // the last quiet frames, sent when speech starts so no word is cut off
    QIODevice*     m_input   = nullptr;
    QIODevice*     m_output  = nullptr;
    QUdpSocket*    m_sendSock = nullptr;
    QUdpSocket*    m_recvSock = nullptr;
    QHostAddress   m_peerAddress;
    quint16        m_port    = 0;
    bool           m_micMuted = false;
    bool           m_callSounds = true;
    QByteArray     m_muteBuf;               // muted capture that is not a whole frame yet
    int            m_mutedTalkFrames = 0;   // loud frames heard while muted
    QElapsedTimer  m_mutedTalkTimer;        // time since the last reminder
    bool           m_mutedTalkNotified = false;
    bool           m_speakerMuted = false;
    bool           m_camMuted = false;

    lmOpusHandler  m_opus;

    // transmit side: random salt per call start, unique key per salt
    QByteArray     m_txSalt;
    QByteArray     m_txKey;
    quint32        m_txSeq = 0;

    // receive side
    QByteArray     m_rxSalt;
    QByteArray     m_rxKey;
    quint32        m_rxLastSeq = 0;
    bool           m_rxHasSeq = false;

    static constexpr quint16 AUDIO_PORT_OFFSET = 1; // udpPort + 1
};


/****************************************************************************
** Class: lmVideoStream
** Description: Video of a call. One TCP connection per direction: every side listens for the video of the
**              peer and connects to the peer's listener to send its own, so there are no roles to negotiate.
**              Frames are VP8 (see lmVpxHandler) and encrypted with the session key, like the voice packets.
**              Being a reliable stream there is no loss handling, a frame is simply not encoded while the
**              connection cannot keep up.
****************************************************************************/
class lmVideoStream : public QObject {
    Q_OBJECT

signals:
    //  A decoded frame of the peer. Emitted from the video thread, connect it to a receiver on another thread
    //  and it is delivered queued.
    void frameReceived(const QImage& image);

public:
    //  Runs on its own thread (encode, decode, encrypt, network), so the UI is never stalled by video.
    //  The object must have no parent.
    explicit lmVideoStream(QObject* parent = nullptr);
    ~lmVideoStream();

    void start(const QHostAddress& peerAddress, quint16 udpPort, lmOpenSSL* crypto, const QString& peerId);
    //  Blocks until the video thread has released the sockets.
    void stop();
    //  A frame of the local video, any size, it is fitted to 1280x720. Safe to call from any thread at any
    //  rate: a frame is dropped when the previous one is still waiting to be encoded.
    void sendFrame(const QImage& image);
    //  The next frame sent is a keyframe, e.g. after the source was switched.
    void requestKeyframe();

private slots:
    void onNewConnection();
    void onRxReadyRead();
    void onTxConnected();
    void onTxDisconnected();
    void tryConnect();

private:
    void doStart(const QHostAddress& peerAddress, quint16 udpPort, const QByteArray& sessionKey);
    void doStop();
    void doSendFrame(const QImage& image);
    void closeRx();

    QThread*       m_thread  = nullptr;
    QByteArray     m_sessionKey;
    QHostAddress   m_peerAddress;
    quint16        m_port    = 0;
    lmVpxHandler   m_vpx;
    QAtomicInt     m_pending;               // a frame is waiting to be encoded
    // transmit side
    QTcpSocket*    m_txSock  = nullptr;
    QTimer*        m_connectTimer = nullptr;
    bool           m_txReady = false;       // connected, handshake sent
    QByteArray     m_txSalt;
    QByteArray     m_txKey;
    quint32        m_txSeq   = 0;
    // receive side
    QTcpServer*    m_server  = nullptr;
    QTcpSocket*    m_rxSock  = nullptr;
    QByteArray     m_rxBuf;
    bool           m_rxHandshaken = false;
    QByteArray     m_rxSalt;
    QByteArray     m_rxKey;
    quint32        m_rxLastSeq = 0;
    bool           m_rxHasSeq = false;
    static constexpr quint16 VIDEO_PORT_OFFSET = 2; // udpPort + 2, TCP
};

/****************************************************************************
** Class: FileSender
** Description: Handles sending files.
****************************************************************************/
class FileSender : public QObject {
	Q_OBJECT 

public:
	FileSender(void);
    FileSender(QString szId, QString szLocalId, QString szPeerId, QString szFilePath, QString szFileName, qint64 nFileSize,
        QString szAddress, int nPort, FileType nType);
	~FileSender(void);

	void init(void);
	void stop(void);

	QString id;
    QString peerId;
	FileType type;

signals:
	void progressUpdated(FileMode mode, FileOp fileOp, FileType type, QString* lpszId, QString* lpszUserId, QString* lpszData);

private slots:
	void connected(void);
	void disconnected(void);
	void readyRead(void);
	void bytesWritten(qint64 bytes);
	void timer_timeout(void);

private:
	void sendFile(void);

    QString localId;
	QString filePath;
	QString fileName;
	qint64 fileSize;
	QString address;
	int port;
	qint64 sentBytes;
	QTcpSocket* socket;
	QFile* file;
	char* buffer;
	bool active;
	qint64 milestone;
	qint64 mile;
	QTimer* timer;
};

/****************************************************************************
** Class: FileReceiver
** Description: Handles receiving files.
****************************************************************************/
class FileReceiver : public QObject {
	Q_OBJECT

public:
	FileReceiver(void);
	FileReceiver(QString szId, QString szPeerId, QString szFilePath, QString szFileName, qint64 nFileSize, 
		QString szAddress, int nPort, FileType nType);
	~FileReceiver(void);

	void init(QTcpSocket* socket);
	void stop(void);
	
	QString id;
    QString peerId;
	FileType type;

signals:
	void progressUpdated(FileMode mode, FileOp fileOp, FileType type, QString* lpszId, QString* lpszUserId, QString* lpszData);

private slots:
	void disconnected(void);
	void readyRead(void);
	void timer_timeout(void);

private:
	void receiveFile(void);

	QString filePath;
	QString fileName;
	qint64 fileSize;
	QString address;
	int port;
	qint64 sentBytes;
	QTcpSocket* socket;
	QFile* file;
	char* buffer;
	bool active;
	qint64 milestone;
	qint64 mile;
	QTimer* timer;
    int numTimeOuts;
    qint64 lastPosition;
};

/****************************************************************************
** Class: MsgStream
** Description: Handles transmission and reception of TCP streaming messages.
****************************************************************************/
class MsgStream : public QObject {
	Q_OBJECT

public:
	MsgStream(void);
	MsgStream(QString szLocalId, QString szPeerId, QString szPeerAddress, int nPort);
	~MsgStream(void);

	void init(void);
	void init(QTcpSocket* socket);
	void stop(void);
	void sendMessage(QByteArray& data);

signals:
	void connectionLost(QString* lpszUserId);
	void messageReceived(QString* lpszUserId, QString* lpszAddress, QByteArray& data);

private slots:
	void connected(void);
	void disconnected(void);
	void readyRead(void);
	void bytesWritten(qint64 bytes);

private:
	QTcpSocket* socket;
	int port;
	QString localId;
	QString peerId;
	QString peerAddress;
	QByteArray outData;
	QByteArray inData;
	quint32 outDataLen;
	quint32 inDataLen;
	bool reading;
};

#endif // NETWORKSTREAMER_H
