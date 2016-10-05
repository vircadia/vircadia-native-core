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

#include "InboundAudioStream.h"

class AudioClient;

class MixedProcessedAudioStream  : public InboundAudioStream {
    Q_OBJECT
public:
    MixedProcessedAudioStream(int numFrameSamples, int numFramesCapacity, int numStaticJitterFrames = -1);

signals:

    void addedSilence(int silentSamplesPerChannel);
    void addedLastFrameRepeatedWithFade(int samplesPerChannel);
    void addedStereoSamples(const QByteArray& samples);

    void processSamples(const QByteArray& inputBuffer, QByteArray& outputBuffer);

public:
    void outputFormatChanged(int sampleRate, int channelCount);

protected:
    int writeDroppableSilentFrames(int silentFrames) override;
    int writeLastFrameRepeatedWithFade(int frames) override;
    int parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties) override;

private:
    int networkToDeviceFrames(int networkFrames);
    int deviceToNetworkFrames(int deviceFrames);

private:
    quint64 _outputSampleRate;
    quint64 _outputChannelCount;
};

#endif // hifi_MixedProcessedAudioStream_h
