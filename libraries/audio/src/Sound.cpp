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
#include "Sound.h"

// procedural audio version of Sound
Sound::Sound(float volume, float frequency, float duration, float decay, QObject* parent) :
    QObject(parent),
    _isStereo(false)
{
    static char monoAudioData[MAX_PACKET_SIZE];
    static int16_t* monoAudioSamples = (int16_t*)(monoAudioData);

    float t;
    const float AUDIO_CALLBACK_MSECS = (float) NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL / (float)SAMPLE_RATE * 1000.0;
    const float MAX_VOLUME = 32000.f;
    const float MAX_DURATION = 2.f;
    const float MIN_AUDIBLE_VOLUME = 0.001f;
    const float NOISE_MAGNITUDE = 0.02f;
    const int MAX_SAMPLE_VALUE = std::numeric_limits<int16_t>::max();
    const int MIN_SAMPLE_VALUE = std::numeric_limits<int16_t>::min();
    int numSamples = NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL; // we add sounds in chunks of this many samples
    
    int chunkStartingSample = 0;
    float waveFrequency = (frequency / SAMPLE_RATE) * TWO_PI;
    while (volume > 0.f) {
        for (int i = 0; i < numSamples; i++) {
            t = (float)chunkStartingSample + (float)i;
            float sample = sinf(t * waveFrequency);
            sample += ((randFloat() - 0.5f) * NOISE_MAGNITUDE);
            sample *= volume * MAX_VOLUME;

            monoAudioSamples[i] = glm::clamp((int)sample, MIN_SAMPLE_VALUE, MAX_SAMPLE_VALUE);
            volume *= (1.f - decay);
        }
        // add the monoAudioSamples to our actual output Byte Array
        _byteArray.append(monoAudioData, numSamples * sizeof(int16_t));
        chunkStartingSample += numSamples;
        duration = glm::clamp(duration - (AUDIO_CALLBACK_MSECS / 1000.f), 0.f, MAX_DURATION);
        //qDebug() << "decaying... _duration=" << _duration;
        if (duration == 0.f || (volume < MIN_AUDIBLE_VOLUME)) {
            volume = 0.f;
        }
    }
}

Sound::Sound(const QUrl& sampleURL, bool isStereo, QObject* parent) :
    QObject(parent),
    _isStereo(isStereo),
    _hasDownloaded(false)
{
    // assume we have a QApplication or QCoreApplication instance and use the
    // QNetworkAccess manager to grab the raw audio file at the given URL

    NetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();

    qDebug() << "Requesting audio file" << sampleURL.toDisplayString();
    
    QNetworkReply* soundDownload = networkAccessManager.get(QNetworkRequest(sampleURL));
    connect(soundDownload, &QNetworkReply::finished, this, &Sound::replyFinished);
    connect(soundDownload, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(replyError(QNetworkReply::NetworkError)));
}

Sound::Sound(const QByteArray byteArray, QObject* parent) :
    QObject(parent),
    _byteArray(byteArray),
    _isStereo(false),
    _hasDownloaded(true)
{
}

void Sound::append(const QByteArray byteArray) {
    _byteArray.append(byteArray);
}

void Sound::replyFinished() {

    QNetworkReply* reply = reinterpret_cast<QNetworkReply*>(sender());
    
    // replace our byte array with the downloaded data
    QByteArray rawAudioByteArray = reply->readAll();

    // foreach(QByteArray b, reply->rawHeaderList())
    //     qDebug() << b.constData() << ": " << reply->rawHeader(b).constData();

    if (reply->hasRawHeader("Content-Type")) {

        QByteArray headerContentType = reply->rawHeader("Content-Type");

        // WAV audio file encountered
        if (headerContentType == "audio/x-wav"
            || headerContentType == "audio/wav"
            || headerContentType == "audio/wave") {

            QByteArray outputAudioByteArray;

            interpretAsWav(rawAudioByteArray, outputAudioByteArray);
            downSample(outputAudioByteArray);
        } else {
            //  Process as RAW file
            downSample(rawAudioByteArray);
        }
        trimFrames();
    } else {
        qDebug() << "Network reply without 'Content-Type'.";
    }
    
    _hasDownloaded = true;
}

void Sound::replyError(QNetworkReply::NetworkError code) {
    QNetworkReply* reply = reinterpret_cast<QNetworkReply*>(sender());
    qDebug() << "Error downloading sound file at" << reply->url().toString() << "-" << reply->errorString();
}

void Sound::downSample(const QByteArray& rawAudioByteArray) {
    // assume that this was a RAW file and is now an array of samples that are
    // signed, 16-bit, 48Khz, mono

    // we want to convert it to the format that the audio-mixer wants
    // which is signed, 16-bit, 24Khz, mono

    _byteArray.resize(rawAudioByteArray.size() / 2);

    int numSourceSamples = rawAudioByteArray.size() / sizeof(int16_t);
    int16_t* sourceSamples = (int16_t*) rawAudioByteArray.data();
    int16_t* destinationSamples = (int16_t*) _byteArray.data();

    
    if (_isStereo) {
        for (int i = 0; i < numSourceSamples; i += 4) {
            destinationSamples[i / 2] = (sourceSamples[i] / 2) + (sourceSamples[i + 2] / 2);
            destinationSamples[(i / 2) + 1] = (sourceSamples[i + 1] / 2) + (sourceSamples[i + 3] / 2);
        }
    } else {
        for (int i = 1; i < numSourceSamples; i += 2) {
            if (i + 1 >= numSourceSamples) {
                destinationSamples[(i - 1) / 2] = (sourceSamples[i - 1] / 2) + (sourceSamples[i] / 2);
            } else {
                destinationSamples[(i - 1) / 2] = (sourceSamples[i - 1] / 4) + (sourceSamples[i] / 2) + (sourceSamples[i + 1] / 4);
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
            qDebug() << "Currently not supporting big-endian audio files.";
            return;
        }

        if (strncmp(fileHeader.riff.type, "WAVE", 4) != 0
            || strncmp(fileHeader.wave.descriptor.id, "fmt", 3) != 0) {
            qDebug() << "Not a WAVE Audio file.";
            return;
        }

        // added the endianess check as an extra level of security

        if (qFromLittleEndian<quint16>(fileHeader.wave.audioFormat) != 1) {
            qDebug() << "Currently not supporting non PCM audio files.";
            return;
        }
        if (qFromLittleEndian<quint16>(fileHeader.wave.numChannels) != 1) {
            qDebug() << "Currently not supporting stereo audio files.";
            return;
        }
        if (qFromLittleEndian<quint16>(fileHeader.wave.bitsPerSample) != 16) {
            qDebug() << "Currently not supporting non 16bit audio files.";
            return;
        }
        if (qFromLittleEndian<quint32>(fileHeader.wave.sampleRate) != 48000) {
            qDebug() << "Currently not supporting non 48KHz audio files.";
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
                qDebug() << "Could not read wav audio data header.";
                return;
            }
        }

        // Now pull out the data
        quint32 outputAudioByteArraySize = qFromLittleEndian<quint32>(dataHeader.descriptor.size);
        outputAudioByteArray.resize(outputAudioByteArraySize);
        if (waveStream.readRawData(outputAudioByteArray.data(), outputAudioByteArraySize) != (int)outputAudioByteArraySize) {
            qDebug() << "Error reading WAV file";
        }

    } else {
        qDebug() << "Could not read wav audio file header.";
        return;
    }
}
