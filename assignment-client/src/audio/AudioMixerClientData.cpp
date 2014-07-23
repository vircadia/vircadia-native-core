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
#include "MovingMinMaxAvg.h"

const int INCOMING_SEQ_STATS_HISTORY_LENGTH = INCOMING_SEQ_STATS_HISTORY_LENGTH_SECONDS /
    (TOO_LONG_SINCE_LAST_SEND_AUDIO_STREAM_STATS / USECS_PER_SECOND);

AudioMixerClientData::AudioMixerClientData() :
    _ringBuffers(),
    _outgoingMixedAudioSequenceNumber(0),
    _incomingAvatarAudioSequenceNumberStats(INCOMING_SEQ_STATS_HISTORY_LENGTH)
{
    
}

AudioMixerClientData::~AudioMixerClientData() {
    for (int i = 0; i < _ringBuffers.size(); i++) {
        // delete this attached PositionalAudioRingBuffer
        delete _ringBuffers[i];
    }
}

AvatarAudioRingBuffer* AudioMixerClientData::getAvatarAudioRingBuffer() const {
    for (int i = 0; i < _ringBuffers.size(); i++) {
        if (_ringBuffers[i]->getType() == PositionalAudioRingBuffer::Microphone) {
            return (AvatarAudioRingBuffer*) _ringBuffers[i];
        }
    }

    // no AvatarAudioRingBuffer found - return NULL
    return NULL;
}

int AudioMixerClientData::parseData(const QByteArray& packet) {

    // parse sequence number for this packet
    int numBytesPacketHeader = numBytesForPacketHeader(packet);
    const char* sequenceAt = packet.constData() + numBytesPacketHeader;
    quint16 sequence = *(reinterpret_cast<const quint16*>(sequenceAt));

    PacketType packetType = packetTypeForPacket(packet);
    if (packetType == PacketTypeMicrophoneAudioWithEcho
        || packetType == PacketTypeMicrophoneAudioNoEcho
        || packetType == PacketTypeSilentAudioFrame) {

        SequenceNumberStats::ArrivalInfo packetArrivalInfo = _incomingAvatarAudioSequenceNumberStats.sequenceNumberReceived(sequence);

        // grab the AvatarAudioRingBuffer from the vector (or create it if it doesn't exist)
        AvatarAudioRingBuffer* avatarRingBuffer = getAvatarAudioRingBuffer();
        
        // read the first byte after the header to see if this is a stereo or mono buffer
        quint8 channelFlag = packet.at(numBytesForPacketHeader(packet) + sizeof(quint16));
        bool isStereo = channelFlag == 1;
        
        if (avatarRingBuffer && avatarRingBuffer->isStereo() != isStereo) {
            // there's a mismatch in the buffer channels for the incoming and current buffer
            // so delete our current buffer and create a new one
            _ringBuffers.removeOne(avatarRingBuffer);
            avatarRingBuffer->deleteLater();
            avatarRingBuffer = NULL;
        }

        if (!avatarRingBuffer) {
            // we don't have an AvatarAudioRingBuffer yet, so add it
            avatarRingBuffer = new AvatarAudioRingBuffer(isStereo, AudioMixer::getUseDynamicJitterBuffers());
            _ringBuffers.push_back(avatarRingBuffer);
        }


        // for now, late packets are simply discarded.  In the future, it may be good to insert them into their correct place
        // in the ring buffer (if that frame hasn't been mixed yet)
        switch (packetArrivalInfo._status) {
            case SequenceNumberStats::Early: {
                int packetsLost = packetArrivalInfo._seqDiffFromExpected;
                avatarRingBuffer->parseData(packet, packetsLost);
                break;
            }
            case SequenceNumberStats::OnTime: {
                // ask the AvatarAudioRingBuffer instance to parse the data
                avatarRingBuffer->parseData(packet, 0);
                break;
            }
            default: {
                break;
            }
        }
    } else if (packetType == PacketTypeInjectAudio) {
        // this is injected audio

        // grab the stream identifier for this injected audio
        QUuid streamIdentifier = QUuid::fromRfc4122(packet.mid(numBytesForPacketHeader(packet) + sizeof(quint16), NUM_BYTES_RFC4122_UUID));

        if (!_incomingInjectedAudioSequenceNumberStatsMap.contains(streamIdentifier)) {
            _incomingInjectedAudioSequenceNumberStatsMap.insert(streamIdentifier, SequenceNumberStats(INCOMING_SEQ_STATS_HISTORY_LENGTH));
        }
        SequenceNumberStats::ArrivalInfo packetArrivalInfo = 
            _incomingInjectedAudioSequenceNumberStatsMap[streamIdentifier].sequenceNumberReceived(sequence);

        InjectedAudioRingBuffer* matchingInjectedRingBuffer = NULL;

        for (int i = 0; i < _ringBuffers.size(); i++) {
            if (_ringBuffers[i]->getType() == PositionalAudioRingBuffer::Injector
                && ((InjectedAudioRingBuffer*) _ringBuffers[i])->getStreamIdentifier() == streamIdentifier) {
                matchingInjectedRingBuffer = (InjectedAudioRingBuffer*) _ringBuffers[i];
            }
        }

        if (!matchingInjectedRingBuffer) {
            // we don't have a matching injected audio ring buffer, so add it
            matchingInjectedRingBuffer = new InjectedAudioRingBuffer(streamIdentifier, AudioMixer::getUseDynamicJitterBuffers());
            _ringBuffers.push_back(matchingInjectedRingBuffer);
        }

        // for now, late packets are simply discarded.  In the future, it may be good to insert them into their correct place
        // in the ring buffer (if that frame hasn't been mixed yet)
        switch (packetArrivalInfo._status) {
            case SequenceNumberStats::Early: {
                int packetsLost = packetArrivalInfo._seqDiffFromExpected;
                matchingInjectedRingBuffer->parseData(packet, packetsLost);
                break;
            }
            case SequenceNumberStats::OnTime: {
                // ask the AvatarAudioRingBuffer instance to parse the data
                matchingInjectedRingBuffer->parseData(packet, 0);
                break;
            }
            default: {
                break;
            }
        }
    } else if (packetType == PacketTypeAudioStreamStats) {

        const char* dataAt = packet.data();

        // skip over header, appendFlag, and num stats packed
        dataAt += (numBytesPacketHeader + sizeof(quint8) + sizeof(quint16));

        // read the downstream audio stream stats
        memcpy(&_downstreamAudioStreamStats, dataAt, sizeof(AudioStreamStats));
    }

    return 0;
}

