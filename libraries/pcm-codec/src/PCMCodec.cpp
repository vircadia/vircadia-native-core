//
//  PCMCodec.cpp
//  libraries/pcm-codec/src
//
//  Created by Nshan G. on 3 July 2022.
//  Copyright 2016 High Fidelity, Inc.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PCMCodec.h"

const char* PCMCodec::NAME { "pcm" };

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

