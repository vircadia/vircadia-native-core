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

#include "Sound.h"

#include <stdint.h>

#include <glm/glm.hpp>

#include <QRunnable>
#include <QThreadPool>
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

#include "flump3dec.h"

QScriptValue soundSharedPointerToScriptValue(QScriptEngine* engine, const SharedSoundPointer& in) {
    return engine->newQObject(new SoundScriptingInterface(in), QScriptEngine::ScriptOwnership);
}

void soundSharedPointerFromScriptValue(const QScriptValue& object, SharedSoundPointer& out) {
    if (auto soundInterface = qobject_cast<SoundScriptingInterface*>(object.toQObject())) {
        out = soundInterface->getSound();
    }
}

SoundScriptingInterface::SoundScriptingInterface(const SharedSoundPointer& sound) : _sound(sound) {
    // During shutdown we can sometimes get an empty sound pointer back
    if (_sound) {
        QObject::connect(_sound.data(), &Sound::ready, this, &SoundScriptingInterface::ready);
    }
}

Sound::Sound(const QUrl& url, bool isStereo, bool isAmbisonic) :
    Resource(url),
    _isStereo(isStereo),
    _isAmbisonic(isAmbisonic),
    _isReady(false)
{
}

void Sound::downloadFinished(const QByteArray& data) {
    // this is a QRunnable, will delete itself after it has finished running
    SoundProcessor* soundProcessor = new SoundProcessor(_url, data, _isStereo, _isAmbisonic);
    connect(soundProcessor, &SoundProcessor::onSuccess, this, &Sound::soundProcessSuccess);
    connect(soundProcessor, &SoundProcessor::onError, this, &Sound::soundProcessError);
    QThreadPool::globalInstance()->start(soundProcessor);
}

void Sound::soundProcessSuccess(QByteArray data, bool stereo, bool ambisonic, float duration) {

    qCDebug(audio) << "Setting ready state for sound file" << _url.toDisplayString();

    _byteArray = data;
    _isStereo = stereo;
    _isAmbisonic = ambisonic;
    _duration = duration;
    _isReady = true;
    finishedLoading(true);

    emit ready();
}

void Sound::soundProcessError(int error, QString str) {
    qCCritical(audio) << "Failed to process sound file" << _url.toDisplayString() << "code =" << error << str;
    emit failed(QNetworkReply::UnknownContentError);
    finishedLoading(false);
}

void SoundProcessor::run() {

    qCDebug(audio) << "Processing sound file" << _url.toDisplayString();

    // replace our byte array with the downloaded data
    QByteArray rawAudioByteArray = QByteArray(_data);
    QString fileName = _url.fileName().toLower();

    static const QString WAV_EXTENSION = ".wav";
    static const QString MP3_EXTENSION = ".mp3";
    static const QString RAW_EXTENSION = ".raw";

    if (fileName.endsWith(WAV_EXTENSION)) {

        QByteArray outputAudioByteArray;

        int sampleRate = interpretAsWav(rawAudioByteArray, outputAudioByteArray);
        if (sampleRate == 0) {
            qCWarning(audio) << "Unsupported WAV file type";
            emit onError(300, "Failed to load sound file, reason: unsupported WAV file type");
            return;
        }

        downSample(outputAudioByteArray, sampleRate);

    } else if (fileName.endsWith(MP3_EXTENSION)) {

        QByteArray outputAudioByteArray;

        int sampleRate = interpretAsMP3(rawAudioByteArray, outputAudioByteArray);
        if (sampleRate == 0) {
            qCWarning(audio) << "Unsupported MP3 file type";
            emit onError(300, "Failed to load sound file, reason: unsupported MP3 file type");
            return;
        }

        downSample(outputAudioByteArray, sampleRate);

    } else if (fileName.endsWith(RAW_EXTENSION)) {
        // check if this was a stereo raw file
        // since it's raw the only way for us to know that is if the file was called .stereo.raw
        if (fileName.toLower().endsWith("stereo.raw")) {
            _isStereo = true;
            qCDebug(audio) << "Processing sound of" << rawAudioByteArray.size() << "bytes from" << _url << "as stereo audio file.";
        }

        // Process as 48khz RAW file
        downSample(rawAudioByteArray, 48000);

    } else {
        qCWarning(audio) << "Unknown sound file type";
        emit onError(300, "Failed to load sound file, reason: unknown sound file type");
        return;
    }

    emit onSuccess(_data, _isStereo, _isAmbisonic, _duration);
}

