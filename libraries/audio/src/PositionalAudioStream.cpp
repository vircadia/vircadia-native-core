//
//  PositionalAudioStream.cpp
//  libraries/audio/src
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PositionalAudioStream.h"
#include "SharedUtil.h"

#include <cstring>

#include <glm/detail/func_common.hpp>
#include <QtCore/QDataStream>

#include <Node.h>
#include <PacketHeaders.h>
#include <UUID.h>

PositionalAudioStream::PositionalAudioStream(PositionalAudioStream::Type type, bool isStereo, bool dynamicJitterBuffers,
    int staticDesiredJitterBufferFrames, int maxFramesOverDesired) :
    InboundAudioStream(isStereo ? NETWORK_BUFFER_LENGTH_SAMPLES_STEREO : NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL,
    AUDIOMIXER_INBOUND_RING_BUFFER_FRAME_CAPACITY, dynamicJitterBuffers, staticDesiredJitterBufferFrames, maxFramesOverDesired),
    _type(type),
    _position(0.0f, 0.0f, 0.0f),
    _orientation(0.0f, 0.0f, 0.0f, 0.0f),
    _shouldLoopbackForNode(false),
    _isStereo(isStereo),
    _lastPopOutputTrailingLoudness(0.0f),
    _listenerUnattenuatedZone(NULL)
{
    // constant defined in AudioMixer.h.  However, we don't want to include this here, since we will soon find a better common home for these audio-related constants
    const int SAMPLE_PHASE_DELAY_AT_90 = 20; 
    _filter.initialize( SAMPLE_RATE, ( NETWORK_BUFFER_LENGTH_SAMPLES_STEREO + (SAMPLE_PHASE_DELAY_AT_90 * 2) ) / 2);
}

void PositionalAudioStream::updateLastPopOutputTrailingLoudness() {
    float lastPopLoudness = _ringBuffer.getFrameLoudness(_lastPopOutput);

    const int TRAILING_AVERAGE_FRAMES = 100;
    const float CURRENT_FRAME_RATIO = 1.0f / TRAILING_AVERAGE_FRAMES;
    const float PREVIOUS_FRAMES_RATIO = 1.0f - CURRENT_FRAME_RATIO;
    const float LOUDNESS_EPSILON = 0.000001f;

    if (lastPopLoudness >= _lastPopOutputTrailingLoudness) {
        _lastPopOutputTrailingLoudness = lastPopLoudness;
    } else {
        _lastPopOutputTrailingLoudness = (_lastPopOutputTrailingLoudness * PREVIOUS_FRAMES_RATIO) + (CURRENT_FRAME_RATIO * lastPopLoudness);

        if (_lastPopOutputTrailingLoudness < LOUDNESS_EPSILON) {
            _lastPopOutputTrailingLoudness = 0;
        }
    }
}

int PositionalAudioStream::parsePositionalData(const QByteArray& positionalByteArray) {
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

AudioStreamStats PositionalAudioStream::getAudioStreamStats() const {
    AudioStreamStats streamStats = InboundAudioStream::getAudioStreamStats();
    streamStats._streamType = _type;
    return streamStats;
}
