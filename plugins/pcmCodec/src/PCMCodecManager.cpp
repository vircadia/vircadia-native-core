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


void PCMCodec::decode(const QByteArray& encodedBuffer, QByteArray& decodedBuffer) {
    decodedBuffer = encodedBuffer;
}

void PCMCodec::encode(const QByteArray& decodedBuffer, QByteArray& encodedBuffer) {
    encodedBuffer = decodedBuffer;
}


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

void zLibCodec::decode(const QByteArray& encodedBuffer, QByteArray& decodedBuffer) {
    decodedBuffer = qUncompress(encodedBuffer);
}

void zLibCodec::encode(const QByteArray& decodedBuffer, QByteArray& encodedBuffer) {
    encodedBuffer = qCompress(decodedBuffer);
}
