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

#include "InjectedAudioRingBuffer.h"

#include "AudioMixer.h"
#include "AudioMixerClientData.h"


AudioMixerClientData::AudioMixerClientData() :
    _ringBuffers(),
    _outgoingMixedAudioSequenceNumber(0)
{
}

AudioMixerClientData::~AudioMixerClientData() {
    QHash<QUuid, PositionalAudioRingBuffer*>::ConstIterator i, end = _ringBuffers.constEnd();
    for (i = _ringBuffers.constBegin(); i != end; i++) {
        // delete this attached InboundAudioStream
        delete i.value();
    }
}

AvatarAudioRingBuffer* AudioMixerClientData::getAvatarAudioRingBuffer() const {
    if (_ringBuffers.contains(QUuid())) {
        return (AvatarAudioRingBuffer*)_ringBuffers.value(QUuid());
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
        PositionalAudioRingBuffer* matchingStream = NULL;

        if (packetType == PacketTypeMicrophoneAudioWithEcho
            || packetType == PacketTypeMicrophoneAudioNoEcho
            || packetType == PacketTypeSilentAudioFrame) {

            QUuid nullUUID = QUuid();
            if (!_ringBuffers.contains(nullUUID)) {
                // we don't have a mic stream yet, so add it

                // read the channel flag to see if our stream is stereo or not
                const char* channelFlagAt = packet.constData() + numBytesForPacketHeader(packet) + sizeof(quint16);
                quint8 channelFlag = *(reinterpret_cast<const quint8*>(channelFlagAt));
                bool isStereo = channelFlag == 1;

                _ringBuffers.insert(nullUUID,
                    matchingStream = new AvatarAudioRingBuffer(isStereo, AudioMixer::getUseDynamicJitterBuffers()));
            } else {
                matchingStream = _ringBuffers.value(nullUUID);
            }
        } else if (packetType == PacketTypeInjectAudio) {
            // this is injected audio

            // grab the stream identifier for this injected audio
            int bytesBeforeStreamIdentifier = numBytesForPacketHeader(packet) + sizeof(quint16);
            QUuid streamIdentifier = QUuid::fromRfc4122(packet.mid(bytesBeforeStreamIdentifier, NUM_BYTES_RFC4122_UUID));

            if (!_ringBuffers.contains(streamIdentifier)) {
                _ringBuffers.insert(streamIdentifier,
                    matchingStream = new InjectedAudioRingBuffer(streamIdentifier, AudioMixer::getUseDynamicJitterBuffers()));
            } else {
                matchingStream = _ringBuffers.value(streamIdentifier);
            }
        }

        return matchingStream->parseData(packet);
    }
    return 0;
}

/*void AudioMixerClientData::checkBuffersBeforeFrameSend(AABox* checkSourceZone, AABox* listenerZone) {
    for (int i = 0; i < _ringBuffers.size(); i++) {
        if (_ringBuffers[i]->shouldBeAddedToMix()) {
            // this is a ring buffer that is ready to go
            // set its flag so we know to push its buffer when all is said and done
            _ringBuffers[i]->setWillBeAddedToMix(true);
            
            // calculate the average loudness for the next NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL
            // that would be mixed in
            _ringBuffers[i]->updateNextOutputTrailingLoudness();
            
            if (checkSourceZone && checkSourceZone->contains(_ringBuffers[i]->getPosition())) {
                _ringBuffers[i]->setListenerUnattenuatedZone(listenerZone);
            } else {
                _ringBuffers[i]->setListenerUnattenuatedZone(NULL);
            }
        }
    }
}

void AudioMixerClientData::pushBuffersAfterFrameSend() {

    QList<PositionalAudioRingBuffer*>::iterator i = _ringBuffers.begin();
    while (i != _ringBuffers.end()) {
        // this was a used buffer, push the output pointer forwards
        PositionalAudioRingBuffer* audioBuffer = *i;

        const int INJECTOR_CONSECUTIVE_NOT_MIXED_THRESHOLD = 100;

        if (audioBuffer->willBeAddedToMix()) {
            audioBuffer->shiftReadPosition(audioBuffer->getSamplesPerFrame());
            audioBuffer->setWillBeAddedToMix(false);
        } else if (audioBuffer->getType() == PositionalAudioRingBuffer::Injector
                   && audioBuffer->hasStarted() && audioBuffer->isStarved()
                   && audioBuffer->getConsecutiveNotMixedCount() > INJECTOR_CONSECUTIVE_NOT_MIXED_THRESHOLD) {
            // this is an empty audio buffer that has starved, safe to delete
            // also delete its sequence number stats
            QUuid streamIdentifier = ((InjectedAudioRingBuffer*)audioBuffer)->getStreamIdentifier();
            _incomingInjectedAudioSequenceNumberStatsMap.remove(streamIdentifier);
            delete audioBuffer;
            i = _ringBuffers.erase(i);
            continue;
        }
        i++;
    }
}*/

