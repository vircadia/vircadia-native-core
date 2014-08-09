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
    : InboundAudioStream(numFrameSamples, numFramesCapacity, settings),
    _networkSamplesWritten(0)
{
}

void MixedProcessedAudioStream::outputFormatChanged(int outputFormatChannelCountTimesSampleRate) {
    _outputFormatChannelsTimesSampleRate = outputFormatChannelCountTimesSampleRate;
    int deviceOutputFrameSize = NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL * _outputFormatChannelsTimesSampleRate / SAMPLE_RATE;
    _ringBuffer.resizeForFrameSize(deviceOutputFrameSize);
}

int MixedProcessedAudioStream::parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties, int networkSamples) {

    memcpy(&_networkSamples[_networkSamplesWritten], packetAfterStreamProperties.data(), packetAfterStreamProperties.size());
    _networkSamplesWritten += packetAfterStreamProperties.size() / sizeof(int16_t);

    QByteArray outputBuffer;
    emit processSamples(packetAfterStreamProperties, outputBuffer);
    
    _ringBuffer.writeData(outputBuffer.data(), outputBuffer.size());

    return packetAfterStreamProperties.size();
}

int MixedProcessedAudioStream::writeDroppableSilentSamples(int silentSamples) {
    int deviceSilentSamplesWritten = InboundAudioStream::writeDroppableSilentSamples(networkToDeviceSamples(silentSamples));

    int networkSilentSamplesWritten = deviceToNetworkSamples(deviceSilentSamplesWritten);
    memset(&_networkSamples[_networkSamplesWritten], 0, networkSilentSamplesWritten * sizeof(int16_t));
    _networkSamplesWritten += networkSilentSamplesWritten;

    return deviceSilentSamplesWritten;
}

static const int STEREO_FACTOR = 2;

int MixedProcessedAudioStream::networkToDeviceSamples(int networkSamples) {
    return networkSamples * _outputFormatChannelsTimesSampleRate / (STEREO_FACTOR * SAMPLE_RATE);
}

int MixedProcessedAudioStream::deviceToNetworkSamples(int deviceSamples) {
    return deviceSamples * (STEREO_FACTOR * SAMPLE_RATE) / _outputFormatChannelsTimesSampleRate;
}
