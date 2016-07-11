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

#include <AudioCodec.h> // should be from the external...

#include <PerfStat.h>

#include "HiFiCodec.h"

const QString HiFiCodec::NAME = "hifiAC";

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

class HiFiEncoder : public Encoder {
public:
    virtual void encode(const QByteArray& decodedBuffer, QByteArray& encodedBuffer) override {
        encodedBuffer = decodedBuffer;
    }
};

class HiFiDecoder : public Decoder {
public:
    virtual void decode(const QByteArray& encodedBuffer, QByteArray& decodedBuffer) override {
        decodedBuffer = encodedBuffer;
    }

    virtual void trackLostFrames(int numFrames)  override { }
};

Encoder* HiFiCodec::createEncoder(int sampleRate, int numChannels) {
    return new HiFiEncoder();
}

Decoder* HiFiCodec::createDecoder(int sampleRate, int numChannels) {
    return new HiFiDecoder();
}

void HiFiCodec::releaseEncoder(Encoder* encoder) {
    delete encoder;
}

void HiFiCodec::releaseDecoder(Decoder* decoder) {
    delete decoder;
}
