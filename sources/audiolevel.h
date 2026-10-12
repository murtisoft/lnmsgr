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

#ifndef AUDIOLEVEL_H
#define AUDIOLEVEL_H

#include <cmath>
#include <QAudioFormat>
#include <QByteArray>

//	Level measuring of raw captured audio. Shared by the microphone meter, and meant for
//	the voice activation gate and the talking-while-muted alert later on.
namespace lmAudioLevel {

//	What digital silence is reported as. The meter never goes lower.
constexpr float SilenceDb = -90.0f;

//	RMS level of interleaved PCM, in dB relative to full scale (0 dB = as loud as the format can get).
//	All channels are measured together. Unsupported sample formats read as silence.
inline float rmsDbfs(const char* data, qsizetype bytes, QAudioFormat::SampleFormat format) {
	double sum = 0;
	qsizetype count = 0;

	switch(format) {
	case QAudioFormat::Int16: {
		const qint16* samples = reinterpret_cast<const qint16*>(data);
		count = bytes / qsizetype(sizeof(qint16));
		for(qsizetype i = 0; i < count; i++) {
			const double v = samples[i] / 32768.0;
			sum += v * v;
		}
		break;
	}
	case QAudioFormat::Int32: {
		const qint32* samples = reinterpret_cast<const qint32*>(data);
		count = bytes / qsizetype(sizeof(qint32));
		for(qsizetype i = 0; i < count; i++) {
			const double v = samples[i] / 2147483648.0;
			sum += v * v;
		}
		break;
	}
	case QAudioFormat::Float: {
		const float* samples = reinterpret_cast<const float*>(data);
		count = bytes / qsizetype(sizeof(float));
		for(qsizetype i = 0; i < count; i++) {
			const double v = samples[i];
			sum += v * v;
		}
		break;
	}
	case QAudioFormat::UInt8: {
		const quint8* samples = reinterpret_cast<const quint8*>(data);
		count = bytes;
		for(qsizetype i = 0; i < count; i++) {
			const double v = (samples[i] - 128) / 128.0;
			sum += v * v;
		}
		break;
	}
	default:
		return SilenceDb;
	}

	if(count <= 0)
		return SilenceDb;
	const double rms = std::sqrt(sum / double(count));
	if(rms < 1e-5)
		return SilenceDb;
	return float(20.0 * std::log10(rms));
}

inline float rmsDbfs(const QByteArray& data, QAudioFormat::SampleFormat format) {
	return rmsDbfs(data.constData(), data.size(), format);
}

}	// namespace lmAudioLevel

#endif // AUDIOLEVEL_H
