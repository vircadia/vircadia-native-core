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

#include <QDebug>

#include <PacketHeaders.h>
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
}

AvatarAudioStream* AudioMixerClientData::getAvatarAudioStream() const {
    if (_audioStreams.contains(QUuid())) {
        return (AvatarAudioStream*)_audioStreams.value(QUuid());
    }
    // no mic stream found - return NULL
    return NULL;
}

int AudioMixerClientData::parseData(const QByteArray& packet) {
    PacketType packetType = packetTypeForPacket(packet);
    if (packetType == PacketTypeAudioStreamStats) {

        const char* dataAt = packet.data();

        // skip over header, appendFlag, and num stats packed
        dataAt += (numBytesForPacketHeader(packet) + sizeof(quint8) + sizeof(quint16));

        // read the downstream audio stream stats
        memcpy(&_downstreamAudioStreamStats, dataAt, sizeof(AudioStreamStats));
        dataAt += sizeof(AudioStreamStats);

        return dataAt - packet.data();

    } else {
        PositionalAudioStream* matchingStream = NULL;

        if (packetType == PacketTypeMicrophoneAudioWithEcho
            || packetType == PacketTypeMicrophoneAudioNoEcho
            || packetType == PacketTypeSilentAudioFrame) {

            QUuid nullUUID = QUuid();
            if (!_audioStreams.contains(nullUUID)) {
                // we don't have a mic stream yet, so add it

                // read the channel flag to see if our stream is stereo or not
                const char* channelFlagAt = packet.constData() + numBytesForPacketHeader(packet) + sizeof(quint16);
                quint8 channelFlag = *(reinterpret_cast<const quint8*>(channelFlagAt));
                bool isStereo = channelFlag == 1;

                _audioStreams.insert(nullUUID,
                    matchingStream = new AvatarAudioStream(isStereo, AudioMixer::getUseDynamicJitterBuffers(),
                    AudioMixer::getStaticDesiredJitterBufferFrames(), AudioMixer::getMaxFramesOverDesired()));
            } else {
                matchingStream = _audioStreams.value(nullUUID);
            }
        } else if (packetType == PacketTypeInjectAudio) {
            // this is injected audio

            // grab the stream identifier for this injected audio
            int bytesBeforeStreamIdentifier = numBytesForPacketHeader(packet) + sizeof(quint16);
            QUuid streamIdentifier = QUuid::fromRfc4122(packet.mid(bytesBeforeStreamIdentifier, NUM_BYTES_RFC4122_UUID));

            if (!_audioStreams.contains(streamIdentifier)) {
                _audioStreams.insert(streamIdentifier,
                    matchingStream = new InjectedAudioStream(streamIdentifier, AudioMixer::getUseDynamicJitterBuffers(),
                    AudioMixer::getStaticDesiredJitterBufferFrames(), AudioMixer::getMaxFramesOverDesired()));
            } else {
                matchingStream = _audioStreams.value(streamIdentifier);
            }
        }

        return matchingStream->parseData(packet);
    }
    return 0;
}

