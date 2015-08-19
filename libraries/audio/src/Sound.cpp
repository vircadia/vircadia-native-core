//
//  Sound.cpp
//  libraries/audio/src
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <stdint.h>

#include <glm/glm.hpp>

#include <QDataStream>
#include <QtCore/QDebug>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <qendian.h>

#include <LimitedNodeList.h>
#include <NetworkAccessManager.h>
#include <SharedUtil.h>

#include "AudioRingBuffer.h"
#include "AudioFormat.h"
#include "AudioBuffer.h"
#include "AudioEditBuffer.h"
#include "AudioLogging.h"
#include "Sound.h"

static int soundMetaTypeId = qRegisterMetaType<Sound*>();

QScriptValue soundSharedPointerToScriptValue(QScriptEngine* engine, SharedSoundPointer const& in) {
    return engine->newQObject(in.data());
}

void soundSharedPointerFromScriptValue(const QScriptValue& object, SharedSoundPointer &out) {
    out = SharedSoundPointer(qobject_cast<Sound*>(object.toQObject()));
}

QScriptValue soundPointerToScriptValue(QScriptEngine* engine, Sound* const& in) {
    return engine->newQObject(in);
}

void soundPointerFromScriptValue(const QScriptValue &object, Sound* &out) {
    out = qobject_cast<Sound*>(object.toQObject());
}

Sound::Sound(const QUrl& url, bool isStereo) :
    Resource(url),
    _isStereo(isStereo),
    _isReady(false)
{

}

void Sound::downloadFinished(const QByteArray& data) {
    // replace our byte array with the downloaded data
    QByteArray rawAudioByteArray = QByteArray(data);
    QString fileName = getURL().fileName();

    const QString WAV_EXTENSION = ".wav";

    if (fileName.endsWith(WAV_EXTENSION)) {

        QString headerContentType = "audio/x-wav";
        //QByteArray headerContentType = reply->rawHeader("Content-Type");

        // WAV audio file encountered
        if (headerContentType == "audio/x-wav"
            || headerContentType == "audio/wav"
            || headerContentType == "audio/wave"
            || fileName.endsWith(WAV_EXTENSION)) {

            QByteArray outputAudioByteArray;

            interpretAsWav(rawAudioByteArray, outputAudioByteArray);
            downSample(outputAudioByteArray);
        } else {
            // check if this was a stereo raw file
            // since it's raw the only way for us to know that is if the file was called .stereo.raw
            if (fileName.toLower().endsWith("stereo.raw")) {
                _isStereo = true;
                qCDebug(audio) << "Processing sound of" << rawAudioByteArray.size() << "bytes from" << getURL() << "as stereo audio file.";
            }

            // Process as RAW file
            downSample(rawAudioByteArray);
        }
        trimFrames();
    } else {
        qCDebug(audio) << "Network reply without 'Content-Type'.";
    }

    _isReady = true;
}

void Sound::downSample(const QByteArray& rawAudioByteArray) {
    // assume that this was a RAW file and is now an array of samples that are
    // signed, 16-bit, 48Khz

    // we want to convert it to the format that the audio-mixer wants
    // which is signed, 16-bit, 24Khz

    int numSourceSamples = rawAudioByteArray.size() / sizeof(AudioConstants::AudioSample);

    int numDestinationBytes = rawAudioByteArray.size() / sizeof(AudioConstants::AudioSample);
    if (_isStereo && numSourceSamples % 2 != 0) {
        numDestinationBytes += sizeof(AudioConstants::AudioSample);
    }

    _byteArray.resize(numDestinationBytes);

    int16_t* sourceSamples = (int16_t*) rawAudioByteArray.data();
    int16_t* destinationSamples = (int16_t*) _byteArray.data();

    if (_isStereo) {
        for (int i = 0; i < numSourceSamples; i += 4) {
            if (i + 2 >= numSourceSamples) {
                destinationSamples[i / 2] = sourceSamples[i];
                destinationSamples[(i / 2) + 1] = sourceSamples[i + 1];
            } else {
                destinationSamples[i / 2] = (sourceSamples[i] + sourceSamples[i + 2]) / 2;
                destinationSamples[(i / 2) + 1] = (sourceSamples[i + 1] + sourceSamples[i + 3]) / 2;
            }
        }
    } else {
        for (int i = 1; i < numSourceSamples; i += 2) {
            if (i + 1 >= numSourceSamples) {
                destinationSamples[(i - 1) / 2] = (sourceSamples[i - 1] + sourceSamples[i]) / 2;
            } else {
                destinationSamples[(i - 1) / 2] = ((sourceSamples[i - 1] + sourceSamples[i + 1]) / 4) + (sourceSamples[i] / 2);
            }
        }
    }
}

void Sound::trimFrames() {

    const uint32_t inputFrameCount = _byteArray.size() / sizeof(int16_t);
    const uint32_t trimCount = 1024;  // number of leading and trailing frames to trim

    if (inputFrameCount <= (2 * trimCount)) {
        return;
    }

    int16_t* inputFrameData = (int16_t*)_byteArray.data();

    AudioEditBufferFloat32 editBuffer(1, inputFrameCount);
    editBuffer.copyFrames(1, inputFrameCount, inputFrameData, false /*copy in*/);

    editBuffer.linearFade(0, trimCount, true);
    editBuffer.linearFade(inputFrameCount - trimCount, inputFrameCount, false);

    editBuffer.copyFrames(1, inputFrameCount, inputFrameData, true /*copy out*/);
}

