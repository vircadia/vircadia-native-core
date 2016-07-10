//
//  PCMCodec.cpp
//  plugins/pcmCodec/src
//
//  Created by Brad Hefta-Gaub on 6/9/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <qapplication.h>

#include <PerfStat.h>

#include "PCMCodecManager.h"

const QString PCMCodec::NAME = "pcm";

void PCMCodec::init() {
}

void PCMCodec::deinit() {
}

bool PCMCodec::activate() {
    CodecPlugin::activate();
    return true;
}

void PCMCodec::deactivate() {
    CodecPlugin::deactivate();
}


bool PCMCodec::isSupported() const {
    return true;
}

class PCMEncoder : public Encoder {
public:
    virtual void encode(const QByteArray& decodedBuffer, QByteArray& encodedBuffer) override {
        encodedBuffer = decodedBuffer;
    }
};

class PCMDecoder : public Decoder {
public:
    virtual void decode(const QByteArray& encodedBuffer, QByteArray& decodedBuffer) override {
        decodedBuffer = encodedBuffer;
    }

    virtual void trackLostFrames(int numFrames)  override { }
};

Encoder* PCMCodec::createEncoder(int sampleRate, int numChannels) {
    return new PCMEncoder();
}

Decoder* PCMCodec::createDecoder(int sampleRate, int numChannels) {
    return new PCMDecoder();
}

void PCMCodec::releaseEncoder(Encoder* encoder) {
    delete encoder;
}

void PCMCodec::releaseDecoder(Decoder* decoder) {
    delete decoder;
}


class zLibEncoder : public Encoder {
public:
    virtual void encode(const QByteArray& decodedBuffer, QByteArray& encodedBuffer) override {
        encodedBuffer = qCompress(decodedBuffer);
    }
};

class zLibDecoder : public Decoder {
public:
    virtual void decode(const QByteArray& encodedBuffer, QByteArray& decodedBuffer) override {
        decodedBuffer = qUncompress(encodedBuffer);
    }

    virtual void trackLostFrames(int numFrames)  override { }
};

const QString zLibCodec::NAME = "zlib";

void zLibCodec::init() {
}

void zLibCodec::deinit() {
}

bool zLibCodec::activate() {
    CodecPlugin::activate();
    return true;
}

void zLibCodec::deactivate() {
    CodecPlugin::deactivate();
}


bool zLibCodec::isSupported() const {
    return true;
}

Encoder* zLibCodec::createEncoder(int sampleRate, int numChannels) {
    return new zLibEncoder();
}

Decoder* zLibCodec::createDecoder(int sampleRate, int numChannels) {
    return new zLibDecoder();
}

void zLibCodec::releaseEncoder(Encoder* encoder) {
    delete encoder;
}

void zLibCodec::releaseDecoder(Decoder* decoder) {
    delete decoder;
}

