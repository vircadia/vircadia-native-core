//
//  opusCodec.cpp
//  plugins/opusCodec/src
//
//  Created by Michael Bailey on 12/20/2019
//  Copyright 2019 Michael Bailey
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OpusCodecManager.h"

#include <QtCore/QCoreApplication>

#include <PerfStat.h>

// Not sure how many of these are needed, but here they are.
#include <opus/opus.h>
#include <opus/opus_types.h>
#include <opus/opus_defines.h>
#include <opus/opus_multistream.h>
#include <opus/opus_projection.h>

#define FRAME_SIZE 960
#define SAMPLE_RATE 48000
#define CHANNELS 2
#define APPLICATION OPUS_APPLICATION_AUDIO
#define BITRATE 64000

#define MAX_FRAME_SIZE 6*FRAME_SIZE
#define MAX_PACKET_SIZE 3*1276

const char* OpusCodec::NAME { "opus" };

void OpusCodec::init() {
}

void OpusCodec::deinit() {
}

bool OpusCodec::activate() {
    CodecPlugin::activate();
    return true;
}

void OpusCodec::deactivate() {
    CodecPlugin::deactivate();
}


bool OpusCodec::isSupported() const {
    return true;
}


class OpusEncoder : public Encoder {
public:
    OpusEncoder(int sampleRate, int numChannels) {
        
    }

    virtual void encode(const QByteArray& decodedBuffer, QByteArray& encodedBuffer) override {
        encodedBuffer.resize(_encodedSize);
    }

private:
    int _encodedSize;
};


Encoder* OpusCodec::createEncoder(int sampleRate, int numChannels) {
    return new OpusEncoder(sampleRate, numChannels);
}

Decoder* OpusCodec::createDecoder(int sampleRate, int numChannels) {
    return this;
}

void OpusCodec::releaseEncoder(Encoder* encoder) {
    delete encoder;
}

void OpusCodec::releaseDecoder(Decoder* decoder) {
    delete decoder;
}