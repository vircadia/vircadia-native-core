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
    static const QHash<QString, AABox>& getAudioZones() { return _audioZones; }
    static const QVector<ZoneSettings>& getZoneSettings() { return _zoneSettings; }
    static const QVector<ReverbSettings>& getReverbSettings() { return _zoneReverbSettings; }
    static const std::pair<QString, CodecPluginPointer> negotiateCodec(std::vector<QString> codecs);

    static bool shouldReplicateTo(const Node& from, const Node& to) {
        return to.getType() == NodeType::DownstreamAudioMixer &&
               to.getPublicSocket() != from.getPublicSocket() &&
               to.getLocalSocket() != from.getLocalSocket();
    }
public slots:
    void run() override;
    void sendStatsPacket() override;

private slots:
    // packet handlers
    void handleMuteEnvironmentPacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer sendingNode);
    void handleNodeMuteRequestPacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer sendingNode);
    void handleNodeKilled(SharedNodePointer killedNode);
    void handleKillAvatarPacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer sendingNode);

    void queueAudioPacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer sendingNode);
    void queueReplicatedAudioPacket(QSharedPointer<ReceivedMessage> packet);
    void removeHRTFsForFinishedInjector(const QUuid& streamID);
    void start();

private:
    // mixing helpers
    std::chrono::microseconds timeFrame(p_high_resolution_clock::time_point& timestamp);
    void throttle(std::chrono::microseconds frameDuration, int frame);
    // pop a frame from any streams on the node
    // returns the number of available streams
    int prepareFrame(const SharedNodePointer& node, unsigned int frame);

    AudioMixerClientData* getOrCreateClientData(Node* node);

    QString percentageForMixStats(int counter);

    void parseSettingsObject(const QJsonObject& settingsObject);
    void clearDomainSettings();

    float _trailingMixRatio { 0.0f };
    float _throttlingRatio { 0.0f };

    int _numSilentPackets { 0 };

    int _numStatFrames { 0 };
    AudioMixerStats _stats;

    AudioMixerSlavePool _slavePool;

    class Timer {
    public:
        class Timing{
        public:
            Timing(uint64_t& sum);
            ~Timing();
        private:
            p_high_resolution_clock::time_point _timing;
            uint64_t& _sum;
        };

        Timing timer() { return Timing(_sum); }
        void get(uint64_t& timing, uint64_t& trailing);
    private:
        static const int TIMER_TRAILING_SECONDS = 10;

        uint64_t _sum { 0 };
        uint64_t _trailing { 0 };
        uint64_t _history[TIMER_TRAILING_SECONDS] {};
        int _index { 0 };
    };
    Timer _ticTiming;
    Timer _sleepTiming;
    Timer _frameTiming;
    Timer _prepareTiming;
    Timer _mixTiming;
    Timer _eventsTiming;
    Timer _packetsTiming;

    static int _numStaticJitterFrames; // -1 denotes dynamic jitter buffering
    static float _noiseMutingThreshold;
    static float _attenuationPerDoublingInDistance;
    static std::map<QString, CodecPluginPointer> _availableCodecs;
    static QStringList _codecPreferenceOrder;
    static QHash<QString, AABox> _audioZones;
    static QVector<ZoneSettings> _zoneSettings;
    static QVector<ReverbSettings> _zoneReverbSettings;

};

#endif // hifi_AudioMixer_h
