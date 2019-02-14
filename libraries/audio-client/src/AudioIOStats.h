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

    /**jsdoc
     * @class AudioStats.AudioStreamStats
     *
     * @hifi-interface
     * @hifi-client-entity
     * @hifi-avatar
     *
     * @property {number} lossRate <em>Read-only.</em>
     * @property {number} lossCount <em>Read-only.</em>
     * @property {number} lossRateWindow <em>Read-only.</em>
     * @property {number} lossCountWindow <em>Read-only.</em>
     * @property {number} framesDesired <em>Read-only.</em>
     * @property {number} framesAvailable <em>Read-only.</em>
     * @property {number} framesAvailableAvg <em>Read-only.</em>
     * @property {number} unplayedMsMax <em>Read-only.</em>
     * @property {number} starveCount <em>Read-only.</em>
     * @property {number} lastStarveDurationCount <em>Read-only.</em>
     * @property {number} dropCount <em>Read-only.</em>
     * @property {number} overflowCount <em>Read-only.</em>
     * @property {number} timegapMsMax <em>Read-only.</em>
     * @property {number} timegapMsAvg <em>Read-only.</em>
     * @property {number} timegapMsMaxWindow <em>Read-only.</em>
     * @property {number} timegapMsAvgWindow <em>Read-only.</em>
     */

    /**jsdoc
     * @function AudioStats.AudioStreamStats.lossRateChanged
     * @param {number} lossRate
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(float, lossRate)

    /**jsdoc
     * @function AudioStats.AudioStreamStats.lossCountChanged
     * @param {number} lossCount
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(float, lossCount)

    /**jsdoc
     * @function AudioStats.AudioStreamStats.lossRateWindowChanged
     * @param {number} lossRateWindow
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(float, lossRateWindow)

    /**jsdoc
     * @function AudioStats.AudioStreamStats.lossCountWindowChanged
     * @param {number} lossCountWindow
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(float, lossCountWindow)

    /**jsdoc
     * @function AudioStats.AudioStreamStats.framesDesiredChanged
     * @param {number} framesDesired
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(int, framesDesired)

    /**jsdoc
     * @function AudioStats.AudioStreamStats.framesAvailableChanged
     * @param {number} framesAvailable
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(int, framesAvailable)

    /**jsdoc
     * @function AudioStats.AudioStreamStats.framesAvailableAvgChanged
     * @param {number} framesAvailableAvg
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(int, framesAvailableAvg)

    /**jsdoc
     * @function AudioStats.AudioStreamStats.unplayedMsMaxChanged
     * @param {number} unplayedMsMax
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(float, unplayedMsMax)

    /**jsdoc
     * @function AudioStats.AudioStreamStats.starveCountChanged
     * @param {number} starveCount
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(int, starveCount)

    /**jsdoc
     * @function AudioStats.AudioStreamStats.lastStarveDurationCountChanged
     * @param {number} lastStarveDurationCount
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(int, lastStarveDurationCount)

    /**jsdoc
     * @function AudioStats.AudioStreamStats.dropCountChanged
     * @param {number} dropCount
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(int, dropCount)

    /**jsdoc
     * @function AudioStats.AudioStreamStats.overflowCountChanged
     * @param {number} overflowCount
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(int, overflowCount)

    /**jsdoc
     * @function AudioStats.AudioStreamStats.timegapMsMaxChanged
     * @param {number} timegapMsMax
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(quint64, timegapMsMax)

    /**jsdoc
     * @function AudioStats.AudioStreamStats.timegapMsAvgChanged
     * @param {number} timegapMsAvg
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(quint64, timegapMsAvg)

    /**jsdoc
     * @function AudioStats.AudioStreamStats.timegapMsMaxWindowChanged
     * @param {number} timegapMsMaxWindow
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(quint64, timegapMsMaxWindow)

    /**jsdoc
     * @function AudioStats.AudioStreamStats.timegapMsAvgWindowChanged
     * @param {number} timegapMsAvgWindow
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(quint64, timegapMsAvgWindow)

public:
    void updateStream(const AudioStreamStats& stats);

private:
    friend class AudioStatsInterface;
    AudioStreamStatsInterface(QObject* parent);
};

class AudioStatsInterface : public QObject {
    Q_OBJECT

    /**jsdoc
     * Audio stats from the client.
     * @namespace AudioStats
     *
     * @hifi-interface
     * @hifi-client-entity
     * @hifi-avatar
     *
     * @property {number} pingMs <em>Read-only.</em>
     * @property {number} inputReadMsMax <em>Read-only.</em>
     * @property {number} inputUnplayedMsMax <em>Read-only.</em>
     * @property {number} outputUnplayedMsMax <em>Read-only.</em>
     * @property {number} sentTimegapMsMax <em>Read-only.</em>
     * @property {number} sentTimegapMsAvg <em>Read-only.</em>
     * @property {number} sentTimegapMsMaxWindow <em>Read-only.</em>
     * @property {number} sentTimegapMsAvgWindow <em>Read-only.</em>
     * @property {AudioStats.AudioStreamStats} clientStream <em>Read-only.</em>
     * @property {AudioStats.AudioStreamStats} mixerStream <em>Read-only.</em>
     */

    /**jsdoc
     * @function AudioStats.pingMsChanged
     * @param {number} pingMs
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(float, pingMs);


    /**jsdoc
     * @function AudioStats.inputReadMsMaxChanged
     * @param {number} inputReadMsMax
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(float, inputReadMsMax);

    /**jsdoc
     * @function AudioStats.inputUnplayedMsMaxChanged
     * @param {number} inputUnplayedMsMax
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(float, inputUnplayedMsMax);

    /**jsdoc
     * @function AudioStats.outputUnplayedMsMaxChanged
     * @param {number} outputUnplayedMsMax
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(float, outputUnplayedMsMax);


    /**jsdoc
     * @function AudioStats.sentTimegapMsMaxChanged
     * @param {number} sentTimegapMsMax
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(quint64, sentTimegapMsMax);

    /**jsdoc
     * @function AudioStats.sentTimegapMsAvgChanged
     * @param {number} sentTimegapMsAvg
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(quint64, sentTimegapMsAvg);

    /**jsdoc
     * @function AudioStats.sentTimegapMsMaxWindowChanged
     * @param {number} sentTimegapMsMaxWindow
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(quint64, sentTimegapMsMaxWindow);

    /**jsdoc
     * @function AudioStats.sentTimegapMsAvgWindowChanged
     * @param {number} sentTimegapMsAvgWindow
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(quint64, sentTimegapMsAvgWindow);

    Q_PROPERTY(AudioStreamStatsInterface* mixerStream READ getMixerStream NOTIFY mixerStreamChanged);
    Q_PROPERTY(AudioStreamStatsInterface* clientStream READ getClientStream NOTIFY clientStreamChanged);

    // FIXME: The injectorStreams property isn't available in JavaScript but the notification signal is.
    Q_PROPERTY(QObject* injectorStreams READ getInjectorStreams NOTIFY injectorStreamsChanged);

public:
    AudioStreamStatsInterface* getMixerStream() const { return _mixer; }
    AudioStreamStatsInterface* getClientStream() const { return _client; }
    QObject* getInjectorStreams() const { return _injectors; }

    void updateLocalBuffers(const MovingMinMaxAvg<float>& inputMsRead,
                            const MovingMinMaxAvg<float>& inputMsUnplayed,
                            const MovingMinMaxAvg<float>& outputMsUnplayed,
                            const MovingMinMaxAvg<quint64>& timegaps);
    void updateMixerStream(const AudioStreamStats& stats) { _mixer->updateStream(stats); emit mixerStreamChanged(); }
    void updateClientStream(const AudioStreamStats& stats) { _client->updateStream(stats); emit clientStreamChanged(); }
    void updateInjectorStreams(const QHash<QUuid, AudioStreamStats>& stats);

signals:

    /**jsdoc
     * @function AudioStats.mixerStreamChanged
     * @returns {Signal}
     */
    void mixerStreamChanged();

    /**jsdoc
     * @function AudioStats.clientStreamChanged
     * @returns {Signal}
     */
    void clientStreamChanged();

    /**jsdoc
     * @function AudioStats.injectorStreamsChanged
     * @returns {Signal}
     */
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