//
// Format description from https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
//
// The header for a WAV file looks like this:
// Positions     Sample Value     Description
//   00-03         "RIFF"       Marks the file as a riff file. Characters are each 1 byte long.
//   04-07         File size (int) Size of the overall file - 8 bytes, in bytes (32-bit integer).
//   08-11         "WAVE"       File Type Header. For our purposes, it always equals "WAVE".
//   12-15         "fmt "       Format chunk marker.
//   16-19         16           Length of format data as listed above
//   20-21         1            Type of format: (1=PCM, 257=Mu-Law, 258=A-Law, 259=ADPCM) - 2 byte integer
//   22-23         2            Number of Channels - 2 byte integer
//   24-27         44100        Sample Rate - 32 byte integer. Sample Rate = Number of Samples per second, or Hertz.
//   28-31         176400       (Sample Rate * BitsPerSample * Channels) / 8.
//   32-33         4            (BitsPerSample * Channels) / 8 - 8 bit mono2 - 8 bit stereo/16 bit mono4 - 16 bit stereo
//   34-35         16           Bits per sample
//   36-39         "data"       Chunk header. Marks the beginning of the data section.
//   40-43         File size (int) Size of the data section.
//   44-??                      Actual sound data
// Sample values are given above for a 16-bit stereo source.
//

struct chunk {
    char        id[4];
    quint32     size;
};

struct RIFFHeader {
    chunk       descriptor;     // "RIFF"
    char        type[4];        // "WAVE"
};

struct WAVEHeader {
    chunk       descriptor;
    quint16     audioFormat;    // Format type: 1=PCM, 257=Mu-Law, 258=A-Law, 259=ADPCM
    quint16     numChannels;    // Number of channels: 1=mono, 2=stereo
    quint32     sampleRate;
    quint32     byteRate;       // Sample rate * Number of Channels * Bits per sample / 8
    quint16     blockAlign;     // (Number of Channels * Bits per sample) / 8.1
    quint16     bitsPerSample;
};

struct DATAHeader {
    chunk       descriptor;
};

struct CombinedHeader {
    RIFFHeader  riff;
    WAVEHeader  wave;
};

void Sound::interpretAsWav(const QByteArray& inputAudioByteArray, QByteArray& outputAudioByteArray) {

    CombinedHeader fileHeader;

    // Create a data stream to analyze the data
    QDataStream waveStream(const_cast<QByteArray *>(&inputAudioByteArray), QIODevice::ReadOnly);
    if (waveStream.readRawData(reinterpret_cast<char *>(&fileHeader), sizeof(CombinedHeader)) == sizeof(CombinedHeader)) {

        if (strncmp(fileHeader.riff.descriptor.id, "RIFF", 4) == 0) {
            waveStream.setByteOrder(QDataStream::LittleEndian);
        } else {
            // descriptor.id == "RIFX" also signifies BigEndian file
            // waveStream.setByteOrder(QDataStream::BigEndian);
            qCDebug(audio) << "Currently not supporting big-endian audio files.";
            return;
        }

        if (strncmp(fileHeader.riff.type, "WAVE", 4) != 0
            || strncmp(fileHeader.wave.descriptor.id, "fmt", 3) != 0) {
            qCDebug(audio) << "Not a WAVE Audio file.";
            return;
        }

        // added the endianess check as an extra level of security

        if (qFromLittleEndian<quint16>(fileHeader.wave.audioFormat) != 1) {
            qCDebug(audio) << "Currently not supporting non PCM audio files.";
            return;
        }
        if (qFromLittleEndian<quint16>(fileHeader.wave.numChannels) == 2) {
            _isStereo = true;
        } else if (qFromLittleEndian<quint16>(fileHeader.wave.numChannels) > 2) {
            qCDebug(audio) << "Currently not support audio files with more than 2 channels.";
        }

        if (qFromLittleEndian<quint16>(fileHeader.wave.bitsPerSample) != 16) {
            qCDebug(audio) << "Currently not supporting non 16bit audio files.";
            return;
        }
        if (qFromLittleEndian<quint32>(fileHeader.wave.sampleRate) != 48000) {
            qCDebug(audio) << "Currently not supporting non 48KHz audio files.";
            return;
        }

        // Skip any extra data in the WAVE chunk
        waveStream.skipRawData(fileHeader.wave.descriptor.size - (sizeof(WAVEHeader) - sizeof(chunk)));

        // Read off remaining header information
        DATAHeader dataHeader;
        while (true) {
            // Read chunks until the "data" chunk is found
            if (waveStream.readRawData(reinterpret_cast<char *>(&dataHeader), sizeof(DATAHeader)) == sizeof(DATAHeader)) {
                if (strncmp(dataHeader.descriptor.id, "data", 4) == 0) {
                    break;
                }
                waveStream.skipRawData(dataHeader.descriptor.size);
            } else {
                qCDebug(audio) << "Could not read wav audio data header.";
                return;
            }
        }

        // Now pull out the data
        quint32 outputAudioByteArraySize = qFromLittleEndian<quint32>(dataHeader.descriptor.size);
        outputAudioByteArray.resize(outputAudioByteArraySize);
        if (waveStream.readRawData(outputAudioByteArray.data(), outputAudioByteArraySize) != (int)outputAudioByteArraySize) {
            qCDebug(audio) << "Error reading WAV file";
        }

    } else {
        qCDebug(audio) << "Could not read wav audio file header.";
        return;
    }
}
