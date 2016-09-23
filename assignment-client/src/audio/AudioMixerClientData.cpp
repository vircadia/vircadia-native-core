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

#include <random>

#include <QtCore/QDebug>
#include <QtCore/QJsonArray>

#include <udt/PacketHeaders.h>
#include <UUID.h>

#include "InjectedAudioStream.h"

#include "AudioMixer.h"
#include "AudioMixerClientData.h"


AudioMixerClientData::AudioMixerClientData(const QUuid& nodeID) :
    NodeData(nodeID),
    audioLimiter(AudioConstants::SAMPLE_RATE, AudioConstants::STEREO),
    _outgoingMixedAudioSequenceNumber(0),
    _downstreamAudioStreamStats()
{
    // of the ~94 blocks in a second of audio sent from the AudioMixer, pick a random one to send out a stats packet on
    // this ensures we send out stats to this client around every second
    // but do not send all of the stats packets out at the same time
    std::random_device randomDevice;
    std::mt19937 numberGenerator { randomDevice() };
    std::uniform_int_distribution<> distribution { 1, (int) ceil(1.0f / AudioConstants::NETWORK_FRAME_SECS) };

    _frameToSendStats = distribution(numberGenerator);
}

AudioMixerClientData::~AudioMixerClientData() {
    if (_codec) {
        _codec->releaseDecoder(_decoder);
        _codec->releaseEncoder(_encoder);
    }
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

void AudioMixerClientData::removeHRTFForStream(const QUuid& nodeID, const QUuid& streamID) {
    auto it = _nodeSourcesHRTFMap.find(nodeID);
    if (it != _nodeSourcesHRTFMap.end()) {
        // erase the stream with the given ID from the given node
        it->second.erase(streamID);

        // is the map for this node now empty?
        // if so we can remove it
        if (it->second.size() == 0) {
            _nodeSourcesHRTFMap.erase(it);
        }
    }
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

                auto avatarAudioStream = new AvatarAudioStream(isStereo, AudioMixer::getStreamSettings());
                avatarAudioStream->setupCodec(_codec, _selectedCodecName, AudioConstants::MONO);
                qDebug() << "creating new AvatarAudioStream... codec:" << _selectedCodecName;

                connect(avatarAudioStream, &InboundAudioStream::mismatchedAudioCodec, this, &AudioMixerClientData::handleMismatchAudioFormat);

                auto emplaced = _audioStreams.emplace(
                    QUuid(),
                    std::unique_ptr<PositionalAudioStream> { avatarAudioStream }
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
                auto injectorStream = new InjectedAudioStream(streamIdentifier, isStereo, AudioMixer::getStreamSettings());

#if INJECTORS_SUPPORT_CODECS
                injectorStream->setupCodec(_codec, _selectedCodecName, isStereo ? AudioConstants::STEREO : AudioConstants::MONO);
                qDebug() << "creating new injectorStream... codec:" << _selectedCodecName;
#endif

                auto emplaced = _audioStreams.emplace(
                    streamIdentifier,
                    std::unique_ptr<InjectedAudioStream> { injectorStream }
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
    QWriteLocker writeLocker { &_streamsLock };

    auto it = _audioStreams.begin();
    while (it != _audioStreams.end()) {
        SharedStreamPointer stream = it->second;

        if (stream->popFrames(1, true) > 0) {
            stream->updateLastPopOutputLoudnessAndTrailingLoudness();
        }

        static const int INJECTOR_MAX_INACTIVE_BLOCKS = 500;

        // if we don't have new data for an injected stream in the last INJECTOR_MAX_INACTIVE_BLOCKS then
        // we remove the injector from our streams
        if (stream->getType() == PositionalAudioStream::Injector
            && stream->getConsecutiveNotMixedCount() > INJECTOR_MAX_INACTIVE_BLOCKS) {
            // this is an inactive injector, pull it from our streams

            // first emit that it is finished so that the HRTF objects for this source can be cleaned up
            emit injectorStreamFinished(it->second->getStreamIdentifier());

            // erase the stream to drop our ref to the shared pointer and remove it
            it = _audioStreams.erase(it);
        } else {
            ++it;
        }
    }
}

bool AudioMixerClientData::shouldSendStats(int frameNumber) {
    return frameNumber == _frameToSendStats;
}

void AudioMixerClientData::sendAudioStreamStatsPackets(const SharedNodePointer& destinationNode) {

    auto nodeList = DependencyManager::get<NodeList>();

    // The append flag is a boolean value that will be packed right after the header.  The first packet sent
    // inside this method will have 0 for this flag, while every subsequent packet will have 1 for this flag.
    // The sole purpose of this flag is so the client can clear its map of injected audio stream stats when
    // it receives a packet with an appendFlag of 0. This prevents the buildup of dead audio stream stats in the client.
    quint8 appendFlag = 0;

    auto streamsCopy = getAudioStreams();

    // pack and send stream stats packets until all audio streams' stats are sent
    int numStreamStatsRemaining = int(streamsCopy.size());
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
    downstreamStats["unplayed"] = (double) streamStats._unplayedMs;
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
        upstreamStats["unplayed"] = (double) streamStats._unplayedMs;
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
            upstreamStats["unplayed"] = (double) streamStats._unplayedMs;
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

void AudioMixerClientData::handleMismatchAudioFormat(SharedNodePointer node, const QString& currentCodec, const QString& recievedCodec) {
    qDebug() << __FUNCTION__ << "sendingNode:" << *node << "currentCodec:" << currentCodec << "recievedCodec:" << recievedCodec;
    sendSelectAudioFormat(node, currentCodec);
}

void AudioMixerClientData::sendSelectAudioFormat(SharedNodePointer node, const QString& selectedCodecName) {
    auto replyPacket = NLPacket::create(PacketType::SelectedAudioFormat);
    replyPacket->writeString(selectedCodecName);
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->sendPacket(std::move(replyPacket), *node);
}


void AudioMixerClientData::setupCodec(CodecPluginPointer codec, const QString& codecName) {
    cleanupCodec(); // cleanup any previously allocated coders first
    _codec = codec;
    _selectedCodecName = codecName;
    if (codec) {
        _encoder = codec->createEncoder(AudioConstants::SAMPLE_RATE, AudioConstants::STEREO);
        _decoder = codec->createDecoder(AudioConstants::SAMPLE_RATE, AudioConstants::MONO);
    }

    auto avatarAudioStream = getAvatarAudioStream();
    if (avatarAudioStream) {
        avatarAudioStream->setupCodec(codec, codecName, AudioConstants::MONO);
    }

#if INJECTORS_SUPPORT_CODECS
    // fixup codecs for any active injectors...
    auto it = _audioStreams.begin();
    while (it != _audioStreams.end()) {
        SharedStreamPointer stream = it->second;
        if (stream->getType() == PositionalAudioStream::Injector) {
            stream->setupCodec(codec, codecName, stream->isStereo() ? AudioConstants::STEREO : AudioConstants::MONO);
        }
        ++it;
    }
#endif
}

void AudioMixerClientData::cleanupCodec() {
    // release any old codec encoder/decoder first...
    if (_codec) {
        if (_decoder) {
            _codec->releaseDecoder(_decoder);
            _decoder = nullptr;
        }
        if (_encoder) {
            _codec->releaseEncoder(_encoder);
            _encoder = nullptr;
        }
    }
}
