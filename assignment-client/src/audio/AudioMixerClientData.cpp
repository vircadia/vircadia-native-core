//
//  AudioMixerClientData.cpp
//  assignment-client/src/audio
//
//  Created by Stephen Birarda on 10/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QDebug>
#include <QtCore/QJsonArray>

#include <udt/PacketHeaders.h>
#include <UUID.h>

#include "InjectedAudioStream.h"

#include "AudioMixer.h"
#include "AudioMixerClientData.h"


AudioMixerClientData::AudioMixerClientData() :
    _audioStreams(),
    _outgoingMixedAudioSequenceNumber(0),
    _downstreamAudioStreamStats()
{
}

AudioMixerClientData::~AudioMixerClientData() {
    QHash<QUuid, PositionalAudioStream*>::ConstIterator i;
    for (i = _audioStreams.constBegin(); i != _audioStreams.constEnd(); i++) {
        // delete this attached InboundAudioStream
        delete i.value();
    }

    // clean up our pair data...
    foreach(PerListenerSourcePairData* pairData, _listenerSourcePairData) {
        delete pairData;
    }
}

AvatarAudioStream* AudioMixerClientData::getAvatarAudioStream() const {
    if (_audioStreams.contains(QUuid())) {
        return (AvatarAudioStream*)_audioStreams.value(QUuid());
    }
    // no mic stream found - return NULL
    return NULL;
}

int AudioMixerClientData::parseData(NLPacket& packet) {
    PacketType packetType = packet.getType();
    
    if (packetType == PacketType::AudioStreamStats) {

        // skip over header, appendFlag, and num stats packed
        packet.seek(sizeof(quint8) + sizeof(quint16));

        // read the downstream audio stream stats
        packet.readPrimitive(&_downstreamAudioStreamStats);

        return packet.pos();

    } else {
        PositionalAudioStream* matchingStream = NULL;

        if (packetType == PacketType::MicrophoneAudioWithEcho
            || packetType == PacketType::MicrophoneAudioNoEcho
            || packetType == PacketType::SilentAudioFrame) {

            QUuid nullUUID = QUuid();
            if (!_audioStreams.contains(nullUUID)) {
                // we don't have a mic stream yet, so add it

                // read the channel flag to see if our stream is stereo or not
                packet.seek(sizeof(quint16));

                quint8 channelFlag;
                packet.readPrimitive(&channelFlag);

                bool isStereo = channelFlag == 1;

                _audioStreams.insert(nullUUID, matchingStream = new AvatarAudioStream(isStereo, AudioMixer::getStreamSettings()));
            } else {
                matchingStream = _audioStreams.value(nullUUID);
            }
        } else if (packetType == PacketType::InjectAudio) {
            // this is injected audio

            // grab the stream identifier for this injected audio
            packet.seek(sizeof(quint16));
            QUuid streamIdentifier = QUuid::fromRfc4122(packet.readWithoutCopy(NUM_BYTES_RFC4122_UUID));

            bool isStereo;
            packet.readPrimitive(&isStereo);

            if (!_audioStreams.contains(streamIdentifier)) {
                // we don't have this injected stream yet, so add it
                _audioStreams.insert(streamIdentifier,
                        matchingStream = new InjectedAudioStream(streamIdentifier, isStereo, AudioMixer::getStreamSettings()));
            } else {
                matchingStream = _audioStreams.value(streamIdentifier);
            }
        }

        // seek to the beginning of the packet so that the next reader is in the right spot
        packet.seek(0);

        return matchingStream->parseData(packet);
    }
    return 0;
}

void AudioMixerClientData::checkBuffersBeforeFrameSend() {
    QHash<QUuid, PositionalAudioStream*>::ConstIterator i;
    for (i = _audioStreams.constBegin(); i != _audioStreams.constEnd(); i++) {
        PositionalAudioStream* stream = i.value();

        if (stream->popFrames(1, true) > 0) {
            stream->updateLastPopOutputLoudnessAndTrailingLoudness();
        }
    }
}

void AudioMixerClientData::removeDeadInjectedStreams() {

    const int INJECTOR_CONSECUTIVE_NOT_MIXED_AFTER_STARTED_THRESHOLD = 100;

    // we have this second threshold in case the injected audio is so short that the injected stream
    // never even reaches its desired size, which means it will never start.
    const int INJECTOR_CONSECUTIVE_NOT_MIXED_THRESHOLD = 1000;

    QHash<QUuid, PositionalAudioStream*>::Iterator i = _audioStreams.begin(), end = _audioStreams.end();
    while (i != end) {
        PositionalAudioStream* audioStream = i.value();
        if (audioStream->getType() == PositionalAudioStream::Injector && audioStream->isStarved()) {
            int notMixedThreshold = audioStream->hasStarted() ? INJECTOR_CONSECUTIVE_NOT_MIXED_AFTER_STARTED_THRESHOLD
                                                              : INJECTOR_CONSECUTIVE_NOT_MIXED_THRESHOLD;
            if (audioStream->getConsecutiveNotMixedCount() >= notMixedThreshold) {
                delete audioStream;
                i = _audioStreams.erase(i);
                continue;
            }
        }
        ++i;
    }
}

