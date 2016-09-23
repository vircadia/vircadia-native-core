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

#define AUDIO_PROPERTY(TYPE, NAME) \
    Q_PROPERTY(TYPE NAME READ NAME NOTIFY NAME##Changed) \
    public: \
        TYPE NAME() const { return _##NAME; } \
        void NAME(TYPE value) { \
            if (_##NAME != value) { \
                _##NAME = value; \
                emit NAME##Changed(value); \
            } \
        } \
    Q_SIGNAL void NAME##Changed(TYPE value); \
    private: \
        TYPE _##NAME{ (TYPE)0 };

class AudioStreamStatsInterface : public QObject {
    Q_OBJECT
    AUDIO_PROPERTY(float, lossRate)
    AUDIO_PROPERTY(float, lossCount)
    AUDIO_PROPERTY(float, lossRateWindow)
    AUDIO_PROPERTY(float, lossCountWindow)

    AUDIO_PROPERTY(int, framesDesired)
    AUDIO_PROPERTY(int, framesAvailable)
    AUDIO_PROPERTY(int, framesAvailableAvg)
    AUDIO_PROPERTY(float, unplayedMsMax)

    AUDIO_PROPERTY(int, starveCount)
    AUDIO_PROPERTY(int, lastStarveDurationCount)
    AUDIO_PROPERTY(int, dropCount)
    AUDIO_PROPERTY(int, overflowCount)

    AUDIO_PROPERTY(quint64, timegapMsMax)
    AUDIO_PROPERTY(quint64, timegapMsAvg)
    AUDIO_PROPERTY(quint64, timegapMsMaxWindow)
    AUDIO_PROPERTY(quint64, timegapMsAvgWindow)

public:
    void updateStream(const AudioStreamStats& stats);

private:
    friend class AudioStatsInterface;
    AudioStreamStatsInterface(QObject* parent);
};

class AudioStatsInterface : public QObject {
    Q_OBJECT
    AUDIO_PROPERTY(float, pingMs);

    AUDIO_PROPERTY(float, inputReadMsMax);
    AUDIO_PROPERTY(float, inputUnplayedMsMax);
    AUDIO_PROPERTY(float, outputUnplayedMsMax);

    AUDIO_PROPERTY(quint64, sentTimegapMsMax);
    AUDIO_PROPERTY(quint64, sentTimegapMsAvg);
    AUDIO_PROPERTY(quint64, sentTimegapMsMaxWindow);
    AUDIO_PROPERTY(quint64, sentTimegapMsAvgWindow);

    Q_PROPERTY(AudioStreamStatsInterface* mixerStream READ getMixerStream);
    Q_PROPERTY(AudioStreamStatsInterface* clientStream READ getClientStream);
    Q_PROPERTY(QObject* injectorStreams READ getInjectorStreams NOTIFY injectorStreamsChanged);

public:
    AudioStreamStatsInterface* getMixerStream() const { return _mixer; }
    AudioStreamStatsInterface* getClientStream() const { return _client; }
    QObject* getInjectorStreams() const { return _injectors; }

    void updateLocalBuffers(const MovingMinMaxAvg<float>& inputMsRead,
                            const MovingMinMaxAvg<float>& inputMsUnplayed,
                            const MovingMinMaxAvg<float>& outputMsUnplayed,
                            const MovingMinMaxAvg<quint64>& timegaps);
    void updateClientStream(const AudioStreamStats& stats) { _client->updateStream(stats); }
    void updateMixerStream(const AudioStreamStats& stats) { _mixer->updateStream(stats); }
    void updateInjectorStreams(const QHash<QUuid, AudioStreamStats>& stats);

signals:
    void injectorStreamsChanged();

private:
    friend class AudioIOStats;
    AudioStatsInterface(QObject* parent);
    AudioStreamStatsInterface* _client;
    AudioStreamStatsInterface* _mixer;
    QObject* _injectors;
};

class AudioIOStats : public QObject {
    Q_OBJECT
public:
    AudioIOStats(MixedProcessedAudioStream* receivedAudioStream);

    void reset();

    AudioStatsInterface* data() const { return _interface; }

    void updateInputMsRead(float ms) const { _inputMsRead.update(ms); }
    void updateInputMsUnplayed(float ms) const { _inputMsUnplayed.update(ms); }
    void updateOutputMsUnplayed(float ms) const { _outputMsUnplayed.update(ms); }
    void sentPacket() const;

    void publish();

public slots:
    void processStreamStatsPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);

private:
    AudioStatsInterface* _interface;

    mutable MovingMinMaxAvg<float> _inputMsRead;
    mutable MovingMinMaxAvg<float> _inputMsUnplayed;
    mutable MovingMinMaxAvg<float> _outputMsUnplayed;

    mutable quint64 _lastSentPacketTime;
    mutable MovingMinMaxAvg<quint64> _packetTimegaps;

    MixedProcessedAudioStream* _receivedAudioStream;
    QHash<QUuid, AudioStreamStats> _injectorStreams;
};

#endif // hifi_AudioIOStats_h
