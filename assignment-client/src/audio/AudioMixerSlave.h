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

#if !defined(Q_MOC_RUN)
// Work around https://bugreports.qt.io/browse/QTBUG-80990
#include <tbb/concurrent_vector.h>
#endif

#include <AABox.h>
#include <AudioHRTF.h>
#include <AudioRingBuffer.h>
#include <ThreadedAssignment.h>
#include <UUIDHasher.h>
#include <NodeList.h>
#include <PositionalAudioStream.h>

#include "AudioMixerClientData.h"
#include "AudioMixerStats.h"

class AvatarAudioStream;
class AudioHRTF;

class AudioMixerSlave {
public:
    using ConstIter = NodeList::const_iterator;
    
    struct SharedData {
        AudioMixerClientData::ConcurrentAddedStreams addedStreams;
        std::vector<Node::LocalID> removedNodes;
        std::vector<NodeIDStreamID> removedStreams;
    };

    AudioMixerSlave(SharedData& sharedData) : _sharedData(sharedData) {};

    // process packets for a given node (requires no configuration)
    void processPackets(const SharedNodePointer& node);

    // configure a round of mixing
    void configureMix(ConstIter begin, ConstIter end, unsigned int frame, int numToRetain);

    // mix and broadcast non-ignored streams to the node (requires configuration using configureMix, above)
    // returns true if a mixed packet was sent to the node
    void mix(const SharedNodePointer& node);

    AudioMixerStats stats;

private:
    // create mix, returns true if mix has audio
    bool prepareMix(const SharedNodePointer& listener);
    void addStream(AudioMixerClientData::MixableStream& mixableStream,
                   AvatarAudioStream& listeningNodeStream,
                   float masterAvatarGain,
                   float masterInjectorGain,
                   bool isSoloing);
    void updateHRTFParameters(AudioMixerClientData::MixableStream& mixableStream,
                              AvatarAudioStream& listeningNodeStream,
                              float masterAvatarGain,
                              float masterInjectorGain);
    void resetHRTFState(AudioMixerClientData::MixableStream& mixableStream);

    void addStreams(Node& listener, AudioMixerClientData& listenerData);

    // mixing buffers
    float _mixSamples[AudioConstants::NETWORK_FRAME_SAMPLES_STEREO];
    int16_t _bufferSamples[AudioConstants::NETWORK_FRAME_SAMPLES_STEREO];

    // frame state
    ConstIter _begin;
    ConstIter _end;
    unsigned int _frame { 0 };
    int _numToRetain { -1 };

    SharedData& _sharedData;
};

#endif // hifi_AudioMixerSlave_h
