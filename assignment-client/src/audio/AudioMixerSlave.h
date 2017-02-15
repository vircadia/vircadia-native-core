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
#include <NodeList.h>

#include "AudioMixerStats.h"

class PositionalAudioStream;
class AvatarAudioStream;
class AudioHRTF;
class AudioMixerClientData;

class AudioMixerSlave {
public:
    using ConstIter = NodeList::const_iterator;

    // process packets for a given node (requires no configuration)
    void processPackets(const SharedNodePointer& node);

    // configure a round of mixing
    void configureMix(ConstIter begin, ConstIter end, unsigned int frame, float throttlingRatio);

    // mix and broadcast non-ignored streams to the node (requires configuration using configureMix, above)
    // returns true if a mixed packet was sent to the node
    void mix(const SharedNodePointer& node);

    AudioMixerStats stats;

private:
    // create mix, returns true if mix has audio
    bool prepareMix(const SharedNodePointer& listener);
    void throttleStream(AudioMixerClientData& listenerData, const QUuid& streamerID,
            const AvatarAudioStream& listenerStream, const PositionalAudioStream& streamer);
    void mixStream(AudioMixerClientData& listenerData, const QUuid& streamerID,
            const AvatarAudioStream& listenerStream, const PositionalAudioStream& streamer);
    void addStream(AudioMixerClientData& listenerData, const QUuid& streamerID,
            const AvatarAudioStream& listenerStream, const PositionalAudioStream& streamer,
            bool throttle);

    // mixing buffers
    float _mixSamples[AudioConstants::NETWORK_FRAME_SAMPLES_STEREO];
    int16_t _bufferSamples[AudioConstants::NETWORK_FRAME_SAMPLES_STEREO];

    // frame state
    ConstIter _begin;
    ConstIter _end;
    unsigned int _frame { 0 };
    float _throttlingRatio { 0.0f };
};

#endif // hifi_AudioMixerSlave_h
