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

static const int STEREO_FACTOR = 2;

MixedProcessedAudioStream::MixedProcessedAudioStream(int numFrameSamples, int numFramesCapacity, const InboundAudioStream::Settings& settings)
    : InboundAudioStream(numFrameSamples, numFramesCapacity, settings)
{
}

void MixedProcessedAudioStream::outputFormatChanged(int outputFormatChannelCountTimesSampleRate) {
    _outputFormatChannelsTimesSampleRate = outputFormatChannelCountTimesSampleRate;
    int deviceOutputFrameSize = networkToDeviceSamples(AudioConstants::NETWORK_FRAME_SAMPLES_STEREO);
    _ringBuffer.resizeForFrameSize(deviceOutputFrameSize);
}

int MixedProcessedAudioStream::writeDroppableSilentSamples(int silentSamples) {
    
    int deviceSilentSamplesWritten = InboundAudioStream::writeDroppableSilentSamples(networkToDeviceSamples(silentSamples));
    
    emit addedSilence(deviceToNetworkSamples(deviceSilentSamplesWritten) / STEREO_FACTOR);

    return deviceSilentSamplesWritten;
}

int MixedProcessedAudioStream::writeLastFrameRepeatedWithFade(int samples) {

    int deviceSamplesWritten = InboundAudioStream::writeLastFrameRepeatedWithFade(networkToDeviceSamples(samples));

    emit addedLastFrameRepeatedWithFade(deviceToNetworkSamples(deviceSamplesWritten) / STEREO_FACTOR);
    
    return deviceSamplesWritten;
}

int MixedProcessedAudioStream::parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties, int networkSamples) {

    emit addedStereoSamples(packetAfterStreamProperties);

    QByteArray outputBuffer;
    emit processSamples(packetAfterStreamProperties, outputBuffer);

    _ringBuffer.writeData(outputBuffer.data(), outputBuffer.size());
    
    return packetAfterStreamProperties.size();
}

int MixedProcessedAudioStream::networkToDeviceSamples(int networkSamples) {
    return (quint64)networkSamples * (quint64)_outputFormatChannelsTimesSampleRate / (quint64)(STEREO_FACTOR
                                                                                               * AudioConstants::SAMPLE_RATE);
}

int MixedProcessedAudioStream::deviceToNetworkSamples(int deviceSamples) {
    return (quint64)deviceSamples * (quint64)(STEREO_FACTOR * AudioConstants::SAMPLE_RATE)
        / (quint64)_outputFormatChannelsTimesSampleRate;
}
