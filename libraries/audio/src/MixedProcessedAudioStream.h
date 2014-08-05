//
//  MixedProcessedAudioStream.h
//  libraries/audio/src
//
//  Created by Yixin Wang on 8/4/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MixedProcessedAudioStream_h
#define hifi_MixedProcessedAudioStream_h

#include "MixedAudioStream.h"

class MixedProcessedAudioStream  : public MixedAudioStream {
    Q_OBJECT
public:
    MixedProcessedAudioStream (int numFrameSamples, int numFramesCapacity, bool dynamicJitterBuffers, int staticDesiredJitterBufferFrames, int maxFramesOverDesired, bool useStDevForJitterCalc);

signals:

    void processSamples(const QByteArray& inputBuffer, QByteArray& outputBuffer);

protected:
    int parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties, int numAudioSamples);
};

#endif // hifi_MixedProcessedAudioStream_h
