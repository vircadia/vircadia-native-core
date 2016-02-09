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

#include <QtCore/QJsonObject>

#include <AABox.h>
#include <UUIDHasher.h>

#include "PositionalAudioStream.h"
#include "AvatarAudioStream.h"

class AudioMixerClientData : public NodeData {
public:
    AudioMixerClientData();

    using AudioStreamMap = std::unordered_map<QUuid, std::unique_ptr<PositionalAudioStream>>;
    
    const AudioStreamMap& getAudioStreams() const { return _audioStreams; }
    AvatarAudioStream* getAvatarAudioStream() const;
    
    int parseData(ReceivedMessage& message);

    void checkBuffersBeforeFrameSend();

    void removeDeadInjectedStreams();

    QJsonObject getAudioStreamStats() const;
    
    void sendAudioStreamStatsPackets(const SharedNodePointer& destinationNode);
    
    void incrementOutgoingMixedAudioSequenceNumber() { _outgoingMixedAudioSequenceNumber++; }
    quint16 getOutgoingSequenceNumber() const { return _outgoingMixedAudioSequenceNumber; }

    void printUpstreamDownstreamStats() const;

signals:
    void injectorStreamFinished(const QUuid& streamIdentifier);

private:
    void printAudioStreamStats(const AudioStreamStats& streamStats) const;

private:
    AudioStreamMap _audioStreams;     // mic stream stored under key of null UUID

    quint16 _outgoingMixedAudioSequenceNumber;

    AudioStreamStats _downstreamAudioStreamStats;
};

#endif // hifi_AudioMixerClientData_h
