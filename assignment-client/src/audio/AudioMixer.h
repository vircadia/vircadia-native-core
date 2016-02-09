//
//  AudioMixer.h
//  assignment-client/src/audio
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioMixer_h
#define hifi_AudioMixer_h

#include <AABox.h>
#include <AudioHRTF.h>
#include <AudioRingBuffer.h>
#include <PairHash.h>
#include <ThreadedAssignment.h>
#include <UUIDHasher.h>

class PositionalAudioStream;
class AvatarAudioStream;
class AudioHRTF;
class AudioMixerClientData;

const int SAMPLE_PHASE_DELAY_AT_90 = 20;

const int READ_DATAGRAMS_STATS_WINDOW_SECONDS = 30;

/// Handles assignments of type AudioMixer - mixing streams of audio and re-distributing to various clients.
class AudioMixer : public ThreadedAssignment {
    Q_OBJECT
public:
    AudioMixer(ReceivedMessage& message);

public slots:
    /// threaded run of assignment
    void run();

    void sendStatsPacket();

    static const InboundAudioStream::Settings& getStreamSettings() { return _streamSettings; }

private slots:
    void broadcastMixes();
    void handleNodeAudioPacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer sendingNode);
    void handleMuteEnvironmentPacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer sendingNode);
    void handleNodeKilled(SharedNodePointer killedNode);

    void clearHRTFsMatchingStreamID(const QUuid& streamID);

private:
    void domainSettingsRequestComplete();
    
    using ListenerSourceIDPair = std::pair<QUuid, QUuid>;
    
    /// adds one stream to the mix for a listening node
    void addStreamToMixForListeningNodeWithStream(ListenerSourceIDPair idPair,
                                                  const AudioMixerClientData& listenerNodeData,
                                                  const PositionalAudioStream& streamToAdd,
                                                  const AvatarAudioStream& listeningNodeStream);

    float gainForSource(const PositionalAudioStream& streamToAdd, const AvatarAudioStream& listeningNodeStream,
                        const glm::vec3& relativePosition, bool isEcho);
    float azimuthForSource(const PositionalAudioStream& streamToAdd, const AvatarAudioStream& listeningNodeStream,
                           const glm::vec3& relativePosition);

    /// prepares and sends a mix to one Node
    bool prepareMixForListeningNode(Node* node);

    /// Send Audio Environment packet for a single node
    void sendAudioEnvironmentPacket(SharedNodePointer node);

    void perSecondActions();

    bool shouldMute(float quietestFrame);

    void parseSettingsObject(const QJsonObject& settingsObject);

    float _trailingSleepRatio;
    float _minAudibilityThreshold;
    float _performanceThrottlingRatio;
    float _attenuationPerDoublingInDistance;
    float _noiseMutingThreshold;
    int _numStatFrames { 0 };
    int _sumListeners { 0 };
    int _mixesLastBlock { 0 };
    int _silentMixesLastBlock { 0 };

    float _mixedSamples[AudioConstants::NETWORK_FRAME_SAMPLES_STEREO];
    int16_t _clampedSamples[AudioConstants::NETWORK_FRAME_SAMPLES_STEREO];

    using HRTFMap = std::unordered_map<ListenerSourceIDPair, AudioHRTF, pair_hash>;
    HRTFMap _hrtfMap;

    QHash<QString, AABox> _audioZones;
    struct ZonesSettings {
        QString source;
        QString listener;
        float coefficient;
    };
    QVector<ZonesSettings> _zonesSettings;
    struct ReverbSettings {
        QString zone;
        float reverbTime;
        float wetLevel;
    };
    QVector<ReverbSettings> _zoneReverbSettings;

    static InboundAudioStream::Settings _streamSettings;

    static bool _printStreamStats;
    static bool _enableFilter;

    quint64 _lastPerSecondCallbackTime;

    bool _sendAudioStreamStats;

    // stats
    MovingMinMaxAvg<int> _datagramsReadPerCallStats;     // update with # of datagrams read for each readPendingDatagrams call
    MovingMinMaxAvg<quint64> _timeSpentPerCallStats;     // update with usecs spent inside each readPendingDatagrams call
    MovingMinMaxAvg<quint64> _timeSpentPerHashMatchCallStats; // update with usecs spent inside each packetVersionAndHashMatch call

    MovingMinMaxAvg<int> _readPendingCallsPerSecondStats;     // update with # of readPendingDatagrams calls in the last second
};

#endif // hifi_AudioMixer_h