void SoundProcessor::downSample(const QByteArray& rawAudioByteArray, int sampleRate) {

    // we want to convert it to the format that the audio-mixer wants
    // which is signed, 16-bit, 24Khz

    if (sampleRate == AudioConstants::SAMPLE_RATE) {
        // no resampling needed
        _data = rawAudioByteArray;
    } else {

        int numChannels = _isAmbisonic ? AudioConstants::AMBISONIC : (_isStereo ? AudioConstants::STEREO : AudioConstants::MONO);
        AudioSRC resampler(sampleRate, AudioConstants::SAMPLE_RATE, numChannels);

        // resize to max possible output
        int numSourceFrames = rawAudioByteArray.size() / (numChannels * sizeof(AudioConstants::AudioSample));
        int maxDestinationFrames = resampler.getMaxOutput(numSourceFrames);
        int maxDestinationBytes = maxDestinationFrames * numChannels * sizeof(AudioConstants::AudioSample);
        _data.resize(maxDestinationBytes);

        int numDestinationFrames = resampler.render((int16_t*)rawAudioByteArray.data(), 
                                                    (int16_t*)_data.data(),
                                                    numSourceFrames);

        // truncate to actual output
        int numDestinationBytes = numDestinationFrames * numChannels * sizeof(AudioConstants::AudioSample);
        _data.resize(numDestinationBytes);
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

static const int WAVEFORMAT_PCM = 1;
static const int WAVEFORMAT_EXTENSIBLE = 0xfffe;

struct WAVEFormat {
    quint16     audioFormat;    // Format type: 1=PCM, 257=Mu-Law, 258=A-Law, 259=ADPCM
    quint16     numChannels;    // Number of channels: 1=mono, 2=stereo
    quint32     sampleRate;
    quint32     byteRate;       // Sample rate * Number of Channels * Bits per sample / 8
    quint16     blockAlign;     // (Number of Channels * Bits per sample) / 8.1
    quint16     bitsPerSample;
};

// returns wavfile sample rate, used for resampling
int SoundProcessor::interpretAsWav(const QByteArray& inputAudioByteArray, QByteArray& outputAudioByteArray) {

    // Create a data stream to analyze the data
    QDataStream waveStream(const_cast<QByteArray *>(&inputAudioByteArray), QIODevice::ReadOnly);

    // Read the "RIFF" chunk
    RIFFHeader riff;
    if (waveStream.readRawData((char*)&riff, sizeof(RIFFHeader)) != sizeof(RIFFHeader)) {
        qCWarning(audio) << "Not a valid WAVE file.";
        return 0;
    }

    // Parse the "RIFF" chunk
    if (strncmp(riff.descriptor.id, "RIFF", 4) == 0) {
        waveStream.setByteOrder(QDataStream::LittleEndian);
    } else {
        qCWarning(audio) << "Currently not supporting big-endian audio files.";
        return 0;
    }
    if (strncmp(riff.type, "WAVE", 4) != 0) {
        qCWarning(audio) << "Not a valid WAVE file.";
        return 0;
    }

    // Read chunks until the "fmt " chunk is found
    chunk fmt;
    while (true) {
        if (waveStream.readRawData((char*)&fmt, sizeof(chunk)) != sizeof(chunk)) {
            qCWarning(audio) << "Not a valid WAVE file.";
            return 0;
        }
        if (strncmp(fmt.id, "fmt ", 4) == 0) {
            break;
        }
        waveStream.skipRawData(qFromLittleEndian<quint32>(fmt.size));   // next chunk
    }

    // Read the "fmt " chunk
    WAVEFormat wave;
    if (waveStream.readRawData((char*)&wave, sizeof(WAVEFormat)) != sizeof(WAVEFormat)) {
        qCWarning(audio) << "Not a valid WAVE file.";
        return 0;
    }

    // Parse the "fmt " chunk
    if (qFromLittleEndian<quint16>(wave.audioFormat) != WAVEFORMAT_PCM &&
        qFromLittleEndian<quint16>(wave.audioFormat) != WAVEFORMAT_EXTENSIBLE) {
        qCWarning(audio) << "Currently not supporting non PCM audio files.";
        return 0;
    }
    if (qFromLittleEndian<quint16>(wave.numChannels) == 2) {
        _isStereo = true;
    } else if (qFromLittleEndian<quint16>(wave.numChannels) == 4) {
        _isAmbisonic = true;
    } else if (qFromLittleEndian<quint16>(wave.numChannels) != 1) {
        qCWarning(audio) << "Currently not supporting audio files with other than 1/2/4 channels.";
        return 0;
    }
    if (qFromLittleEndian<quint16>(wave.bitsPerSample) != 16) {
        qCWarning(audio) << "Currently not supporting non 16bit audio files.";
        return 0;
    }

    // Skip any extra data in the "fmt " chunk
    waveStream.skipRawData(qFromLittleEndian<quint32>(fmt.size) - sizeof(WAVEFormat));

    // Read chunks until the "data" chunk is found
    chunk data;
    while (true) {
        if (waveStream.readRawData((char*)&data, sizeof(chunk)) != sizeof(chunk)) {
            qCWarning(audio) << "Not a valid WAVE file.";
            return 0;
        }
        if (strncmp(data.id, "data", 4) == 0) {
            break;
        }
        waveStream.skipRawData(qFromLittleEndian<quint32>(data.size));  // next chunk
    }

    // Read the "data" chunk
    quint32 outputAudioByteArraySize = qFromLittleEndian<quint32>(data.size);
    outputAudioByteArray.resize(outputAudioByteArraySize);
    if (waveStream.readRawData(outputAudioByteArray.data(), outputAudioByteArraySize) != (int)outputAudioByteArraySize) {
        qCWarning(audio) << "Error reading WAV file";
        return 0;
    }

    _duration = (float)(outputAudioByteArraySize / (wave.sampleRate * wave.numChannels * wave.bitsPerSample / 8.0f));
    return wave.sampleRate;
}

// returns MP3 sample rate, used for resampling
int SoundProcessor::interpretAsMP3(const QByteArray& inputAudioByteArray, QByteArray& outputAudioByteArray) {
    using namespace flump3dec;

    static const int MP3_SAMPLES_MAX = 1152;
    static const int MP3_CHANNELS_MAX = 2;
    static const int MP3_BUFFER_SIZE = MP3_SAMPLES_MAX * MP3_CHANNELS_MAX * sizeof(int16_t);
    uint8_t mp3Buffer[MP3_BUFFER_SIZE];

    // create bitstream
    Bit_stream_struc *bitstream = bs_new();
    if (bitstream == nullptr) {
        return 0;
    }

    // create decoder
    mp3tl *decoder = mp3tl_new(bitstream, MP3TL_MODE_16BIT);
    if (decoder == nullptr) {
        bs_free(bitstream);
        return 0;
    }

    // initialize
    bs_set_data(bitstream, (uint8_t*)inputAudioByteArray.data(), inputAudioByteArray.size());
    int frameCount = 0;
    int sampleRate = 0;
    int numChannels = 0;

    // skip ID3 tag, if present
    Mp3TlRetcode result = mp3tl_skip_id3(decoder);

    while (!(result == MP3TL_ERR_NO_SYNC || result == MP3TL_ERR_NEED_DATA)) {

        mp3tl_sync(decoder);

        // find MP3 header
        const fr_header *header = nullptr;
        result = mp3tl_decode_header(decoder, &header);

        if (result == MP3TL_ERR_OK) {

            if (frameCount++ == 0) {

                qCDebug(audio) << "Decoding MP3 with bitrate =" << header->bitrate
                               << "sample rate =" << header->sample_rate
                               << "channels =" << header->channels;

                // save header info
                sampleRate = header->sample_rate;
                numChannels = header->channels;

                // skip Xing header, if present
                result = mp3tl_skip_xing(decoder, header);
            }

            // decode MP3 frame
            if (result == MP3TL_ERR_OK) {

                result = mp3tl_decode_frame(decoder, mp3Buffer, MP3_BUFFER_SIZE);

                // fill bad frames with silence
                int len = header->frame_samples * header->channels * sizeof(int16_t);
                if (result == MP3TL_ERR_BAD_FRAME) {
                    memset(mp3Buffer, 0, len);
                }

                if (result == MP3TL_ERR_OK || result == MP3TL_ERR_BAD_FRAME) {
                    outputAudioByteArray.append((char*)mp3Buffer, len);
                }
            }
        }
    }

    // free decoder
    mp3tl_free(decoder);

    // free bitstream
    bs_free(bitstream);

    int outputAudioByteArraySize = outputAudioByteArray.size();
    if (outputAudioByteArraySize == 0) {
        qCWarning(audio) << "Error decoding MP3 file";
        return 0;
    }

    _isStereo = (numChannels == 2);
    _isAmbisonic = false;
    _duration = (float)outputAudioByteArraySize / (sampleRate * numChannels * sizeof(int16_t));
    return sampleRate;
}
