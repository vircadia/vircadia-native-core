//
//  OpusCodecManager.h
//  plugins/opusCodec/src
//
//  Copyright 2020 Dale Glass
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <PerfStat.h>
#include <QtCore/QLoggingCategory>
#include <AudioConstants.h>

#include "OpusDecoder.h"

static QLoggingCategory decoder("AthenaOpusDecoder");

static QString error_to_string(int error) {
    switch (error) {
        case OPUS_OK:
            return "OK";
        case OPUS_BAD_ARG:
            return "One or more invalid/out of range arguments.";
        case OPUS_BUFFER_TOO_SMALL:
            return "The mode struct passed is invalid.";
        case OPUS_INTERNAL_ERROR:
            return "An internal error was detected.";
        case OPUS_INVALID_PACKET:
            return "The compressed data passed is corrupted.";
        case OPUS_UNIMPLEMENTED:
            return "Invalid/unsupported request number.";
        case OPUS_INVALID_STATE:
            return "An encoder or decoder structure is invalid or already freed.";
        default:
            return QString("Unknown error code: %i").arg(error);
    }
}


AthenaOpusDecoder::AthenaOpusDecoder(int sampleRate, int numChannels) {
    int error;

    _opusSampleRate = sampleRate;
    _opusNumChannels = numChannels;

    _decoder = opus_decoder_create(sampleRate, numChannels, &error);

    if (error != OPUS_OK) {
        qCCritical(decoder) << "Failed to initialize Opus encoder: " << error_to_string(error);
        _decoder = nullptr;
        return;
    }


    qCDebug(decoder) << "Opus decoder initialized, sampleRate = " << sampleRate << "; numChannels = " << numChannels;
}

AthenaOpusDecoder::~AthenaOpusDecoder() {
    if (_decoder) {
        opus_decoder_destroy(_decoder);
    }

}

void AthenaOpusDecoder::decode(const QByteArray &encodedBuffer, QByteArray &decodedBuffer) {
    assert(_decoder);
    PerformanceTimer perfTimer("AthenaOpusDecoder::decode");

    // The audio system encodes and decodes always in fixed size chunks
    int bufferSize = AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL * static_cast<int>(sizeof(int16_t))
        * _opusNumChannels;

    decodedBuffer.resize(bufferSize);
    int bufferFrames = decodedBuffer.size() / _opusNumChannels / static_cast<int>(sizeof(opus_int16));
    int decoded_frames = opus_decode(_decoder, reinterpret_cast<const unsigned char*>(encodedBuffer.data()),
        encodedBuffer.length(), reinterpret_cast<opus_int16*>(decodedBuffer.data()), bufferFrames, 0);

    if (decoded_frames >= 0) {

        if (decoded_frames < bufferFrames) {
            qCWarning(decoder) << "Opus decoder returned " << decoded_frames << ", but " << bufferFrames
                << " were expected!";

            int start = decoded_frames * static_cast<int>(sizeof(int16_t)) * _opusNumChannels;
            memset( &decodedBuffer.data()[start], 0, static_cast<size_t>(decodedBuffer.length() - start));
        } else if (decoded_frames > bufferFrames) {
            // This should never happen
            qCCritical(decoder) << "Opus decoder returned " << decoded_frames << ", but only " << bufferFrames
                << " were expected! Buffer overflow!?";
        }
    } else {
        qCCritical(decoder) << "Failed to decode audio: " << error_to_string(decoded_frames);
        decodedBuffer.fill('\0');
    }

}

void AthenaOpusDecoder::lostFrame(QByteArray &decodedBuffer) {
    assert(_decoder);

    PerformanceTimer perfTimer("AthenaOpusDecoder::lostFrame");

    int bufferSize = AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL * static_cast<int>(sizeof(int16_t))
        * _opusNumChannels;
    decodedBuffer.resize(bufferSize);
    int bufferFrames = decodedBuffer.size() / _opusNumChannels / static_cast<int>(sizeof(opus_int16));

    int decoded_frames = opus_decode(_decoder, nullptr, 0, reinterpret_cast<opus_int16*>(decodedBuffer.data()),
        bufferFrames, 1);

    if (decoded_frames >= 0) {

        if ( decoded_frames < bufferFrames ) {
            qCWarning(decoder) << "Opus decoder returned " << decoded_frames << ", but " << bufferFrames
                << " were expected!";

            int start = decoded_frames * static_cast<int>(sizeof(int16_t)) * _opusNumChannels;
            memset( &decodedBuffer.data()[start], 0, static_cast<size_t>(decodedBuffer.length() - start));
        } else if (decoded_frames > bufferFrames) {
            // This should never happen
            qCCritical(decoder) << "Opus decoder returned " << decoded_frames << ", but only " << bufferFrames
                << " were expected! Buffer overflow!?";
        }

    } else {
        qCCritical(decoder) << "Failed to decode lost frame: " << error_to_string(decoded_frames);
        decodedBuffer.fill('\0');
    }

}
