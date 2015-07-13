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
#include <AudioRingBuffer.h>
#include <ThreadedAssignment.h>

class PositionalAudioStream;
class AvatarAudioStream;
class AudioMixerClientData;

const int SAMPLE_PHASE_DELAY_AT_90 = 20;

const int READ_DATAGRAMS_STATS_WINDOW_SECONDS = 30;

/// Handles assignments of type AudioMixer - mixing streams of audio and re-distributing to various clients.
class AudioMixer : public ThreadedAssignment {
    Q_OBJECT
public:
    AudioMixer(NLPacket& packet);

    void deleteLater() { qDebug() << "DELETE LATER CALLED?"; QObject::deleteLater(); }
public slots:
    /// threaded run of assignment
    void run();

    void sendStatsPacket();

    static const InboundAudioStream::Settings& getStreamSettings() { return _streamSettings; }

private slots:
    void handleNodeAudioPacket(QSharedPointer<NLPacket> packet, SharedNodePointer sendingNode);
    void handleMuteEnvironmentPacket(QSharedPointer<NLPacket> packet, SharedNodePointer sendingNode);

private:
    /// adds one stream to the mix for a listening node
    int addStreamToMixForListeningNodeWithStream(AudioMixerClientData* listenerNodeData,
                                                    const QUuid& streamUUID,
                                                    PositionalAudioStream* streamToAdd,
                                                    AvatarAudioStream* listeningNodeStream);

    /// prepares and sends a mix to one Node
    int prepareMixForListeningNode(Node* node);

    /// Send Audio Environment packet for a single node
    void sendAudioEnvironmentPacket(SharedNodePointer node);

    // used on a per stream basis to run the filter on before mixing, large enough to handle the historical
    // data from a phase delay as well as an entire network buffer
    int16_t _preMixSamples[AudioConstants::NETWORK_FRAME_SAMPLES_STEREO + (SAMPLE_PHASE_DELAY_AT_90 * 2)];

    // client samples capacity is larger than what will be sent to optimize mixing
    // we are MMX adding 4 samples at a time so we need client samples to have an extra 4
    int16_t _mixSamples[AudioConstants::NETWORK_FRAME_SAMPLES_STEREO + (SAMPLE_PHASE_DELAY_AT_90 * 2)];

    void perSecondActions();

    bool shouldMute(float quietestFrame);

    void parseSettingsObject(const QJsonObject& settingsObject);

    float _trailingSleepRatio;
    float _minAudibilityThreshold;
    float _performanceThrottlingRatio;
    float _attenuationPerDoublingInDistance;
    float _noiseMutingThreshold;
    int _numStatFrames;
    int _sumListeners;
    int _sumMixes;

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