void AudioMixerClientData::sendAudioStreamStatsPackets(const SharedNodePointer& destinationNode) {

    // since audio stream stats packets are sent periodically, this is a good place to remove our dead injected streams.
    removeDeadInjectedStreams();

    auto nodeList = DependencyManager::get<NodeList>();

    // The append flag is a boolean value that will be packed right after the header.  The first packet sent
    // inside this method will have 0 for this flag, while every subsequent packet will have 1 for this flag.
    // The sole purpose of this flag is so the client can clear its map of injected audio stream stats when
    // it receives a packet with an appendFlag of 0. This prevents the buildup of dead audio stream stats in the client.
    quint8 appendFlag = 0;

    // pack and send stream stats packets until all audio streams' stats are sent
    int numStreamStatsRemaining = _audioStreams.size();
    QHash<QUuid, PositionalAudioStream*>::ConstIterator audioStreamsIterator = _audioStreams.constBegin();

    while (numStreamStatsRemaining > 0) {
        auto statsPacket = NLPacket::create(PacketType::AudioStreamStats);

        // pack the append flag in this packet
        statsPacket->writePrimitive(appendFlag);
        appendFlag = 1;

        int numStreamStatsRoomFor = (statsPacket->size() - sizeof(quint8) - sizeof(quint16)) / sizeof(AudioStreamStats);

        // calculate and pack the number of stream stats to follow
        quint16 numStreamStatsToPack = std::min(numStreamStatsRemaining, numStreamStatsRoomFor);
        statsPacket->writePrimitive(numStreamStatsToPack);

        // pack the calculated number of stream stats
        for (int i = 0; i < numStreamStatsToPack; i++) {
            PositionalAudioStream* stream = audioStreamsIterator.value();

            stream->perSecondCallbackForUpdatingStats();

            AudioStreamStats streamStats = stream->getAudioStreamStats();
            statsPacket->writePrimitive(streamStats);

            audioStreamsIterator++;
        }

        numStreamStatsRemaining -= numStreamStatsToPack;

        // send the current packet
        nodeList->sendPacket(std::move(statsPacket), *destinationNode);
    }
}

QJsonObject AudioMixerClientData::getAudioStreamStats() const {
    QJsonObject result;

    QJsonObject downstreamStats;
    AudioStreamStats streamStats = _downstreamAudioStreamStats;
    downstreamStats["desired"] = streamStats._desiredJitterBufferFrames;
    downstreamStats["available_avg_10s"] = streamStats._framesAvailableAverage;
    downstreamStats["available"] = (double) streamStats._framesAvailable;
    downstreamStats["starves"] = (double) streamStats._starveCount;
    downstreamStats["not_mixed"] = (double) streamStats._consecutiveNotMixedCount;
    downstreamStats["overflows"] = (double) streamStats._overflowCount;
    downstreamStats["lost%"] = streamStats._packetStreamStats.getLostRate() * 100.0f;
    downstreamStats["lost%_30s"] = streamStats._packetStreamWindowStats.getLostRate() * 100.0f;
    downstreamStats["min_gap"] = formatUsecTime(streamStats._timeGapMin);
    downstreamStats["max_gap"] = formatUsecTime(streamStats._timeGapMax);
    downstreamStats["avg_gap"] = formatUsecTime(streamStats._timeGapAverage);
    downstreamStats["min_gap_30s"] = formatUsecTime(streamStats._timeGapWindowMin);
    downstreamStats["max_gap_30s"] = formatUsecTime(streamStats._timeGapWindowMax);
    downstreamStats["avg_gap_30s"] = formatUsecTime(streamStats._timeGapWindowAverage);

    result["downstream"] = downstreamStats;

    AvatarAudioStream* avatarAudioStream = getAvatarAudioStream();

    if (avatarAudioStream) {
        QJsonObject upstreamStats;

        AudioStreamStats streamStats = avatarAudioStream->getAudioStreamStats();
        upstreamStats["mic.desired"] = streamStats._desiredJitterBufferFrames;
        upstreamStats["desired_calc"] = avatarAudioStream->getCalculatedJitterBufferFrames();
        upstreamStats["available_avg_10s"] = streamStats._framesAvailableAverage;
        upstreamStats["available"] = (double) streamStats._framesAvailable;
        upstreamStats["starves"] = (double) streamStats._starveCount;
        upstreamStats["not_mixed"] = (double) streamStats._consecutiveNotMixedCount;
        upstreamStats["overflows"] = (double) streamStats._overflowCount;
        upstreamStats["silents_dropped"] = (double) streamStats._framesDropped;
        upstreamStats["lost%"] = streamStats._packetStreamStats.getLostRate() * 100.0f;
        upstreamStats["lost%_30s"] = streamStats._packetStreamWindowStats.getLostRate() * 100.0f;
        upstreamStats["min_gap"] = formatUsecTime(streamStats._timeGapMin);
        upstreamStats["max_gap"] = formatUsecTime(streamStats._timeGapMax);
        upstreamStats["avg_gap"] = formatUsecTime(streamStats._timeGapAverage);
        upstreamStats["min_gap_30s"] = formatUsecTime(streamStats._timeGapWindowMin);
        upstreamStats["max_gap_30s"] = formatUsecTime(streamStats._timeGapWindowMax);
        upstreamStats["avg_gap_30s"] = formatUsecTime(streamStats._timeGapWindowAverage);

        result["upstream"] = upstreamStats;
    } else {
        result["upstream"] = "mic unknown";
    }

    QHash<QUuid, PositionalAudioStream*>::ConstIterator i;
    QJsonArray injectorArray;
    for (i = _audioStreams.constBegin(); i != _audioStreams.constEnd(); i++) {
        if (i.value()->getType() == PositionalAudioStream::Injector) {
            QJsonObject upstreamStats;

            AudioStreamStats streamStats = i.value()->getAudioStreamStats();
            upstreamStats["inj.desired"]  = streamStats._desiredJitterBufferFrames;
            upstreamStats["desired_calc"] = i.value()->getCalculatedJitterBufferFrames();
            upstreamStats["available_avg_10s"] = streamStats._framesAvailableAverage;
            upstreamStats["available"] = (double) streamStats._framesAvailable;
            upstreamStats["starves"] = (double) streamStats._starveCount;
            upstreamStats["not_mixed"] = (double) streamStats._consecutiveNotMixedCount;
            upstreamStats["overflows"] = (double) streamStats._overflowCount;
            upstreamStats["silents_dropped"] = (double) streamStats._framesDropped;
            upstreamStats["lost%"] = streamStats._packetStreamStats.getLostRate() * 100.0f;
            upstreamStats["lost%_30s"] = streamStats._packetStreamWindowStats.getLostRate() * 100.0f;
            upstreamStats["min_gap"] = formatUsecTime(streamStats._timeGapMin);
            upstreamStats["max_gap"] = formatUsecTime(streamStats._timeGapMax);
            upstreamStats["avg_gap"] = formatUsecTime(streamStats._timeGapAverage);
            upstreamStats["min_gap_30s"] = formatUsecTime(streamStats._timeGapWindowMin);
            upstreamStats["max_gap_30s"] = formatUsecTime(streamStats._timeGapWindowMax);
            upstreamStats["avg_gap_30s"] = formatUsecTime(streamStats._timeGapWindowAverage);

            injectorArray.push_back(upstreamStats);
        }
    }

    result["injectors"] = injectorArray;

    return result;
}

