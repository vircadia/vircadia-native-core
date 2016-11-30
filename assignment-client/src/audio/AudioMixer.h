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
#include <ThreadedAssignment.h>
#include <UUIDHasher.h>

#include "AudioMixerStats.h"
#include "AudioMixerSlavePool.h"

class PositionalAudioStream;
class AvatarAudioStream;
class AudioHRTF;
class AudioMixerClientData;

/// Handles assignments of type AudioMixer - mixing streams of audio and re-distributing to various clients.
class AudioMixer : public ThreadedAssignment {
    Q_OBJECT
public:
    AudioMixer(ReceivedMessage& message);

    struct ZoneSettings {
        QString source;
        QString listener;
        float coefficient;
    };
    struct ReverbSettings {
        QString zone;
        float reverbTime;
        float wetLevel;
    };

    static int getStaticJitterFrames() { return _numStaticJitterFrames; }
    static bool shouldMute(float quietestFrame) { return quietestFrame > _noiseMutingThreshold; }
    static float getAttenuationPerDoublingInDistance() { return _attenuationPerDoublingInDistance; }
    static float getMinimumAudibilityThreshold() { return _performanceThrottlingRatio > 0.0f ? _minAudibilityThreshold : 0.0f; }
    static const QHash<QString, AABox>& getAudioZones() { return _audioZones; }
    static const QVector<ZoneSettings>& getZoneSettings() { return _zoneSettings; }
    static const QVector<ReverbSettings>& getReverbSettings() { return _zoneReverbSettings; }

public slots:
    void run() override;
    void sendStatsPacket() override;

private slots:
    // packet handlers
    void handleNodeAudioPacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer sendingNode);
    void handleMuteEnvironmentPacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer sendingNode);
    void handleNegotiateAudioFormat(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);
    void handleNodeKilled(SharedNodePointer killedNode);
    void handleNodeIgnoreRequestPacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer sendingNode);
    void handleRadiusIgnoreRequestPacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer sendingNode);
    void handleKillAvatarPacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer sendingNode);
    void handleNodeMuteRequestPacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer sendingNode);

    void start();
    void removeHRTFsForFinishedInjector(const QUuid& streamID);

private:
    // mixing helpers
    // check and maybe throttle mixer load by changing audibility threshold
    void manageLoad(p_high_resolution_clock::time_point& frameTimestamp, unsigned int& framesSinceManagement);
    // pop a frame from any streams on the node
    // returns the number of available streams
    int prepareFrame(const SharedNodePointer& node, unsigned int frame);

    AudioMixerClientData* getOrCreateClientData(Node* node);

    QString percentageForMixStats(int counter);

    void parseSettingsObject(const QJsonObject& settingsObject);

    int _numStatFrames { 0 };
    AudioMixerStats _stats;

    QString _codecPreferenceOrder;

    AudioMixerSlavePool _slavePool;

    static const int MIX_TIMES_TRAILING_SECONDS = 10;
    std::chrono::microseconds _sumMixTimes { 0 };
    uint64_t _sumMixTimesTrailing { 0 };
    uint64_t _sumMixTimesHistory[MIX_TIMES_TRAILING_SECONDS] { 0 };
    int _sumMixTimesIndex { 0 };

    static int _numStaticJitterFrames; // -1 denotes dynamic jitter buffering
    static float _noiseMutingThreshold;
    static float _attenuationPerDoublingInDistance;
    static float _trailingSleepRatio;
    static float _performanceThrottlingRatio;
    static float _minAudibilityThreshold;
    static QHash<QString, AABox> _audioZones;
    static QVector<ZoneSettings> _zoneSettings;
    static QVector<ReverbSettings> _zoneReverbSettings;
};

#endif // hifi_AudioMixer_h
