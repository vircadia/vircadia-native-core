//
//  RawMixedAudioStream.h
//  libraries/audio/src
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RawMixedAudioStream_h
#define hifi_RawMixedAudioStream_h

#include "InboundAudioStream.h"
#include "PacketHeaders.h"

class RawMixedAudioStream  : public InboundAudioStream {
    Q_OBJECT
public:
    RawMixedAudioStream (int numFrameSamples, int numFramesCapacity, bool dynamicJitterBuffers, int staticDesiredJitterBufferFrames, int maxFramesOverDesired, bool useStDevForJitterCalc);

signals:

    void processSamples(const QByteArray& inputBuffer, QByteArray& outputBuffer);

protected:
    int parseStreamProperties(PacketType type, const QByteArray& packetAfterSeqNum, int& numAudioSamples);
    int parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties, int numAudioSamples);
};

#endif // hifi_RawMixedAudioStream_h
