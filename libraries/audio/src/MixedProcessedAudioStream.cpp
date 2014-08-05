//
//  MixedProcessedAudioStream.cpp
//  libraries/audio/src
//
//  Created by Yixin Wang on 8/4/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MixedProcessedAudioStream.h"

MixedProcessedAudioStream ::MixedProcessedAudioStream (int numFrameSamples, int numFramesCapacity, bool dynamicJitterBuffers, int staticDesiredJitterBufferFrames, int maxFramesOverDesired, bool useStDevForJitterCalc)
    : MixedAudioStream(numFrameSamples, numFramesCapacity, dynamicJitterBuffers, staticDesiredJitterBufferFrames, maxFramesOverDesired, useStDevForJitterCalc)
{
}

int MixedProcessedAudioStream ::parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties, int numAudioSamples) {

    QByteArray outputBuffer;
    emit processSamples(packetAfterStreamProperties, outputBuffer);
    
    _ringBuffer.writeData(outputBuffer.data(), outputBuffer.size());

    return packetAfterStreamProperties.size();
}
