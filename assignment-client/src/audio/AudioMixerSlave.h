//
//  AudioMixerSlave.h
//  assignment-client/src/audio
//
//  Created by Zach Pomerantz on 11/22/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioMixerSlave_h
#define hifi_AudioMixerSlave_h

#include <AABox.h>
#include <AudioHRTF.h>
#include <AudioRingBuffer.h>
#include <ThreadedAssignment.h>
#include <UUIDHasher.h>

#include "AudioMixerStats.h"

class PositionalAudioStream;
class AvatarAudioStream;
class AudioHRTF;
class AudioMixerClientData;

class AudioMixerSlave {
public:
    // mix and broadcast non-ignored streams to the node
    // returns true if a mixed packet was sent to the node
    void mix(const SharedNodePointer& node, unsigned int frame);

    AudioMixerStats stats;

private:
    // create mix, returns true if mix has audio
    bool prepareMix(const SharedNodePointer& node);
    // add a stream to the mix
    void addStreamToMix(AudioMixerClientData& listenerData, const QUuid& streamerID,
            const AvatarAudioStream& listenerStream, const PositionalAudioStream& streamer);

    float gainForSource(const AvatarAudioStream& listener, const PositionalAudioStream& streamer,
            const glm::vec3& relativePosition, bool isEcho);
    float azimuthForSource(const AvatarAudioStream& listener, const PositionalAudioStream& streamer,
            const glm::vec3& relativePosition);

    // mixing buffers
    float _mixedSamples[AudioConstants::NETWORK_FRAME_SAMPLES_STEREO];
    int16_t _clampedSamples[AudioConstants::NETWORK_FRAME_SAMPLES_STEREO];
};

#endif // hifi_AudioMixerSlave_h
