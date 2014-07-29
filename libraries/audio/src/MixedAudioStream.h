//
//  MixedAudioStream.h
//  libraries/audio/src
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MixedAudioStream_h
#define hifi_MixedAudioStream_h

#include "InboundAudioStream.h"
#include "PacketHeaders.h"

class MixedAudioStream : public InboundAudioStream {
public:
    MixedAudioStream(int numFrameSamples, int numFramesCapacity, bool dynamicJitterBuffers, int maxFramesOverDesired, bool useStDevForJitterCalc = false);

    float getNextOutputFrameLoudness() const { return _ringBuffer.getNextOutputFrameLoudness(); }

protected:
    int parseStreamProperties(PacketType type, const QByteArray& packetAfterSeqNum, int& numAudioSamples);
    int parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties, int numAudioSamples);
};

#endif // hifi_MixedAudioStream_h
