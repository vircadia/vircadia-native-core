//
//  AudioMixerClientData.h
//  assignment-client/src/audio
//
//  Created by Stephen Birarda on 10/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioMixerClientData_h
#define hifi_AudioMixerClientData_h

#include <AABox.h>
#include <NodeData.h>
#include <PositionalAudioRingBuffer.h>

#include "AvatarAudioRingBuffer.h"
#include "AudioStreamStats.h"
#include "SequenceNumberStats.h"


const int INCOMING_SEQ_STATS_HISTORY_LENGTH_SECONDS = 30;

class AudioMixerClientData : public NodeData {
public:
    AudioMixerClientData();
    ~AudioMixerClientData();
    
    const QList<PositionalAudioRingBuffer*> getRingBuffers() const { return _ringBuffers; }
    AvatarAudioRingBuffer* getAvatarAudioRingBuffer() const;
    
    int parseData(const QByteArray& packet);
    void checkBuffersBeforeFrameSend(AABox* checkSourceZone = NULL, AABox* listenerZone = NULL);
    void pushBuffersAfterFrameSend();

    AudioStreamStats getAudioStreamStatsOfStream(const PositionalAudioRingBuffer* ringBuffer) const;
    QString getAudioStreamStatsString() const;
    
    void sendAudioStreamStatsPackets(const SharedNodePointer& destinationNode);
    
    void incrementOutgoingMixedAudioSequenceNumber() { _outgoingMixedAudioSequenceNumber++; }
    quint16 getOutgoingSequenceNumber() const { return _outgoingMixedAudioSequenceNumber; }

private:
    QList<PositionalAudioRingBuffer*> _ringBuffers;

    quint16 _outgoingMixedAudioSequenceNumber;
    SequenceNumberStats _incomingAvatarAudioSequenceNumberStats;
    QHash<QUuid, SequenceNumberStats> _incomingInjectedAudioSequenceNumberStatsMap;

    AudioStreamStats _downstreamAudioStreamStats;
};

#endif // hifi_AudioMixerClientData_h
