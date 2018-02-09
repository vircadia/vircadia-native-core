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

#include <QtCore/QCoreApplication>

#include <PerfStat.h>

#include "PCMCodecManager.h"

const char* PCMCodec::NAME { "pcm" };

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



Encoder* PCMCodec::createEncoder(int sampleRate, int numChannels) {
    return this;
}

Decoder* PCMCodec::createDecoder(int sampleRate, int numChannels) {
    return this;
}

void PCMCodec::releaseEncoder(Encoder* encoder) {
    // do nothing
}

void PCMCodec::releaseDecoder(Decoder* decoder) {
    // do nothing
}

const char* zLibCodec::NAME { "zlib" };

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
    return this;
}

Decoder* zLibCodec::createDecoder(int sampleRate, int numChannels) {
    return this;
}

void zLibCodec::releaseEncoder(Encoder* encoder) {
    // do nothing... it wasn't allocated
}

void zLibCodec::releaseDecoder(Decoder* decoder) {
    // do nothing... it wasn't allocated
}

