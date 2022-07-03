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

#include "PCMCodecManager.h"

#include <QtCore/QCoreApplication>

#include <PerfStat.h>

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

const QString PCMCodecManager::getName() const {
    return PCMCodec::getName();
}



Encoder* PCMCodecManager::createEncoder(int sampleRate, int numChannels) {
    return PCMCodec::createEncoder(sampleRate, numChannels);
}

Decoder* PCMCodecManager::createDecoder(int sampleRate, int numChannels) {
    return PCMCodec::createDecoder(sampleRate, numChannels);
}

void PCMCodecManager::releaseEncoder(Encoder* encoder) {
    PCMCodec::releaseEncoder(encoder);
}

void PCMCodecManager::releaseDecoder(Decoder* decoder) {
    PCMCodec::releaseDecoder(decoder);
}

void zLibCodecManager::init() {
}

void zLibCodecManager::deinit() {
}

bool zLibCodecManager::activate() {
    CodecPlugin::activate();
    return true;
}

void zLibCodecManager::deactivate() {
    CodecPlugin::deactivate();
}

bool zLibCodecManager::isSupported() const {
    return true;
}

const QString zLibCodecManager::getName() const {
    return zLibCodec::getName();
}

Encoder* zLibCodecManager::createEncoder(int sampleRate, int numChannels) {
    return zLibCodec::createEncoder(sampleRate, numChannels);
}

Decoder* zLibCodecManager::createDecoder(int sampleRate, int numChannels) {
    return zLibCodec::createDecoder(sampleRate, numChannels);
}

void zLibCodecManager::releaseEncoder(Encoder* encoder) {
    return zLibCodec::releaseEncoder(encoder);
}

void zLibCodecManager::releaseDecoder(Decoder* decoder) {
    return zLibCodec::releaseDecoder(decoder);
}