void AudioMixerClientData::checkBuffersBeforeFrameSend(AABox* checkSourceZone, AABox* listenerZone) {
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
}

AudioStreamStats AudioMixerClientData::getAudioStreamStatsOfStream(const PositionalAudioRingBuffer* ringBuffer) const {
    
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
}

void AudioMixerClientData::sendAudioStreamStatsPackets(const SharedNodePointer& destinationNode) {
    
    // have all the seq number stats of each audio stream push their current stats into their history,
    // which moves that history window 1 second forward (since that's how long since the last stats were pushed into history)
    _incomingAvatarAudioSequenceNumberStats.pushStatsToHistory();
    QHash<QUuid, SequenceNumberStats>::Iterator i = _incomingInjectedAudioSequenceNumberStatsMap.begin();
    QHash<QUuid, SequenceNumberStats>::Iterator end = _incomingInjectedAudioSequenceNumberStatsMap.end();
    while (i != end) {
        i.value().pushStatsToHistory();
        i++;
    }

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
    QList<PositionalAudioRingBuffer*>::ConstIterator ringBuffersIterator = _ringBuffers.constBegin();
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
            AudioStreamStats streamStats = getAudioStreamStatsOfStream(*ringBuffersIterator);
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
        AudioStreamStats streamStats = getAudioStreamStatsOfStream(avatarRingBuffer);
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
    
    for (int i = 0; i < _ringBuffers.size(); i++) {
        if (_ringBuffers[i]->getType() == PositionalAudioRingBuffer::Injector) {
            AudioStreamStats streamStats = getAudioStreamStatsOfStream(_ringBuffers[i]);
            result += " UPSTREAM.inj.desired:" + QString::number(streamStats._ringBufferDesiredJitterBufferFrames)
                + " desired_calc:" + QString::number(_ringBuffers[i]->getCalculatedDesiredJitterBufferFrames())
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
