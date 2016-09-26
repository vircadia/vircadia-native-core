//
//  MixedAudioStream.h
//  libraries/audio/src
//
//  Created by Yixin Wang on 8/4/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MixedAudioStream_h
#define hifi_MixedAudioStream_h

#include "InboundAudioStream.h"

class MixedAudioStream : public InboundAudioStream {
public:
    MixedAudioStream(int numFrameSamples, int numFramesCapacity, int numStaticJitterFrames = -1);

    float getNextOutputFrameLoudness() const { return _ringBuffer.getNextOutputFrameLoudness(); }
};

#endif // hifi_MixedAudioStream_h
