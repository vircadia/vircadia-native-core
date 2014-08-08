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

MixedProcessedAudioStream::MixedProcessedAudioStream(int numFrameSamples, int numFramesCapacity, const InboundAudioStream::Settings& settings)
    : InboundAudioStream(numFrameSamples, numFramesCapacity, settings)
{
}

void MixedProcessedAudioStream::outputFormatChanged(int outputFormatChannelCountTimesSampleRate) {
    _outputFormatChannelsTimesSampleRate = outputFormatChannelCountTimesSampleRate;
    int deviceOutputFrameSize = NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL * _outputFormatChannelsTimesSampleRate / SAMPLE_RATE;
    _ringBuffer.resizeForFrameSize(deviceOutputFrameSize);
}

int MixedProcessedAudioStream::parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties, int networkSamples) {

    QByteArray outputBuffer;
    emit processSamples(packetAfterStreamProperties, outputBuffer);
    
    _ringBuffer.writeData(outputBuffer.data(), outputBuffer.size());

    return packetAfterStreamProperties.size();
}

int MixedProcessedAudioStream::writeDroppableSilentSamples(int silentSamples) {
    return InboundAudioStream::writeDroppableSilentSamples(networkToDeviceSamples(silentSamples));
}

int MixedProcessedAudioStream::networkToDeviceSamples(int networkSamples) {
    const int STEREO_DIVIDER = 2;
    return networkSamples * _outputFormatChannelsTimesSampleRate / (STEREO_DIVIDER * SAMPLE_RATE);
}
