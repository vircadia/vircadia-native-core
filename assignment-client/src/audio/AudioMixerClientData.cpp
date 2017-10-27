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

#include "AudioHelpers.h"
#include "AudioMixer.h"
#include "AudioMixerClientData.h"


AudioMixerClientData::AudioMixerClientData(const QUuid& nodeID) :
    NodeData(nodeID),
    audioLimiter(AudioConstants::SAMPLE_RATE, AudioConstants::STEREO),
    _ignoreZone(*this),
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

void AudioMixerClientData::queuePacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer node) {
    if (!_packetQueue.node) {
        _packetQueue.node = node;
    }
    _packetQueue.push(message);
}

void AudioMixerClientData::processPackets() {
    SharedNodePointer node = _packetQueue.node;
    assert(_packetQueue.empty() || node);
    _packetQueue.node.clear();

    while (!_packetQueue.empty()) {
        auto& packet = _packetQueue.front();

        switch (packet->getType()) {
            case PacketType::MicrophoneAudioNoEcho:
            case PacketType::MicrophoneAudioWithEcho:
            case PacketType::InjectAudio:
            case PacketType::SilentAudioFrame: {

                if (node->isUpstream()) {
                    setupCodecForReplicatedAgent(packet);
                }

                QMutexLocker lock(&getMutex());
                parseData(*packet);

                optionallyReplicatePacket(*packet, *node);

                break;
            }
            case PacketType::AudioStreamStats: {
                QMutexLocker lock(&getMutex());
                parseData(*packet);

                break;
            }
            case PacketType::NegotiateAudioFormat:
                negotiateAudioFormat(*packet, node);
                break;
            case PacketType::RequestsDomainListData:
                parseRequestsDomainListData(*packet);
                break;
            case PacketType::PerAvatarGainSet:
                parsePerAvatarGainSet(*packet, node);
                break;
            case PacketType::NodeIgnoreRequest:
                parseNodeIgnoreRequest(packet, node);
                break;
            case PacketType::RadiusIgnoreRequest:
                parseRadiusIgnoreRequest(packet, node);
                break;
            default:
                Q_UNREACHABLE();
        }

        _packetQueue.pop();
    }
    assert(_packetQueue.empty());
}

bool isReplicatedPacket(PacketType packetType) {
    return packetType == PacketType::ReplicatedMicrophoneAudioNoEcho
        || packetType == PacketType::ReplicatedMicrophoneAudioWithEcho
        || packetType == PacketType::ReplicatedInjectAudio
        || packetType == PacketType::ReplicatedSilentAudioFrame;
}

void AudioMixerClientData::optionallyReplicatePacket(ReceivedMessage& message, const Node& node) {

    // first, make sure that this is a packet from a node we are supposed to replicate
    if (node.isReplicated()) {

        // now make sure it's a packet type that we want to replicate

        // first check if it is an original type that we should replicate
        PacketType mirroredType = PacketTypeEnum::getReplicatedPacketMapping().value(message.getType());

        if (mirroredType == PacketType::Unknown) {
            // if it wasn't check if it is a replicated type that we should re-replicate
            if (PacketTypeEnum::getReplicatedPacketMapping().key(message.getType()) != PacketType::Unknown) {
                mirroredType = message.getType();
            } else {
                qDebug() << "Packet passed to optionallyReplicatePacket was not a replicatable type - returning";
                return;
            }
        }

        std::unique_ptr<NLPacket> packet;
        auto nodeList = DependencyManager::get<NodeList>();

        // enumerate the downstream audio mixers and send them the replicated version of this packet
        nodeList->unsafeEachNode([&](const SharedNodePointer& downstreamNode) {
            if (AudioMixer::shouldReplicateTo(node, *downstreamNode)) {
                // construct the packet only once, if we have any downstream audio mixers to send to
                if (!packet) {
                    // construct an NLPacket to send to the replicant that has the contents of the received packet
                    packet = NLPacket::create(mirroredType);

                    if (!isReplicatedPacket(message.getType())) {
                        // since this packet will be non-sourced, we add the replicated node's ID here
                        packet->write(node.getUUID().toRfc4122());
                    }

                    packet->write(message.getMessage());
                }
                
                nodeList->sendUnreliablePacket(*packet, *downstreamNode);
            }
        });
    }

}

void AudioMixerClientData::negotiateAudioFormat(ReceivedMessage& message, const SharedNodePointer& node) {
    quint8 numberOfCodecs;
    message.readPrimitive(&numberOfCodecs);
    std::vector<QString> codecs;
    for (auto i = 0; i < numberOfCodecs; i++) {
        codecs.push_back(message.readString());
    }
    const std::pair<QString, CodecPluginPointer> codec = AudioMixer::negotiateCodec(codecs);

    setupCodec(codec.second, codec.first);
    sendSelectAudioFormat(node, codec.first);
}

void AudioMixerClientData::parseRequestsDomainListData(ReceivedMessage& message) {
    bool isRequesting;
    message.readPrimitive(&isRequesting);
    setRequestsDomainListData(isRequesting);
}

