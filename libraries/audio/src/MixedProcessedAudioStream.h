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

#include <QtMultimedia/QAudioFormat>
#include "InboundAudioStream.h"

class MixedProcessedAudioStream  : public InboundAudioStream {
    Q_OBJECT
public:
    MixedProcessedAudioStream (int numFrameSamples, int numFramesCapacity, bool dynamicJitterBuffers, int staticDesiredJitterBufferFrames, int maxFramesOverDesired, bool useStDevForJitterCalc);

    void outputFormatChanged(const QAudioFormat& outputFormat);

signals:
    
    void processSamples(const QByteArray& inputBuffer, QByteArray& outputBuffer);

protected:
    int parseStreamProperties(PacketType type, const QByteArray& packetAfterSeqNum, int& numAudioSamples);
    int parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties, int numAudioSamples);

private:
    int _outputFormatChannelsTimesSampleRate;
};

#endif // hifi_MixedProcessedAudioStream_h
