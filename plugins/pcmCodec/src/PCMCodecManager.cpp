//
//  PCMCodecManager.cpp
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

const QString PCMCodecManager::NAME = "PCM Codec";

void PCMCodecManager::init() {
}

void PCMCodecManager::deinit() {
}

bool PCMCodecManager::activate() {
    CodecPlugin::activate();
    return true;
}

void PCMCodecManager::deactivate() {
    CodecPlugin::deactivate();
}


bool PCMCodecManager::isSupported() const {
    return true;
}


void PCMCodecManager::decode(const QByteArray& encodedBuffer, QByteArray& decodedBuffer) {
    // this codec doesn't actually do anything....
    decodedBuffer = encodedBuffer;

    //decodedBuffer = qUncompress(encodedBuffer);
    //qDebug() << __FUNCTION__ << "from:" << encodedBuffer.size() << " to:" << decodedBuffer.size();
}

void PCMCodecManager::encode(const QByteArray& decodedBuffer, QByteArray& encodedBuffer) {
    // this codec doesn't actually do anything....
    encodedBuffer = decodedBuffer;

    //encodedBuffer = qCompress(decodedBuffer);
    //qDebug() << __FUNCTION__ << "from:" << decodedBuffer.size() << " to:" << encodedBuffer.size();
}
