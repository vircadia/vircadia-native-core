//
//  AudioIOStats.h
//  interface/src/audio
//
//  Created by Stephen Birarda on 2014-12-16.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioIOStats_h
#define hifi_AudioIOStats_h

#include "MovingMinMaxAvg.h"

#include <QObject>

#include <AudioStreamStats.h>
#include <Node.h>
#include <NLPacket.h>

class MixedProcessedAudioStream;

class AudioIOStats : public QObject {
    Q_OBJECT
public:
    AudioIOStats(MixedProcessedAudioStream* receivedAudioStream);
               
    void reset();
    
    void updateInputMsRead(float ms) { _inputMsRead.update(ms); }
    void updateInputMsUnplayed(float ms) { _inputMsUnplayed.update(ms); }
    void updateOutputMsUnplayed(float ms) { _outputMsUnplayed.update(ms); }
    void sentPacket();
    
    const MovingMinMaxAvg<float>& getInputMsRead() const;
    const MovingMinMaxAvg<float>& getInputMsUnplayed() const;
    const MovingMinMaxAvg<float>& getOutputMsUnplayed() const;
    const MovingMinMaxAvg<quint64>& getPacketTimegaps() const;

    const AudioStreamStats getMixerDownstreamStats() const;
    const AudioStreamStats& getMixerAvatarStreamStats() const {  return _mixerAvatarStreamStats; }
    const QHash<QUuid, AudioStreamStats>& getMixerInjectedStreamStatsMap() const { return _mixerInjectedStreamStatsMap; }
    
    void sendDownstreamAudioStatsPacket();

public slots:
    void processStreamStatsPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);

private:
    MixedProcessedAudioStream* _receivedAudioStream;

    mutable MovingMinMaxAvg<float> _inputMsRead;
    mutable MovingMinMaxAvg<float> _inputMsUnplayed;
    mutable MovingMinMaxAvg<float> _outputMsUnplayed;

    quint64 _lastSentPacketTime;
    mutable MovingMinMaxAvg<quint64> _packetTimegaps;
    
    AudioStreamStats _mixerAvatarStreamStats;
    QHash<QUuid, AudioStreamStats> _mixerInjectedStreamStatsMap;
};

#endif // hifi_AudioIOStats_h
