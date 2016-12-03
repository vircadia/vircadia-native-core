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
#include "AudioLogging.h"
#include "AudioSRC.h"

#include "Sound.h"

QScriptValue soundSharedPointerToScriptValue(QScriptEngine* engine, const SharedSoundPointer& in) {
    return engine->newQObject(new SoundScriptingInterface(in), QScriptEngine::ScriptOwnership);
}

void soundSharedPointerFromScriptValue(const QScriptValue& object, SharedSoundPointer& out) {
    if (auto soundInterface = qobject_cast<SoundScriptingInterface*>(object.toQObject())) {
        out = soundInterface->getSound();
    }
}

SoundScriptingInterface::SoundScriptingInterface(SharedSoundPointer sound) : _sound(sound) {
    QObject::connect(sound.data(), &Sound::ready, this, &SoundScriptingInterface::ready);
}

Sound::Sound(const QUrl& url, bool isStereo, bool isAmbisonic) :
    Resource(url),
    _isStereo(isStereo),
    _isAmbisonic(isAmbisonic),
    _isReady(false)
{

}

void Sound::downloadFinished(const QByteArray& data) {
    // replace our byte array with the downloaded data
    QByteArray rawAudioByteArray = QByteArray(data);
    QString fileName = getURL().fileName().toLower();

    static const QString WAV_EXTENSION = ".wav";
    static const QString RAW_EXTENSION = ".raw";
    if (fileName.endsWith(WAV_EXTENSION)) {

        QByteArray outputAudioByteArray;

        int sampleRate = interpretAsWav(rawAudioByteArray, outputAudioByteArray);
        if (sampleRate != 0) {
            downSample(outputAudioByteArray, sampleRate);
        }
    } else if (fileName.endsWith(RAW_EXTENSION)) {
        // check if this was a stereo raw file
        // since it's raw the only way for us to know that is if the file was called .stereo.raw
        if (fileName.toLower().endsWith("stereo.raw")) {
            _isStereo = true;
            qCDebug(audio) << "Processing sound of" << rawAudioByteArray.size() << "bytes from" << getURL() << "as stereo audio file.";
        }

        // Process as 48khz RAW file
        downSample(rawAudioByteArray, 48000);
    } else {
        qCDebug(audio) << "Unknown sound file type";
    }

    finishedLoading(true);

    _isReady = true;
    emit ready();
}

