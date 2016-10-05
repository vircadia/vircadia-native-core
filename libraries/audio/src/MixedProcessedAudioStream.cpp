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
#include "AudioLogging.h"

static const int STEREO_FACTOR = 2;

MixedProcessedAudioStream::MixedProcessedAudioStream(int numFrameSamples, int numFramesCapacity, int numStaticJitterFrames)
    : InboundAudioStream(numFrameSamples, numFramesCapacity, numStaticJitterFrames) {}

void MixedProcessedAudioStream::outputFormatChanged(int sampleRate, int channelCount) {
    _outputSampleRate = sampleRate;
    _outputChannelCount = channelCount;
    int deviceOutputFrameFrames = networkToDeviceFrames(AudioConstants::NETWORK_FRAME_SAMPLES_STEREO / STEREO_FACTOR);
    int deviceOutputFrameSamples = deviceOutputFrameFrames * STEREO_FACTOR;
    _ringBuffer.resizeForFrameSize(deviceOutputFrameSamples);
}

int MixedProcessedAudioStream::writeDroppableSilentSamples(int silentSamples) {
    int deviceSilentFrames = networkToDeviceFrames(silentSamples / STEREO_FACTOR);
    int deviceSilentSamples = deviceSilentFrames * STEREO_FACTOR;
    int deviceSilentSamplesWritten = InboundAudioStream::writeDroppableSilentSamples(deviceSilentSamples);
    emit addedSilence(deviceToNetworkFrames(deviceSilentSamplesWritten / STEREO_FACTOR));
    return deviceSilentSamplesWritten;
}

int MixedProcessedAudioStream::writeLastFrameRepeatedWithFade(int samples) {
    int deviceFrames = networkToDeviceFrames(samples / STEREO_FACTOR);
    int deviceSamples = deviceFrames * STEREO_FACTOR;
    int deviceSamplesWritten = InboundAudioStream::writeLastFrameRepeatedWithFade(deviceSamples);
    emit addedLastFrameRepeatedWithFade(deviceToNetworkFrames(deviceSamplesWritten / STEREO_FACTOR));
    return deviceSamplesWritten;
}

int MixedProcessedAudioStream::parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties) {
    QByteArray decodedBuffer;
    if (_decoder) {
        _decoder->decode(packetAfterStreamProperties, decodedBuffer);
    } else {
        decodedBuffer = packetAfterStreamProperties;
    }

    emit addedStereoSamples(decodedBuffer);

    QByteArray outputBuffer;
    emit processSamples(decodedBuffer, outputBuffer);

    _ringBuffer.writeData(outputBuffer.data(), outputBuffer.size());
    qCDebug(audiostream, "Wrote %d samples to buffer (%d available)", outputBuffer.size() / (int)sizeof(int16_t), getSamplesAvailable());
    
    return packetAfterStreamProperties.size();
}

int MixedProcessedAudioStream::networkToDeviceFrames(int networkFrames) {
    return ((quint64)networkFrames * _outputChannelCount * _outputSampleRate) /
        (quint64)(STEREO_FACTOR * AudioConstants::SAMPLE_RATE);
}

int MixedProcessedAudioStream::deviceToNetworkFrames(int deviceFrames) {
    return (quint64)deviceFrames * (quint64)(STEREO_FACTOR * AudioConstants::SAMPLE_RATE) /
        (_outputSampleRate * _outputChannelCount);
}