void AudioMixerClientData::parsePerAvatarGainSet(ReceivedMessage& message, const SharedNodePointer& node) {
    QUuid uuid = node->getUUID();
    // parse the UUID from the packet
    QUuid avatarUuid = QUuid::fromRfc4122(message.readWithoutCopy(NUM_BYTES_RFC4122_UUID));
    uint8_t packedGain;
    message.readPrimitive(&packedGain);
    float gain = unpackFloatGainFromByte(packedGain);
    hrtfForStream(avatarUuid, QUuid()).setGainAdjustment(gain);
    qDebug() << "Setting gain adjustment for hrtf[" << uuid << "][" << avatarUuid << "] to " << gain;
}

void AudioMixerClientData::parseNodeIgnoreRequest(QSharedPointer<ReceivedMessage> message, const SharedNodePointer& node) {
    node->parseIgnoreRequestMessage(message);
}

void AudioMixerClientData::parseRadiusIgnoreRequest(QSharedPointer<ReceivedMessage> message, const SharedNodePointer& node) {
    node->parseIgnoreRadiusRequestMessage(message);
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

void AudioMixerClientData::removeAgentAvatarAudioStream() {
    QWriteLocker writeLocker { &_streamsLock };
    auto it = _audioStreams.find(QUuid());
    if (it != _audioStreams.end()) {
        _audioStreams.erase(it);
    }
    writeLocker.unlock();
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
            || packetType == PacketType::ReplicatedMicrophoneAudioWithEcho
            || packetType == PacketType::MicrophoneAudioNoEcho
            || packetType == PacketType::ReplicatedMicrophoneAudioNoEcho
            || packetType == PacketType::SilentAudioFrame
            || packetType == PacketType::ReplicatedSilentAudioFrame) {

            QWriteLocker writeLocker { &_streamsLock };

            auto micStreamIt = _audioStreams.find(QUuid());
            if (micStreamIt == _audioStreams.end()) {
                // we don't have a mic stream yet, so add it

                // read the channel flag to see if our stream is stereo or not
                message.seek(sizeof(quint16));

                quint8 channelFlag;
                message.readPrimitive(&channelFlag);

                bool isStereo = channelFlag == 1;

                auto avatarAudioStream = new AvatarAudioStream(isStereo, AudioMixer::getStaticJitterFrames());
                avatarAudioStream->setupCodec(_codec, _selectedCodecName, AudioConstants::MONO);
                qDebug() << "creating new AvatarAudioStream... codec:" << _selectedCodecName;

                connect(avatarAudioStream, &InboundAudioStream::mismatchedAudioCodec,
                        this, &AudioMixerClientData::handleMismatchAudioFormat);

                auto emplaced = _audioStreams.emplace(
                    QUuid(),
                    std::unique_ptr<PositionalAudioStream> { avatarAudioStream }
                );

                micStreamIt = emplaced.first;
            }

            matchingStream = micStreamIt->second;

            writeLocker.unlock();

            isMicStream = true;
        } else if (packetType == PacketType::InjectAudio
                   || packetType == PacketType::ReplicatedInjectAudio) {
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
                auto injectorStream = new InjectedAudioStream(streamIdentifier, isStereo, AudioMixer::getStaticJitterFrames());

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

int AudioMixerClientData::checkBuffersBeforeFrameSend() {
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

    return (int)_audioStreams.size();
}

bool AudioMixerClientData::shouldSendStats(int frameNumber) {
    return frameNumber == _frameToSendStats;
}

void AudioMixerClientData::sendAudioStreamStatsPackets(const SharedNodePointer& destinationNode) {

    auto nodeList = DependencyManager::get<NodeList>();

    // The append flag is a boolean value that will be packed right after the header.
    // This flag allows the client to know when it has received all stats packets, so it can group any downstream effects,
    // and clear its cache of injector stream stats; it helps to prevent buildup of dead audio stream stats in the client.
    quint8 appendFlag = AudioStreamStats::START;

    auto streamsCopy = getAudioStreams();

    // pack and send stream stats packets until all audio streams' stats are sent
    int numStreamStatsRemaining = int(streamsCopy.size());
    auto it = streamsCopy.cbegin();

    while (numStreamStatsRemaining > 0) {
        auto statsPacket = NLPacket::create(PacketType::AudioStreamStats);

        int numStreamStatsRoomFor = (int)(statsPacket->size() - sizeof(quint8) - sizeof(quint16)) / sizeof(AudioStreamStats);

        // calculate the number of stream stats to follow
        quint16 numStreamStatsToPack = std::min(numStreamStatsRemaining, numStreamStatsRoomFor);

        // is this the terminal packet?
        if (numStreamStatsRemaining <= numStreamStatsToPack) {
            appendFlag |= AudioStreamStats::END;
        }

        // pack the append flag in this packet
        statsPacket->writePrimitive(appendFlag);
        appendFlag = 0;

        // pack the number of stream stats to follow
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
    sendSelectAudioFormat(node, currentCodec);
}

void AudioMixerClientData::sendSelectAudioFormat(SharedNodePointer node, const QString& selectedCodecName) {
    auto replyPacket = NLPacket::create(PacketType::SelectedAudioFormat);
    replyPacket->writeString(selectedCodecName);
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->sendPacket(std::move(replyPacket), *node);
}

void AudioMixerClientData::encodeFrameOfZeros(QByteArray& encodedZeros) {
    static QByteArray zeros(AudioConstants::NETWORK_FRAME_BYTES_STEREO, 0);
    if (_shouldFlushEncoder) {
        if (_encoder) {
            _encoder->encode(zeros, encodedZeros);
        } else {
            encodedZeros = zeros;
        }
    }
    _shouldFlushEncoder = false;
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

AudioMixerClientData::IgnoreZone& AudioMixerClientData::IgnoreZoneMemo::get(unsigned int frame) {
    // check for a memoized zone
    if (frame != _frame.load(std::memory_order_acquire)) {
        AvatarAudioStream* stream = _data.getAvatarAudioStream();

        // get the initial dimensions from the stream
        glm::vec3 corner = stream ? stream->getAvatarBoundingBoxCorner() : glm::vec3(0);
        glm::vec3 scale = stream ? stream->getAvatarBoundingBoxScale() : glm::vec3(0);

        // enforce a minimum scale
        static const glm::vec3 MIN_IGNORE_BOX_SCALE = glm::vec3(0.3f, 1.3f, 0.3f);
        if (glm::any(glm::lessThan(scale, MIN_IGNORE_BOX_SCALE))) {
            scale = MIN_IGNORE_BOX_SCALE;
        }

        // quadruple the scale (this is arbitrary number chosen for comfort)
        const float IGNORE_BOX_SCALE_FACTOR = 4.0f;
        scale *= IGNORE_BOX_SCALE_FACTOR;

        // create the box (we use a box for the zone for convenience)
        AABox box(corner, scale);

        // update the memoized zone
        // This may be called by multiple threads concurrently,
        // so take a lock and only update the memo if this call is first.
        // This prevents concurrent updates from invalidating the returned reference
        // (contingent on the preconditions listed in the header).
        std::lock_guard<std::mutex> lock(_mutex);
        if (frame != _frame.load(std::memory_order_acquire)) {
            _zone = box;
            unsigned int oldFrame = _frame.exchange(frame, std::memory_order_release);
            Q_UNUSED(oldFrame);
        }
    }

    return _zone;
}

void AudioMixerClientData::IgnoreNodeCache::cache(bool shouldIgnore) {
    if (!_isCached) {
        _shouldIgnore = shouldIgnore;
        _isCached = true;
    }
}

bool AudioMixerClientData::IgnoreNodeCache::isCached() {
    return _isCached;
}

bool AudioMixerClientData::IgnoreNodeCache::shouldIgnore() {
    bool ignore = _shouldIgnore;
    _isCached = false;
    return ignore;
}

bool AudioMixerClientData::shouldIgnore(const SharedNodePointer self, const SharedNodePointer node, unsigned int frame) {
    // this is symmetric over self / node; if computed, it is cached in the other

    // check the cache to avoid computation
    auto& cache = _nodeSourcesIgnoreMap[node->getUUID()];
    if (cache.isCached()) {
        return cache.shouldIgnore();
    }

    AudioMixerClientData* nodeData = static_cast<AudioMixerClientData*>(node->getLinkedData());
    if (!nodeData) {
        return false;
    }

    // compute shouldIgnore
    bool shouldIgnore = true;
    if ( // the nodes are not ignoring each other explicitly (or are but get data regardless)
            (!self->isIgnoringNodeWithID(node->getUUID()) ||
             (nodeData->getRequestsDomainListData() && node->getCanKick())) &&
            (!node->isIgnoringNodeWithID(self->getUUID()) ||
             (getRequestsDomainListData() && self->getCanKick())))  {

        // if either node is enabling an ignore radius, check their proximity
        if ((self->isIgnoreRadiusEnabled() || node->isIgnoreRadiusEnabled())) {
            auto& zone = _ignoreZone.get(frame);
            auto& nodeZone = nodeData->_ignoreZone.get(frame);
            shouldIgnore = zone.touches(nodeZone);
        } else {
            shouldIgnore = false;
        }
    }

    // cache in node
    nodeData->_nodeSourcesIgnoreMap[self->getUUID()].cache(shouldIgnore);

    return shouldIgnore;
}

void AudioMixerClientData::setupCodecForReplicatedAgent(QSharedPointer<ReceivedMessage> message) {
    // hop past the sequence number that leads the packet
    message->seek(sizeof(quint16));

    // pull the codec string from the packet
    auto codecString = message->readString();

    if (codecString != _selectedCodecName) {
        qDebug() << "Manually setting codec for replicated agent" << uuidStringWithoutCurlyBraces(getNodeID())
        << "-" << codecString;

        const std::pair<QString, CodecPluginPointer> codec = AudioMixer::negotiateCodec({ codecString });
        setupCodec(codec.second, codec.first);

        // seek back to the beginning of the message so other readers are in the right place
        message->seek(0);
    }
}
