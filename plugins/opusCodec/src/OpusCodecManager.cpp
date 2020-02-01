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

#include "OpusEncoder.h"
#include "OpusDecoder.h"

const char* AthenaOpusCodec::NAME { "opus" };

void AthenaOpusCodec::init() {
}

void AthenaOpusCodec::deinit() {
}

bool AthenaOpusCodec::activate() {
    CodecPlugin::activate();
    return true;
}

void AthenaOpusCodec::deactivate() {
    CodecPlugin::deactivate();
}


bool AthenaOpusCodec::isSupported() const {
    return true;
}


Encoder* AthenaOpusCodec::createEncoder(int sampleRate, int numChannels) {
    return new AthenaOpusEncoder(sampleRate, numChannels);
}

Decoder* AthenaOpusCodec::createDecoder(int sampleRate, int numChannels) {
    return new AthenaOpusDecoder(sampleRate, numChannels);
}

void AthenaOpusCodec::releaseEncoder(Encoder* encoder) {
    delete encoder;
}

void AthenaOpusCodec::releaseDecoder(Decoder* decoder) {
    delete decoder;
}
