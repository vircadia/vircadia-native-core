//
//  PositionalAudioRingBuffer.cpp
//  libraries/audio/src
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PositionalAudioRingBuffer.h"
#include "SharedUtil.h"

#include <cstring>

#include <glm/detail/func_common.hpp>
#include <QtCore/QDataStream>

#include <Node.h>
#include <PacketHeaders.h>
#include <UUID.h>

PositionalAudioRingBuffer::PositionalAudioRingBuffer(PositionalAudioRingBuffer::Type type, bool isStereo, bool dynamicJitterBuffers) :
InboundAudioStream(isStereo ? NETWORK_BUFFER_LENGTH_SAMPLES_STEREO : NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL,
AUDIOMIXER_INBOUND_RING_BUFFER_FRAME_CAPACITY, dynamicJitterBuffers),
_type(type),
_position(0.0f, 0.0f, 0.0f),
_orientation(0.0f, 0.0f, 0.0f, 0.0f),
_shouldLoopbackForNode(false),
_isStereo(isStereo),
_nextOutputTrailingLoudness(0.0f),
_listenerUnattenuatedZone(NULL)
{
}

void PositionalAudioRingBuffer::updateNextOutputTrailingLoudness() {
    float nextLoudness = _ringBuffer.getNextOutputFrameLoudness();

    const int TRAILING_AVERAGE_FRAMES = 100;
    const float CURRENT_FRAME_RATIO = 1.0f / TRAILING_AVERAGE_FRAMES;
    const float PREVIOUS_FRAMES_RATIO = 1.0f - CURRENT_FRAME_RATIO;
    const float LOUDNESS_EPSILON = 0.000001f;

    if (nextLoudness >= _nextOutputTrailingLoudness) {
        _nextOutputTrailingLoudness = nextLoudness;
    } else {
        _nextOutputTrailingLoudness = (_nextOutputTrailingLoudness * PREVIOUS_FRAMES_RATIO) + (CURRENT_FRAME_RATIO * nextLoudness);

        if (_nextOutputTrailingLoudness < LOUDNESS_EPSILON) {
            _nextOutputTrailingLoudness = 0;
        }
    }
}

int PositionalAudioRingBuffer::parsePositionalData(const QByteArray& positionalByteArray) {
    QDataStream packetStream(positionalByteArray);

    packetStream.readRawData(reinterpret_cast<char*>(&_position), sizeof(_position));
    packetStream.readRawData(reinterpret_cast<char*>(&_orientation), sizeof(_orientation));

    // if this node sent us a NaN for first float in orientation then don't consider this good audio and bail
    if (glm::isnan(_orientation.x)) {
        // NOTE: why would we reset the ring buffer here?
        _ringBuffer.reset();
        return 0;
    }

    return packetStream.device()->pos();
}

AudioStreamStats PositionalAudioRingBuffer::getAudioStreamStats() const {
    AudioStreamStats streamStats = InboundAudioStream::getAudioStreamStats();
    streamStats._streamType = _type;
    return streamStats;
}
