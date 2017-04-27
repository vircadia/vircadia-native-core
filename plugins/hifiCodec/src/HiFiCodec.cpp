//
//  HiFiCodec.cpp
//  plugins/hifiCodec/src
//
//  Created by Brad Hefta-Gaub on 7/10/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <qapplication.h>

#include <AudioCodec.h>
#include <AudioConstants.h>

#include "HiFiCodec.h"

const char* HiFiCodec::NAME { "hifiAC" };

void HiFiCodec::init() {
}

void HiFiCodec::deinit() {
}

bool HiFiCodec::activate() {
    CodecPlugin::activate();
    return true;
}

void HiFiCodec::deactivate() {
    CodecPlugin::deactivate();
}


bool HiFiCodec::isSupported() const {
    return true;
}

class HiFiEncoder : public Encoder, public AudioEncoder {
public:
    HiFiEncoder(int sampleRate, int numChannels) : AudioEncoder(sampleRate, numChannels) { 
        _encodedSize = (AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL * sizeof(int16_t) * numChannels) / 4;  // codec reduces by 1/4th
    }

    virtual void encode(const QByteArray& decodedBuffer, QByteArray& encodedBuffer) override {
        encodedBuffer.resize(_encodedSize);
        AudioEncoder::process((const int16_t*)decodedBuffer.constData(), (int16_t*)encodedBuffer.data(), AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);
    }
private:
    int _encodedSize;
};

class HiFiDecoder : public Decoder, public AudioDecoder {
public:
    HiFiDecoder(int sampleRate, int numChannels) : AudioDecoder(sampleRate, numChannels) { 
        _decodedSize = AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL * sizeof(int16_t) * numChannels;
    }

    virtual void decode(const QByteArray& encodedBuffer, QByteArray& decodedBuffer) override {
        decodedBuffer.resize(_decodedSize);
        AudioDecoder::process((const int16_t*)encodedBuffer.constData(), (int16_t*)decodedBuffer.data(), AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL, true);
    }

    virtual void lostFrame(QByteArray& decodedBuffer) override {
        decodedBuffer.resize(_decodedSize);
        // this performs packet loss interpolation
        AudioDecoder::process(nullptr, (int16_t*)decodedBuffer.data(), AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL, false);
    }
private:
    int _decodedSize;
};

Encoder* HiFiCodec::createEncoder(int sampleRate, int numChannels) {
    return new HiFiEncoder(sampleRate, numChannels);
}

Decoder* HiFiCodec::createDecoder(int sampleRate, int numChannels) {
    return new HiFiDecoder(sampleRate, numChannels);
}

void HiFiCodec::releaseEncoder(Encoder* encoder) {
    delete encoder;
}

void HiFiCodec::releaseDecoder(Decoder* decoder) {
    delete decoder;
}
