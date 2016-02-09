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
    _outgoingMixedAudioSequenceNumber(0),
    _downstreamAudioStreamStats()
{

}

AvatarAudioStream* AudioMixerClientData::getAvatarAudioStream() {
    QReadLocker readLocker { &_streamsLock };
    
    auto it = _audioStreams.find(QUuid());
    if (it != _audioStreams.end()) {
        return dynamic_cast<AvatarAudioStream*>(it->second.get());
    }

    // no mic stream found - return NULL
    return NULL;
}

int AudioMixerClientData::parseData(ReceivedMessage& message) {
    PacketType packetType = message.getType();
    
    if (packetType == PacketType::AudioStreamStats) {

        // skip over header, appendFlag, and num stats packed
        message.seek(sizeof(quint8) + sizeof(quint16));

        // read the downstream audio stream stats
        message.readPrimitive(&_downstreamAudioStreamStats);

        return message.getPosition();

    } else {
        SharedStreamPointer matchingStream;

        bool isMicStream = false;

        if (packetType == PacketType::MicrophoneAudioWithEcho
            || packetType == PacketType::MicrophoneAudioNoEcho
            || packetType == PacketType::SilentAudioFrame) {

            QWriteLocker writeLocker { &_streamsLock };

            auto micStreamIt = _audioStreams.find(QUuid());
            if (micStreamIt == _audioStreams.end()) {
                // we don't have a mic stream yet, so add it

                // read the channel flag to see if our stream is stereo or not
                message.seek(sizeof(quint16));

                quint8 channelFlag;
                message.readPrimitive(&channelFlag);

                bool isStereo = channelFlag == 1;

                auto emplaced = _audioStreams.emplace(
                    QUuid(),
                    std::unique_ptr<PositionalAudioStream> { new AvatarAudioStream(isStereo, AudioMixer::getStreamSettings()) }
                );

                micStreamIt = emplaced.first;
            }

            matchingStream = micStreamIt->second;

            writeLocker.unlock();

            isMicStream = true;
        } else if (packetType == PacketType::InjectAudio) {
            // this is injected audio

            // grab the stream identifier for this injected audio
            message.seek(sizeof(quint16));
            QUuid streamIdentifier = QUuid::fromRfc4122(message.readWithoutCopy(NUM_BYTES_RFC4122_UUID));

            bool isStereo;
            message.readPrimitive(&isStereo);

            QWriteLocker writeLock { &_streamsLock };

            auto streamIt = _audioStreams.find(streamIdentifier);

            if (streamIt == _audioStreams.end()) {
                // we don't have this injected stream yet, so add it
                auto emplaced = _audioStreams.emplace(
                    streamIdentifier,
                    std::unique_ptr<InjectedAudioStream> { new InjectedAudioStream(streamIdentifier, isStereo, AudioMixer::getStreamSettings()) }
                );

                streamIt = emplaced.first;
            }

            matchingStream = streamIt->second;

            writeLock.unlock();
        }

        // seek to the beginning of the packet so that the next reader is in the right spot
        message.seek(0);

        // check the overflow count before we parse data
        auto overflowBefore = matchingStream->getOverflowCount();
        auto parseResult = matchingStream->parseData(message);

        if (matchingStream->getOverflowCount() > overflowBefore) {
            qDebug() << "Just overflowed on stream from" << message.getSourceID() << "at" << message.getSenderSockAddr();
            qDebug() << "This stream is for" << (isMicStream ? "microphone audio" : "injected audio");
        }

        return parseResult;
    }
    return 0;
}

void AudioMixerClientData::checkBuffersBeforeFrameSend() {
    QReadLocker readLocker { &_streamsLock };

    auto it = _audioStreams.cbegin();
    while (it != _audioStreams.cend()) {
        SharedStreamPointer stream = it->second;

        if (stream->popFrames(1, true) > 0) {
            stream->updateLastPopOutputLoudnessAndTrailingLoudness();
        }

        ++it;
    }
}

void AudioMixerClientData::removeDeadInjectedStreams() {

    const int INJECTOR_CONSECUTIVE_NOT_MIXED_AFTER_STARTED_THRESHOLD = 100;

    // we have this second threshold in case the injected audio is so short that the injected stream
    // never even reaches its desired size, which means it will never start.
    const int INJECTOR_CONSECUTIVE_NOT_MIXED_THRESHOLD = 1000;

    QWriteLocker writeLocker { &_streamsLock };

    auto it = _audioStreams.begin();
    while (it != _audioStreams.end()) {
        PositionalAudioStream* audioStream = it->second.get();

        if (audioStream->getType() == PositionalAudioStream::Injector && audioStream->isStarved()) {
            InjectedAudioStream* injectedStream = dynamic_cast<InjectedAudioStream*>(audioStream);
            int notMixedThreshold = audioStream->hasStarted()
                ? INJECTOR_CONSECUTIVE_NOT_MIXED_AFTER_STARTED_THRESHOLD
                : INJECTOR_CONSECUTIVE_NOT_MIXED_THRESHOLD;

            if (injectedStream->getConsecutiveNotMixedCount() >= notMixedThreshold) {
                emit injectorStreamFinished(injectedStream->getStreamIdentifier());
                it = _audioStreams.erase(it);
                continue;
            }
        }

        ++it;
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

    auto streamsCopy = getAudioStreams();

    // pack and send stream stats packets until all audio streams' stats are sent
    int numStreamStatsRemaining = streamsCopy.size();
    auto it = streamsCopy.cbegin();

    while (numStreamStatsRemaining > 0) {
        auto statsPacket = NLPacket::create(PacketType::AudioStreamStats);

        // pack the append flag in this packet
        statsPacket->writePrimitive(appendFlag);
        appendFlag = 1;

        int numStreamStatsRoomFor = (int)(statsPacket->size() - sizeof(quint8) - sizeof(quint16)) / sizeof(AudioStreamStats);

        // calculate and pack the number of stream stats to follow
        quint16 numStreamStatsToPack = std::min(numStreamStatsRemaining, numStreamStatsRoomFor);
        statsPacket->writePrimitive(numStreamStatsToPack);

        // pack the calculated number of stream stats
        for (int i = 0; i < numStreamStatsToPack; i++) {
            PositionalAudioStream* stream = it->second.get();

            stream->perSecondCallbackForUpdatingStats();

            AudioStreamStats streamStats = stream->getAudioStreamStats();
            statsPacket->writePrimitive(streamStats);

            ++it;
        }

        numStreamStatsRemaining -= numStreamStatsToPack;

        // send the current packet
        nodeList->sendPacket(std::move(statsPacket), *destinationNode);
    }
}

QJsonObject AudioMixerClientData::getAudioStreamStats() {
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

    QJsonArray injectorArray;
    auto streamsCopy = getAudioStreams();
    for (auto& injectorPair : streamsCopy) {
        if (injectorPair.second->getType() == PositionalAudioStream::Injector) {
            QJsonObject upstreamStats;

            AudioStreamStats streamStats = injectorPair.second->getAudioStreamStats();
            upstreamStats["inj.desired"]  = streamStats._desiredJitterBufferFrames;
            upstreamStats["desired_calc"] = injectorPair.second->getCalculatedJitterBufferFrames();
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

void AudioMixerClientData::printUpstreamDownstreamStats() {
    auto streamsCopy = getAudioStreams();

    // print the upstream (mic stream) stats if the mic stream exists
    auto it = streamsCopy.find(QUuid());
    if (it != streamsCopy.end()) {
        printf("Upstream:\n");
        printAudioStreamStats(it->second->getAudioStreamStats());
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