void AudioMixerClientData::printUpstreamDownstreamStats() const {
    // print the upstream (mic stream) stats if the mic stream exists
    if (_audioStreams.contains(QUuid())) {
        printf("Upstream:\n");
        printAudioStreamStats(_audioStreams.value(QUuid())->getAudioStreamStats());
    }
    // print the downstream stats if they contain valid info
    if (_downstreamAudioStreamStats._packetStreamStats._received > 0) {
        printf("Downstream:\n");
        printAudioStreamStats(_downstreamAudioStreamStats);
    }
}

void AudioMixerClientData::printAudioStreamStats(const AudioStreamStats& streamStats) const {
    printf("                      Packet loss | overall: %5.2f%% (%d lost), last_30s: %5.2f%% (%d lost)\n",
           (double)(streamStats._packetStreamStats.getLostRate() * 100.0f),
           streamStats._packetStreamStats._lost,
           (double)(streamStats._packetStreamWindowStats.getLostRate() * 100.0f),
           streamStats._packetStreamWindowStats._lost);

    printf("                Ringbuffer frames | desired: %u, avg_available(10s): %u, available: %u\n",
        streamStats._desiredJitterBufferFrames,
        streamStats._framesAvailableAverage,
        streamStats._framesAvailable);

    printf("                 Ringbuffer stats | starves: %u, prev_starve_lasted: %u, frames_dropped: %u, overflows: %u\n",
        streamStats._starveCount,
        streamStats._consecutiveNotMixedCount,
        streamStats._framesDropped,
        streamStats._overflowCount);

    printf("  Inter-packet timegaps (overall) | min: %9s, max: %9s, avg: %9s\n",
        formatUsecTime(streamStats._timeGapMin).toLatin1().data(),
        formatUsecTime(streamStats._timeGapMax).toLatin1().data(),
        formatUsecTime(streamStats._timeGapAverage).toLatin1().data());

    printf(" Inter-packet timegaps (last 30s) | min: %9s, max: %9s, avg: %9s\n",
        formatUsecTime(streamStats._timeGapWindowMin).toLatin1().data(),
        formatUsecTime(streamStats._timeGapWindowMax).toLatin1().data(),
        formatUsecTime(streamStats._timeGapWindowAverage).toLatin1().data());
}


PerListenerSourcePairData* AudioMixerClientData::getListenerSourcePairData(const QUuid& sourceUUID) {
    if (!_listenerSourcePairData.contains(sourceUUID)) {
        PerListenerSourcePairData* newData = new PerListenerSourcePairData();
        _listenerSourcePairData[sourceUUID] = newData;
    }
    return _listenerSourcePairData[sourceUUID];
}
