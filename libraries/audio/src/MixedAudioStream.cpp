//
//  MixedAudioStream.cpp
//  libraries/audio/src
//
//  Created by Yixin Wang on 8/4/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MixedAudioStream.h"

#include "AudioConstants.h"

static const int STEREO_FACTOR = 2;

MixedAudioStream::MixedAudioStream(int numFramesCapacity, int numStaticJitterFrames) :
    InboundAudioStream(STEREO_FACTOR, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL,
        numFramesCapacity, numStaticJitterFrames) {}
