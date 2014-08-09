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

class MixedProcessedAudioStream  : public InboundAudioStream {
    Q_OBJECT
public:
    MixedProcessedAudioStream(int numFrameSamples, int numFramesCapacity, const InboundAudioStream::Settings& settings);

signals:
    
    void processSamples(const QByteArray& inputBuffer, QByteArray& outputBuffer);

public:
    void outputFormatChanged(int outputFormatChannelCountTimesSampleRate);

protected:
    int parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties, int networkSamples);
    int writeDroppableSilentSamples(int silentSamples);

private:
    int networkToDeviceSamples(int networkSamples);
    int deviceToNetworkSamples(int deviceSamples);
private:
    int _outputFormatChannelsTimesSampleRate;

    int16_t _networkSamples[10 * NETWORK_BUFFER_LENGTH_SAMPLES_STEREO];
    int _networkSamplesWritten;
};

#endif // hifi_MixedProcessedAudioStream_h