/*AudioStreamStats AudioMixerClientData::getAudioStreamStatsOfStream(const PositionalAudioRingBuffer* ringBuffer) const {
    
    AudioStreamStats streamStats;

    streamStats._streamType = ringBuffer->getType();
    if (streamStats._streamType == PositionalAudioRingBuffer::Injector) {
        streamStats._streamIdentifier = ((InjectedAudioRingBuffer*)ringBuffer)->getStreamIdentifier();
        const SequenceNumberStats& sequenceNumberStats = _incomingInjectedAudioSequenceNumberStatsMap[streamStats._streamIdentifier];
        streamStats._packetStreamStats = sequenceNumberStats.getStats();
        streamStats._packetStreamWindowStats = sequenceNumberStats.getStatsForHistoryWindow();
    } else {
        streamStats._packetStreamStats = _incomingAvatarAudioSequenceNumberStats.getStats();
        streamStats._packetStreamWindowStats = _incomingAvatarAudioSequenceNumberStats.getStatsForHistoryWindow();
    }

    const MovingMinMaxAvg<quint64>& timeGapStats = ringBuffer->getInterframeTimeGapStatsForStatsPacket();
    streamStats._timeGapMin = timeGapStats.getMin();
    streamStats._timeGapMax = timeGapStats.getMax();
    streamStats._timeGapAverage = timeGapStats.getAverage();
    streamStats._timeGapWindowMin = timeGapStats.getWindowMin();
    streamStats._timeGapWindowMax = timeGapStats.getWindowMax();
    streamStats._timeGapWindowAverage = timeGapStats.getWindowAverage();

    streamStats._ringBufferFramesAvailable = ringBuffer->framesAvailable();
    streamStats._ringBufferFramesAvailableAverage = ringBuffer->getFramesAvailableAverage();
    streamStats._ringBufferDesiredJitterBufferFrames = ringBuffer->getDesiredJitterBufferFrames();
    streamStats._ringBufferStarveCount = ringBuffer->getStarveCount();
    streamStats._ringBufferConsecutiveNotMixedCount = ringBuffer->getConsecutiveNotMixedCount();
    streamStats._ringBufferOverflowCount = ringBuffer->getOverflowCount();
    streamStats._ringBufferSilentFramesDropped = ringBuffer->getSilentFramesDropped();

    return streamStats;
}*/

void AudioMixerClientData::sendAudioStreamStatsPackets(const SharedNodePointer& destinationNode) {
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

    // pack and send stream stats packets until all ring buffers' stats are sent
    int numStreamStatsRemaining = _ringBuffers.size();
    QHash<QUuid, PositionalAudioRingBuffer*>::ConstIterator ringBuffersIterator = _ringBuffers.constBegin();
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
            AudioStreamStats streamStats = ringBuffersIterator.value()->updateSeqHistoryAndGetAudioStreamStats();
            memcpy(dataAt, &streamStats, sizeof(AudioStreamStats));
            dataAt += sizeof(AudioStreamStats);

            ringBuffersIterator++;
        }
        numStreamStatsRemaining -= numStreamStatsToPack;

        // send the current packet
        nodeList->writeDatagram(packet, dataAt - packet, destinationNode);
    }
}