void Sound::downSample(const QByteArray& rawAudioByteArray, int sampleRate) {

    // we want to convert it to the format that the audio-mixer wants
    // which is signed, 16-bit, 24Khz

    if (sampleRate == AudioConstants::SAMPLE_RATE) {

        // no resampling needed
        _byteArray = rawAudioByteArray;

    } else if (_isAmbisonic) {

        // FIXME: add a proper Ambisonic resampler!
        int numChannels = 4;
        AudioSRC resampler[4] { {sampleRate, AudioConstants::SAMPLE_RATE, 1}, 
                                {sampleRate, AudioConstants::SAMPLE_RATE, 1}, 
                                {sampleRate, AudioConstants::SAMPLE_RATE, 1}, 
                                {sampleRate, AudioConstants::SAMPLE_RATE, 1} };

        // resize to max possible output
        int numSourceFrames = rawAudioByteArray.size() / (numChannels * sizeof(AudioConstants::AudioSample));
        int maxDestinationFrames = resampler[0].getMaxOutput(numSourceFrames);
        int maxDestinationBytes = maxDestinationFrames * numChannels * sizeof(AudioConstants::AudioSample);
        _byteArray.resize(maxDestinationBytes);

        int numDestinationFrames = 0;

        // iterate over channels
        int16_t* srcBuffer = new int16_t[numSourceFrames];
        int16_t* dstBuffer = new int16_t[maxDestinationFrames];
        for (int ch = 0; ch < 4; ch++) {

            int16_t* src = (int16_t*)rawAudioByteArray.data();
            int16_t* dst = (int16_t*)_byteArray.data();

            // deinterleave samples
            for (int i = 0; i < numSourceFrames; i++) {
                srcBuffer[i] = src[4*i + ch];
            }

            // resample one channel
            numDestinationFrames = resampler[ch].render(srcBuffer, dstBuffer, numSourceFrames);

            // reinterleave samples
            for (int i = 0; i < numDestinationFrames; i++) {
                dst[4*i + ch] = dstBuffer[i];
            }
        }
        delete[] srcBuffer;
        delete[] dstBuffer;

        // truncate to actual output
        int numDestinationBytes = numDestinationFrames * numChannels * sizeof(AudioConstants::AudioSample);
        _byteArray.resize(numDestinationBytes);

    } else {

        int numChannels = _isStereo ? 2 : 1;
        AudioSRC resampler(sampleRate, AudioConstants::SAMPLE_RATE, numChannels);

        // resize to max possible output
        int numSourceFrames = rawAudioByteArray.size() / (numChannels * sizeof(AudioConstants::AudioSample));
        int maxDestinationFrames = resampler.getMaxOutput(numSourceFrames);
        int maxDestinationBytes = maxDestinationFrames * numChannels * sizeof(AudioConstants::AudioSample);
        _byteArray.resize(maxDestinationBytes);

        int numDestinationFrames = resampler.render((int16_t*)rawAudioByteArray.data(), 
                                                    (int16_t*)_byteArray.data(), 
                                                    numSourceFrames);

        // truncate to actual output
        int numDestinationBytes = numDestinationFrames * numChannels * sizeof(AudioConstants::AudioSample);
        _byteArray.resize(numDestinationBytes);
    }
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

// returns wavfile sample rate, used for resampling
int Sound::interpretAsWav(const QByteArray& inputAudioByteArray, QByteArray& outputAudioByteArray) {

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
            return 0;
        }

        if (strncmp(fileHeader.riff.type, "WAVE", 4) != 0
            || strncmp(fileHeader.wave.descriptor.id, "fmt", 3) != 0) {
            qCDebug(audio) << "Not a WAVE Audio file.";
            return 0;
        }

        // added the endianess check as an extra level of security

        if (qFromLittleEndian<quint16>(fileHeader.wave.audioFormat) != 1) {
            qCDebug(audio) << "Currently not supporting non PCM audio files.";
            return 0;
        }
        if (qFromLittleEndian<quint16>(fileHeader.wave.numChannels) == 2) {
            _isStereo = true;
        } else if (qFromLittleEndian<quint16>(fileHeader.wave.numChannels) == 4) {
            _isAmbisonic = true;
        } else if (qFromLittleEndian<quint16>(fileHeader.wave.numChannels) != 1) {
            qCDebug(audio) << "Currently not support audio files with other than 1/2/4 channels.";
            return 0;
        }

        if (qFromLittleEndian<quint16>(fileHeader.wave.bitsPerSample) != 16) {
            qCDebug(audio) << "Currently not supporting non 16bit audio files.";
            return 0;
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
                return 0;
            }
        }

        // Now pull out the data
        quint32 outputAudioByteArraySize = qFromLittleEndian<quint32>(dataHeader.descriptor.size);
        outputAudioByteArray.resize(outputAudioByteArraySize);
        if (waveStream.readRawData(outputAudioByteArray.data(), outputAudioByteArraySize) != (int)outputAudioByteArraySize) {
            qCDebug(audio) << "Error reading WAV file";
            return 0;
        }

        _duration = (float) (outputAudioByteArraySize / (fileHeader.wave.sampleRate * fileHeader.wave.numChannels * fileHeader.wave.bitsPerSample / 8.0f));
        return fileHeader.wave.sampleRate;

    } else {
        qCDebug(audio) << "Could not read wav audio file header.";
        return 0;
    }
}