void AudioMixerClientData::checkBuffersBeforeFrameSend(AABox* checkSourceZone, AABox* listenerZone) {
    QHash<QUuid, PositionalAudioStream*>::ConstIterator i;
    for (i = _audioStreams.constBegin(); i != _audioStreams.constEnd(); i++) {
        PositionalAudioStream* stream = i.value();
        if (stream->popFrames(1, true) > 0) {
            // this is a ring buffer that is ready to go

            // calculate the trailing avg loudness for the next frame
            // that would be mixed in
            stream->updateLastPopOutputTrailingLoudness();

            if (checkSourceZone && checkSourceZone->contains(stream->getPosition())) {
                stream->setListenerUnattenuatedZone(listenerZone);
            } else {
                stream->setListenerUnattenuatedZone(NULL);
            }
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

    char packet[MAX_PACKET_SIZE];
    NodeList* nodeList = NodeList::getInstance();

    // The append flag is a boolean value that will be packed right after the header.  The first packet sent 
    // inside this method will have 0 for this flag, while every subsequent packet will have 1 for this flag.
    // The sole purpose of this flag is so the client can clear its map of injected audio stream stats when
    // it receives a packet with an appendFlag of 0. This prevents the buildup of dead audio stream stats in the client.
    quint8 appendFlag = 0;

    // pack header
    int numBytesPacketHeader = populatePacketHeader(packet, PacketTypeAudioStreamStats);
    char* headerEndAt = packet + numBytesPacketHeader;

    // calculate how many stream stat structs we can fit in each packet
    const int numStreamStatsRoomFor = (MAX_PACKET_SIZE - numBytesPacketHeader - sizeof(quint8) - sizeof(quint16)) / sizeof(AudioStreamStats);

    // pack and send stream stats packets until all audio streams' stats are sent
    int numStreamStatsRemaining = _audioStreams.size();
    QHash<QUuid, PositionalAudioStream*>::ConstIterator audioStreamsIterator = _audioStreams.constBegin();
    while (numStreamStatsRemaining > 0) {

        char* dataAt = headerEndAt;

        // pack the append flag
        memcpy(dataAt, &appendFlag, sizeof(quint8));
        appendFlag = 1;
        dataAt += sizeof(quint8);

        // calculate and pack the number of stream stats to follow
        quint16 numStreamStatsToPack = std::min(numStreamStatsRemaining, numStreamStatsRoomFor);
        memcpy(dataAt, &numStreamStatsToPack, sizeof(quint16));
        dataAt += sizeof(quint16);

        // pack the calculated number of stream stats
        for (int i = 0; i < numStreamStatsToPack; i++) {
            AudioStreamStats streamStats = audioStreamsIterator.value()->updateSeqHistoryAndGetAudioStreamStats();
            memcpy(dataAt, &streamStats, sizeof(AudioStreamStats));
            dataAt += sizeof(AudioStreamStats);

            audioStreamsIterator++;
        }
        numStreamStatsRemaining -= numStreamStatsToPack;

        // send the current packet
        nodeList->writeDatagram(packet, dataAt - packet, destinationNode);
    }
}

QString AudioMixerClientData::getAudioStreamStatsString() const {
    QString result;
    AudioStreamStats streamStats = _downstreamAudioStreamStats;
    result += "DOWNSTREAM.desired:" + QString::number(streamStats._desiredJitterBufferFrames)
        + " available_avg_10s:" + QString::number(streamStats._framesAvailableAverage)
        + " available:" + QString::number(streamStats._framesAvailable)
        + " starves:" + QString::number(streamStats._starveCount)
        + " not_mixed:" + QString::number(streamStats._consecutiveNotMixedCount)
        + " overflows:" + QString::number(streamStats._overflowCount)
        + " silents_dropped: ?"
        + " lost%:" + QString::number(streamStats._packetStreamStats.getLostRate() * 100.0f, 'f', 2)
        + " lost%_30s:" + QString::number(streamStats._packetStreamWindowStats.getLostRate() * 100.0f, 'f', 2)
        + " min_gap:" + formatUsecTime(streamStats._timeGapMin)
        + " max_gap:" + formatUsecTime(streamStats._timeGapMax)
        + " avg_gap:" + formatUsecTime(streamStats._timeGapAverage)
        + " min_gap_30s:" + formatUsecTime(streamStats._timeGapWindowMin)
        + " max_gap_30s:" + formatUsecTime(streamStats._timeGapWindowMax)
        + " avg_gap_30s:" + formatUsecTime(streamStats._timeGapWindowAverage);

    AvatarAudioStream* avatarAudioStream = getAvatarAudioStream();
    if (avatarAudioStream) {
        AudioStreamStats streamStats = avatarAudioStream->getAudioStreamStats();
        result += " UPSTREAM.mic.desired:" + QString::number(streamStats._desiredJitterBufferFrames)
            + " desired_calc:" + QString::number(avatarAudioStream->getCalculatedJitterBufferFrames())
            + " available_avg_10s:" + QString::number(streamStats._framesAvailableAverage)
            + " available:" + QString::number(streamStats._framesAvailable)
            + " starves:" + QString::number(streamStats._starveCount)
            + " not_mixed:" + QString::number(streamStats._consecutiveNotMixedCount)
            + " overflows:" + QString::number(streamStats._overflowCount)
            + " silents_dropped:" + QString::number(streamStats._framesDropped)
            + " lost%:" + QString::number(streamStats._packetStreamStats.getLostRate() * 100.0f, 'f', 2)
            + " lost%_30s:" + QString::number(streamStats._packetStreamWindowStats.getLostRate() * 100.0f, 'f', 2)
            + " min_gap:" + formatUsecTime(streamStats._timeGapMin)
            + " max_gap:" + formatUsecTime(streamStats._timeGapMax)
            + " avg_gap:" + formatUsecTime(streamStats._timeGapAverage)
            + " min_gap_30s:" + formatUsecTime(streamStats._timeGapWindowMin)
            + " max_gap_30s:" + formatUsecTime(streamStats._timeGapWindowMax)
            + " avg_gap_30s:" + formatUsecTime(streamStats._timeGapWindowAverage);
    } else {
        result = "mic unknown";
    }
    
    QHash<QUuid, PositionalAudioStream*>::ConstIterator i;
    for (i = _audioStreams.constBegin(); i != _audioStreams.constEnd(); i++) {
        if (i.value()->getType() == PositionalAudioStream::Injector) {
            AudioStreamStats streamStats = i.value()->getAudioStreamStats();
            result += " UPSTREAM.inj.desired:" + QString::number(streamStats._desiredJitterBufferFrames)
                + " desired_calc:" + QString::number(i.value()->getCalculatedJitterBufferFrames())
                + " available_avg_10s:" + QString::number(streamStats._framesAvailableAverage)
                + " available:" + QString::number(streamStats._framesAvailable)
                + " starves:" + QString::number(streamStats._starveCount)
                + " not_mixed:" + QString::number(streamStats._consecutiveNotMixedCount)
                + " overflows:" + QString::number(streamStats._overflowCount)
                + " silents_dropped:" + QString::number(streamStats._framesDropped)
                + " lost%:" + QString::number(streamStats._packetStreamStats.getLostRate() * 100.0f, 'f', 2)
                + " lost%_30s:" + QString::number(streamStats._packetStreamWindowStats.getLostRate() * 100.0f, 'f', 2)
                + " min_gap:" + formatUsecTime(streamStats._timeGapMin)
                + " max_gap:" + formatUsecTime(streamStats._timeGapMax)
                + " avg_gap:" + formatUsecTime(streamStats._timeGapAverage)
                + " min_gap_30s:" + formatUsecTime(streamStats._timeGapWindowMin)
                + " max_gap_30s:" + formatUsecTime(streamStats._timeGapWindowMax)
                + " avg_gap_30s:" + formatUsecTime(streamStats._timeGapWindowAverage);
        }
    }
    return result;
}

void AudioMixerClientData::printUpstreamDownstreamStats() const {
    // print the upstream (mic stream) stats if the mic stream exists
    if (_audioStreams.contains(QUuid())) {
        printf("    Upstream:\n");
        printAudioStreamStats(_audioStreams.value(QUuid())->getAudioStreamStats());
    }
    // print the downstream stats if they contain valid info
    if (_downstreamAudioStreamStats._packetStreamStats._received > 0) {
        printf("    Downstream:\n");
        printAudioStreamStats(_downstreamAudioStreamStats);
    }
}

void AudioMixerClientData::printAudioStreamStats(const AudioStreamStats& streamStats) const {
    printf("                      Packet loss | overall: %5.2f%% (%d lost), last_30s: %5.2f%% (%d lost)",
        streamStats._packetStreamStats.getLostRate() * 100.0f,
        streamStats._packetStreamStats._lost,
        streamStats._packetStreamWindowStats.getLostRate() * 100.0f,
        streamStats._packetStreamWindowStats._lost);

    printf("                Ringbuffer frames | desired: %u, avg_available(10s): %u, available: %u",
        streamStats._desiredJitterBufferFrames,
        streamStats._framesAvailableAverage,
        streamStats._framesAvailable);
    
    printf("                 Ringbuffer stats | starves: %u, prev_starve_lasted: %u, frames_dropped: %u, overflows: %u",
        streamStats._starveCount,
        streamStats._consecutiveNotMixedCount,
        streamStats._framesDropped,
        streamStats._overflowCount);

    printf("  Inter-packet timegaps (overall) | min: %9s, max: %9s, avg: %9s",
        formatUsecTime(streamStats._timeGapMin).toLatin1().data(),
        formatUsecTime(streamStats._timeGapMax).toLatin1().data(),
        formatUsecTime(streamStats._timeGapAverage).toLatin1().data());

    printf(" Inter-packet timegaps (last 30s) | min: %9s, max: %9s, avg: %9s",
        formatUsecTime(streamStats._timeGapWindowMin).toLatin1().data(),
        formatUsecTime(streamStats._timeGapWindowMax).toLatin1().data(),
        formatUsecTime(streamStats._timeGapWindowAverage).toLatin1().data());
}
