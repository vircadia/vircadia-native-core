//
//  CodecPlugin.h
//  plugins/src/plugins
//
//  Created by Brad Hefta-Gaub on 6/9/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "Plugin.h"

class Encoder {
public:
    virtual ~Encoder() { }
    virtual void encode(const QByteArray& decodedBuffer, QByteArray& encodedBuffer) = 0;
};

class Decoder {
public:
    virtual ~Decoder() { }
    virtual void decode(const QByteArray& encodedBuffer, QByteArray& decodedBuffer) = 0;

    // numFrames - number of samples (mono) or sample-pairs (stereo)
    virtual void trackLostFrames(int numFrames) = 0;
};

class CodecPlugin : public Plugin {
public:
    virtual Encoder* createEncoder(int sampleRate, int numChannels) = 0;
    virtual Decoder* createDecoder(int sampleRate, int numChannels) = 0;
    virtual void releaseEncoder(Encoder* encoder) = 0;
    virtual void releaseDecoder(Decoder* decoder) = 0;
};
