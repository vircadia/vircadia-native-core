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
#include "SequenceNumbersStats.h"

class AudioMixerClientData : public NodeData {
public:
    AudioMixerClientData();
    ~AudioMixerClientData();
    
    const QList<PositionalAudioRingBuffer*> getRingBuffers() const { return _ringBuffers; }
    AvatarAudioRingBuffer* getAvatarAudioRingBuffer() const;
    
    int parseData(const QByteArray& packet);
    void checkBuffersBeforeFrameSend(AABox* checkSourceZone = NULL, AABox* listenerZone = NULL);
    void pushBuffersAfterFrameSend();

    void getJitterBuffersStats(AudioMixerJitterBuffersStats& stats) const;
    
    int encodeAudioStreamStatsPacket(char* packet) const;
    
    QString getJitterBufferStatsString() const;

    void incrementOutgoingSequenceNumber() { _outgoingSequenceNumber++; }

    quint16 getOutgoingSequenceNumber() const { return _outgoingSequenceNumber; }
    //const SequenceNumberStats& getIncomingSequenceNumberStats() const { return _incomingSequenceNumberStats; }

private:
    QList<PositionalAudioRingBuffer*> _ringBuffers;

    quint16 _outgoingSequenceNumber;
    SequenceNumberStats _incomingAvatarSequenceNumberStats;
    QHash<QUuid, SequenceNumberStats> _incomingInjectedSequenceNumberStatsMap;
};

#endif // hifi_AudioMixerClientData_h
