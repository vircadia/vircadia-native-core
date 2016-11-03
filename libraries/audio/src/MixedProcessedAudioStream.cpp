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

MixedProcessedAudioStream::MixedProcessedAudioStream(int numFramesCapacity, int numStaticJitterFrames)
    : InboundAudioStream(AudioConstants::STEREO, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL,
        numFramesCapacity, numStaticJitterFrames) {}

void MixedProcessedAudioStream::outputFormatChanged(int sampleRate, int channelCount) {
    _outputSampleRate = sampleRate;
    _outputChannelCount = channelCount;
    int deviceOutputFrameFrames = networkToDeviceFrames(AudioConstants::NETWORK_FRAME_SAMPLES_STEREO / AudioConstants::STEREO);
    int deviceOutputFrameSamples = deviceOutputFrameFrames * AudioConstants::STEREO;
    _ringBuffer.resizeForFrameSize(deviceOutputFrameSamples);
}

int MixedProcessedAudioStream::writeDroppableSilentFrames(int silentFrames) {
    int deviceSilentFrames = networkToDeviceFrames(silentFrames);
    int deviceSilentFramesWritten = InboundAudioStream::writeDroppableSilentFrames(deviceSilentFrames);
    emit addedSilence(deviceToNetworkFrames(deviceSilentFramesWritten));
    return deviceSilentFramesWritten;
}

int MixedProcessedAudioStream::writeLastFrameRepeatedWithFade(int frames) {
    int deviceFrames = networkToDeviceFrames(frames);
    int deviceFramesWritten = InboundAudioStream::writeLastFrameRepeatedWithFade(deviceFrames);
    emit addedLastFrameRepeatedWithFade(deviceToNetworkFrames(deviceFramesWritten));
    return deviceFramesWritten;
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
        (quint64)(AudioConstants::STEREO * AudioConstants::SAMPLE_RATE);
}

int MixedProcessedAudioStream::deviceToNetworkFrames(int deviceFrames) {
    return (quint64)deviceFrames * (quint64)(AudioConstants::STEREO * AudioConstants::SAMPLE_RATE) /
        (_outputSampleRate * _outputChannelCount);
}
