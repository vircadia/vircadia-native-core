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
    
    void updateInputMsecsRead(float msecsRead) { _audioInputMsecsReadStats.update(msecsRead); }
    void sentPacket();
    
    AudioStreamStats getMixerDownstreamStats() const;
    const AudioStreamStats& getMixerAvatarStreamStats() const {  return _mixerAvatarStreamStats; }
    const QHash<QUuid, AudioStreamStats>& getMixerInjectedStreamStatsMap() const { return _mixerInjectedStreamStatsMap; }
    
    const MovingMinMaxAvg<float>& getAudioInputMsecsReadStats() const { return _audioInputMsecsReadStats; }
    const MovingMinMaxAvg<float>& getInputRungBufferMsecsAvailableStats() const { return _inputRingBufferMsecsAvailableStats; }
    const MovingMinMaxAvg<float>& getAudioOutputMsecsUnplayedStats() const { return _audioOutputMsecsUnplayedStats; }
    
    const MovingMinMaxAvg<quint64>& getPacketSentTimeGaps() const { return _packetSentTimeGaps; }
    
    void sendDownstreamAudioStatsPacket();

public slots:
    void processStreamStatsPacket(QSharedPointer<NLPacket> packet, SharedNodePointer sendingNode);

private:
    MixedProcessedAudioStream* _receivedAudioStream;
    
    MovingMinMaxAvg<float> _audioInputMsecsReadStats;
    MovingMinMaxAvg<float> _inputRingBufferMsecsAvailableStats;
    
    MovingMinMaxAvg<float> _audioOutputMsecsUnplayedStats;
    
    AudioStreamStats _mixerAvatarStreamStats;
    QHash<QUuid, AudioStreamStats> _mixerInjectedStreamStatsMap;
    
    quint64 _lastSentAudioPacket;
    MovingMinMaxAvg<quint64> _packetSentTimeGaps;
};

#endif // hifi_AudioIOStats_h