QString AudioMixerClientData::getAudioStreamStatsString() const {
    QString result;
    AudioStreamStats streamStats = _downstreamAudioStreamStats;
    result += "DOWNSTREAM.desired:" + QString::number(streamStats._ringBufferDesiredJitterBufferFrames)
        + " available_avg_10s:" + QString::number(streamStats._ringBufferFramesAvailableAverage)
        + " available:" + QString::number(streamStats._ringBufferFramesAvailable)
        + " starves:" + QString::number(streamStats._ringBufferStarveCount)
        + " not_mixed:" + QString::number(streamStats._ringBufferConsecutiveNotMixedCount)
        + " overflows:" + QString::number(streamStats._ringBufferOverflowCount)
        + " silents_dropped: ?"
        + " lost%:" + QString::number(streamStats._packetStreamStats.getLostRate() * 100.0f, 'f', 2)
        + " lost%_30s:" + QString::number(streamStats._packetStreamWindowStats.getLostRate() * 100.0f, 'f', 2)
        + " min_gap:" + formatUsecTime(streamStats._timeGapMin)
        + " max_gap:" + formatUsecTime(streamStats._timeGapMax)
        + " avg_gap:" + formatUsecTime(streamStats._timeGapAverage)
        + " min_gap_30s:" + formatUsecTime(streamStats._timeGapWindowMin)
        + " max_gap_30s:" + formatUsecTime(streamStats._timeGapWindowMax)
        + " avg_gap_30s:" + formatUsecTime(streamStats._timeGapWindowAverage);

    AvatarAudioRingBuffer* avatarRingBuffer = getAvatarAudioRingBuffer();
    if (avatarRingBuffer) {
        AudioStreamStats streamStats = avatarRingBuffer->getAudioStreamStats();
        result += " UPSTREAM.mic.desired:" + QString::number(streamStats._ringBufferDesiredJitterBufferFrames)
            + " desired_calc:" + QString::number(avatarRingBuffer->getCalculatedDesiredJitterBufferFrames())
            + " available_avg_10s:" + QString::number(streamStats._ringBufferFramesAvailableAverage)
            + " available:" + QString::number(streamStats._ringBufferFramesAvailable)
            + " starves:" + QString::number(streamStats._ringBufferStarveCount)
            + " not_mixed:" + QString::number(streamStats._ringBufferConsecutiveNotMixedCount)
            + " overflows:" + QString::number(streamStats._ringBufferOverflowCount)
            + " silents_dropped:" + QString::number(streamStats._ringBufferSilentFramesDropped)
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
    
    QHash<QUuid, PositionalAudioRingBuffer*>::ConstIterator i, end = _ringBuffers.constEnd();
    for (i = _ringBuffers.constBegin(); i != end; i++) {
        if (i.value()->getType() == PositionalAudioRingBuffer::Injector) {
            AudioStreamStats streamStats = i.value()->getAudioStreamStats();
            result += " UPSTREAM.inj.desired:" + QString::number(streamStats._ringBufferDesiredJitterBufferFrames)
                + " desired_calc:" + QString::number(i.value()->getCalculatedDesiredJitterBufferFrames())
                + " available_avg_10s:" + QString::number(streamStats._ringBufferFramesAvailableAverage)
                + " available:" + QString::number(streamStats._ringBufferFramesAvailable)
                + " starves:" + QString::number(streamStats._ringBufferStarveCount)
                + " not_mixed:" + QString::number(streamStats._ringBufferConsecutiveNotMixedCount)
                + " overflows:" + QString::number(streamStats._ringBufferOverflowCount)
                + " silents_dropped:" + QString::number(streamStats._ringBufferSilentFramesDropped)
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
