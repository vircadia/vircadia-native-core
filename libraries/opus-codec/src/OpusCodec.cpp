//
//  OpusCodec.cpp
//  libraries/opus-codec/src
//
//  Created by Nshan G. on 3 July 2022.
//  Copyright 2019 Michael Bailey
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OpusCodec.h"
#include "OpusEncoder.h"
#include "OpusDecoder.h"

const char* OpusCodec::NAME { "opus" };

Encoder* OpusCodec::createEncoder(int sampleRate, int numChannels) {
    return new OpusEncoder(sampleRate, numChannels);
}

Decoder* OpusCodec::createDecoder(int sampleRate, int numChannels) {
    return new OpusDecoder(sampleRate, numChannels);
}

void OpusCodec::releaseEncoder(Encoder* encoder) {
    delete encoder;
}

void OpusCodec::releaseDecoder(Decoder* decoder) {
    delete decoder;
}
