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
#include <AudioFormat.h> // For AudioFilterHSF1s and _penumbraFilter
#include <AudioBuffer.h> // For AudioFilterHSF1s and _penumbraFilter
#include <AudioFilter.h> // For AudioFilterHSF1s and _penumbraFilter
#include <AudioFilterBank.h> // For AudioFilterHSF1s and _penumbraFilter

#include "PositionalAudioStream.h"
#include "AvatarAudioStream.h"

class PerListenerSourcePairData {
public:
    PerListenerSourcePairData() {
        _penumbraFilter.initialize(AudioConstants::SAMPLE_RATE, AudioConstants::NETWORK_FRAME_SAMPLES_STEREO / 2);
    };
    AudioFilterHSF1s& getPenumbraFilter() { return _penumbraFilter; }

private:
    AudioFilterHSF1s _penumbraFilter;
};

class AudioMixerClientData : public NodeData {
public:
    AudioMixerClientData();
    ~AudioMixerClientData();
    
    const QHash<QUuid, PositionalAudioStream*>& getAudioStreams() const { return _audioStreams; }
    AvatarAudioStream* getAvatarAudioStream() const;
    
    int parseData(NLPacket& packet);

    void checkBuffersBeforeFrameSend();

    void removeDeadInjectedStreams();

    QJsonObject getAudioStreamStats() const;
    
    void sendAudioStreamStatsPackets(const SharedNodePointer& destinationNode);
    
    void incrementOutgoingMixedAudioSequenceNumber() { _outgoingMixedAudioSequenceNumber++; }
    quint16 getOutgoingSequenceNumber() const { return _outgoingMixedAudioSequenceNumber; }

    void printUpstreamDownstreamStats() const;

    PerListenerSourcePairData* getListenerSourcePairData(const QUuid& sourceUUID);
private:
    void printAudioStreamStats(const AudioStreamStats& streamStats) const;

private:
    QHash<QUuid, PositionalAudioStream*> _audioStreams;     // mic stream stored under key of null UUID

    // TODO: how can we prune this hash when a stream is no longer present?
    QHash<QUuid, PerListenerSourcePairData*> _listenerSourcePairData;

    quint16 _outgoingMixedAudioSequenceNumber;

    AudioStreamStats _downstreamAudioStreamStats;
};

#endif // hifi_AudioMixerClientData_h
