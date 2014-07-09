//
//  AudioStreamStats.h
//  libraries/audio/src
//
//  Created by Yixin Wang on 6/25/2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioStreamStats_h
#define hifi_AudioStreamStats_h

#include "PositionalAudioRingBuffer.h"

class AudioStreamStats {
public:
    AudioStreamStats()
        : _streamType(PositionalAudioRingBuffer::Microphone),
        _streamIdentifier(),
        _timeGapMin(0),
        _timeGapMax(0),
        _timeGapAverage(0.0f),
        _timeGapMovingMin(0),
        _timeGapMovingMax(0),
        _timeGapMovingAverage(0.0f),
        _ringBufferFramesAvailable(0),
        _ringBufferCurrentJitterBufferFrames(0),
        _ringBufferDesiredJitterBufferFrames(0),
        _ringBufferStarveCount(0),
        _ringBufferConsecutiveNotMixedCount(0),
        _ringBufferOverflowCount(0),
        _ringBufferSilentFramesDropped(0),
        _packetsReceived(0),
        _packetsUnreasonable(0),
        _packetsEarly(0),
        _packetsLate(0),
        _packetsLost(0),
        _packetsRecovered(0),
        _packetsDuplicate(0)
    {}

    PositionalAudioRingBuffer::Type _streamType;
    QUuid _streamIdentifier;

    quint64 _timeGapMin;
    quint64 _timeGapMax;
    float _timeGapAverage;
    quint64 _timeGapMovingMin;
    quint64 _timeGapMovingMax;
    float _timeGapMovingAverage;

    quint32 _ringBufferFramesAvailable;
    quint16 _ringBufferCurrentJitterBufferFrames;
    quint16 _ringBufferDesiredJitterBufferFrames;
    quint32 _ringBufferStarveCount;
    quint32 _ringBufferConsecutiveNotMixedCount;
    quint32 _ringBufferOverflowCount;
    quint32 _ringBufferSilentFramesDropped;

    quint32 _packetsReceived;
    quint32 _packetsUnreasonable;
    quint32 _packetsEarly;
    quint32 _packetsLate;
    quint32 _packetsLost;
    quint32 _packetsRecovered;
    quint32 _packetsDuplicate;
};

#endif  // hifi_AudioStreamStats_h
