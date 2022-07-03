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
#include <QtCore/QSharedPointer>

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

    /*@jsdoc
     * Statistics for an audio stream.
     *
     * <p>Provided in properties of the {@link AudioStats} API.</p>
     *
     * @class AudioStats.AudioStreamStats
     * @hideconstructor
     *
     * @hifi-interface
     * @hifi-client-entity
     * @hifi-avatar
     *
     * @property {number} dropCount - The number of silent or old audio frames dropped.
     *     <em>Read-only.</em>
     * @property {number} framesAvailable - The number of audio frames containing data available.
     *     <em>Read-only.</em>
     * @property {number} framesAvailableAvg - The time-weighted average of audio frames containing data available.
     *     <em>Read-only.</em>
     * @property {number} framesDesired - The desired number of audio frames for the jitter buffer.
     *     <em>Read-only.</em>
     * @property {number} lastStarveDurationCount - The most recent number of consecutive times that audio frames have not been 
     *     available for processing.
     *     <em>Read-only.</em>
     * @property {number} lossCount - The total number of audio packets lost.
     *     <em>Read-only.</em>
     * @property {number} lossCountWindow - The number of audio packets lost since the previous statistic.
     *     <em>Read-only.</em>
     * @property {number} lossRate - The ratio of the total number of audio packets lost to the total number of audio packets 
     *     expected.
     *     <em>Read-only.</em>
     * @property {number} lossRateWindow - The ratio of the number of audio packets lost to the number of audio packets 
     *     expected since the previous statistic.
     *     <em>Read-only.</em>
     * @property {number} overflowCount - The number of times that the audio ring buffer has overflowed.
     *     <em>Read-only.</em>
     * @property {number} starveCount - The total number of times that audio frames have not been available for processing.
     *     <em>Read-only.</em>
     * @property {number} timegapMsAvg - The overall average time between data packets, in ms.
     *     <em>Read-only.</em>
     * @property {number} timegapMsAvgWindow - The recent average time between data packets, in ms.
     *     <em>Read-only.</em>
     * @property {number} timegapMsMax - The overall maximum time between data packets, in ms.
     *     <em>Read-only.</em>
     * @property {number} timegapMsMaxWindow - The recent maximum time between data packets, in ms.
     *     <em>Read-only.</em>
     * @property {number} unplayedMsMax - The duration of audio waiting to be played, in ms.
     *     <em>Read-only.</em>
     */

    /*@jsdoc
     * Triggered when the ratio of the total number of audio packets lost to the total number of audio packets expected changes.
     * @function AudioStats.AudioStreamStats.lossRateChanged
     * @param {number} lossRate - The ratio of the total number of audio packets lost to the total number of audio packets 
     *     expected.
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(float, lossRate)

    /*@jsdoc
     * Triggered when the total number of audio packets lost changes.
     * @function AudioStats.AudioStreamStats.lossCountChanged
     * @param {number} lossCount - The total number of audio packets lost.
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(float, lossCount)

    /*@jsdoc
     * Triggered when the ratio of the number of audio packets lost to the number of audio packets expected since the previous 
     * statistic changes.
     * @function AudioStats.AudioStreamStats.lossRateWindowChanged
     * @param {number} lossRateWindow - The ratio of the number of audio packets lost to the number of audio packets expected 
     *     since the previous statistic.
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(float, lossRateWindow)

    /*@jsdoc
     * Triggered when the number of audio packets lost since the previous statistic changes.
     * @function AudioStats.AudioStreamStats.lossCountWindowChanged
     * @param {number} lossCountWindow - The number of audio packets lost since the previous statistic.
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(float, lossCountWindow)

    /*@jsdoc
     * Triggered when the desired number of audio frames for the jitter buffer changes.
     * @function AudioStats.AudioStreamStats.framesDesiredChanged
     * @param {number} framesDesired - The desired number of audio frames for the jitter buffer.
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(int, framesDesired)

    /*@jsdoc
     * Triggered when the number of audio frames containing data available changes.
     * @function AudioStats.AudioStreamStats.framesAvailableChanged
     * @param {number} framesAvailable - The number of audio frames containing data available.
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(int, framesAvailable)

    /*@jsdoc
     * Triggered when the time-weighted average of audio frames containing data available changes.
     * @function AudioStats.AudioStreamStats.framesAvailableAvgChanged
     * @param {number} framesAvailableAvg - The time-weighted average of audio frames containing data available.
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(int, framesAvailableAvg)

    /*@jsdoc
     * Triggered when the duration of audio waiting to be played changes.
     * @function AudioStats.AudioStreamStats.unplayedMsMaxChanged
     * @param {number} unplayedMsMax - The duration of audio waiting to be played, in ms.
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(float, unplayedMsMax)

    /*@jsdoc
     * Triggered when the total number of times that audio frames have not been available for processing changes.
     * @function AudioStats.AudioStreamStats.starveCountChanged
     * @param {number} starveCount - The total number of times that audio frames have not been available for processing.
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(int, starveCount)

    /*@jsdoc
     * Triggered when the most recenbernumber of consecutive times that audio frames have not been available for processing 
     * changes.
     * @function AudioStats.AudioStreamStats.lastStarveDurationCountChanged
     * @param {number} lastStarveDurationCount - The most recent number of consecutive times that audio frames have not been 
     *     available for processing.
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(int, lastStarveDurationCount)

    /*@jsdoc
     * Triggered when the number of silent or old audio frames dropped changes.
     * @function AudioStats.AudioStreamStats.dropCountChanged
     * @param {number} dropCount - The number of silent or old audio frames dropped.
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(int, dropCount)

    /*@jsdoc
     * Triggered when the number of times that the audio ring buffer has overflowed changes.
     * @function AudioStats.AudioStreamStats.overflowCountChanged
     * @param {number} overflowCount - The number of times that the audio ring buffer has overflowed.
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(int, overflowCount)

    /*@jsdoc
     * Triggered when the overall maximum time between data packets changes.
     * @function AudioStats.AudioStreamStats.timegapMsMaxChanged
     * @param {number} timegapMsMax - The overall maximum time between data packets, in ms.
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(quint64, timegapMsMax)

    /*@jsdoc
     * Triggered when the overall average time between data packets changes.
     * @function AudioStats.AudioStreamStats.timegapMsAvgChanged
     * @param {number} timegapMsAvg - The overall average time between data packets, in ms.
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(quint64, timegapMsAvg)

    /*@jsdoc
     * Triggered when the recent maximum time between data packets changes.
     * @function AudioStats.AudioStreamStats.timegapMsMaxWindowChanged
     * @param {number} timegapMsMaxWindow - The recent maximum time between data packets, in ms.
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(quint64, timegapMsMaxWindow)

    /*@jsdoc
     * Triggered when the recent average time between data packets changes.
     * @function AudioStats.AudioStreamStats.timegapMsAvgWindowChanged
     * @param {number} timegapMsAvgWindow - The recent average time between data packets, in ms.
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

    /*@jsdoc
     * The <code>AudioStats</code> API provides statistics of the client and mixer audio.
     *
     * @namespace AudioStats
     *
     * @hifi-interface
     * @hifi-client-entity
     * @hifi-avatar
     *
     * @property {AudioStats.AudioStreamStats} clientStream - Statistics of the client's audio stream.
     *     <em>Read-only.</em>
     * @property {number} inputReadMsMax - The maximum duration of a block of audio data recently read from the microphone, in 
     *     ms.
     *     <em>Read-only.</em>
     * @property {number} inputUnplayedMsMax - The maximum duration of microphone audio recently in the input buffer waiting to 
     *     be played, in ms.
     *     <em>Read-only.</em>
     * @property {AudioStats.AudioStreamStats} mixerStream - Statistics of the audio mixer's stream.
     *     <em>Read-only.</em>
     * @property {number} outputUnplayedMsMax - The maximum duration of output audio recently in the output buffer waiting to 
     *     be played, in ms.
     *     <em>Read-only.</em>
     * @property {number} pingMs - The current ping time to the audio mixer, in ms.
     *     <em>Read-only.</em>
     * @property {number} sentTimegapMsAvg - The overall average time between sending data packets to the audio mixer, in ms.
     *     <em>Read-only.</em>
     * @property {number} sentTimegapMsAvgWindow - The recent average time between sending data packets to the audio mixer, in 
     *     ms.
     *     <em>Read-only.</em>
     * @property {number} sentTimegapMsMax - The overall maximum time between sending data packets to the audio mixer, in ms.
     *     <em>Read-only.</em>
     * @property {number} sentTimegapMsMaxWindow - The recent maximum time between sending data packets to the audio mixer, in 
     *     ms.
     *     <em>Read-only.</em>
     */

    /*@jsdoc
     * Triggered when the ping time to the audio mixer changes.
     * @function AudioStats.pingMsChanged
     * @param {number} pingMs - The ping time to the audio mixer, in ms.
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(float, pingMs);


    /*@jsdoc
     * Triggered when the maximum duration of a block of audio data recently read from the microphone changes.
     * @function AudioStats.inputReadMsMaxChanged
     * @param {number} inputReadMsMax - The maximum duration of a block of audio data recently read from the microphone, in ms.
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(float, inputReadMsMax);

    /*@jsdoc
     * Triggered when the maximum duration of microphone audio recently in the input buffer waiting to be played changes.
     * @function AudioStats.inputUnplayedMsMaxChanged
     * @param {number} inputUnplayedMsMax - The maximum duration of microphone audio recently in the input buffer waiting to be 
     *     played, in ms.
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(float, inputUnplayedMsMax);

    /*@jsdoc
     * Triggered when the maximum duration of output audio recently in the output buffer waiting to be played changes.
     * @function AudioStats.outputUnplayedMsMaxChanged
     * @param {number} outputUnplayedMsMax - The maximum duration of output audio recently in the output buffer waiting to be 
     *     played, in ms.
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(float, outputUnplayedMsMax);


    /*@jsdoc
     * Triggered when the overall maximum time between sending data packets to the audio mixer changes.
     * @function AudioStats.sentTimegapMsMaxChanged
     * @param {number} sentTimegapMsMax -  The overall maximum time between sending data packets to the audio mixer, in ms.
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(quint64, sentTimegapMsMax);

    /*@jsdoc
     * Triggered when the overall average time between sending data packets to the audio mixer changes.
     * @function AudioStats.sentTimegapMsAvgChanged
     * @param {number} sentTimegapMsAvg - The overall average time between sending data packets to the audio mixer, in ms.
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(quint64, sentTimegapMsAvg);

    /*@jsdoc
     * Triggered when the recent maximum time between sending data packets to the audio mixer changes.
     * @function AudioStats.sentTimegapMsMaxWindowChanged
     * @param {number} sentTimegapMsMaxWindow - The recent maximum time between sending data packets to the audio mixer, in ms.
     * @returns {Signal} 
     */
    AUDIO_PROPERTY(quint64, sentTimegapMsMaxWindow);

    /*@jsdoc
     * Triggered when the recent average time between sending data packets to the audio mixer changes.
     * @function AudioStats.sentTimegapMsAvgWindowChanged
     * @param {number} sentTimegapMsAvgWindow - The recent average time between sending data packets to the audio mixer, in 
     *     ms.
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

    /*@jsdoc
     * Triggered when the mixer's stream statistics have been updated.
     * @function AudioStats.mixerStreamChanged
     * @returns {Signal}
     */
    void mixerStreamChanged();

    /*@jsdoc
     * Triggered when the client's stream statisticss have been updated.
     * @function AudioStats.clientStreamChanged
     * @returns {Signal}
     */
    void clientStreamChanged();

    /*@jsdoc
     * Triggered when the injector streams' statistics have been updated.
     * <p><strong>Note:</strong> The injector streams' statistics are currently not provided.</p>
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
