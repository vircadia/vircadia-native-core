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

#include "AudioMixerClientData.h"

#include <random>

#include <glm/common.hpp>

#include <QtCore/QDebug>
#include <QtCore/QJsonArray>

#include <udt/PacketHeaders.h>
#include <UUID.h>

#include "InjectedAudioStream.h"

#include "AudioLogging.h"
#include "AudioHelpers.h"
#include "AudioMixer.h"

AudioMixerClientData::AudioMixerClientData(const QUuid& nodeID, Node::LocalID nodeLocalID) :
    NodeData(nodeID, nodeLocalID),
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

void AudioMixerClientData::queuePacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer node) {
    if (!_packetQueue.node) {
        _packetQueue.node = node;
    }
    _packetQueue.push(message);
}

int AudioMixerClientData::processPackets(ConcurrentAddedStreams& addedStreams) {
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

                processStreamPacket(*packet, addedStreams);

                optionallyReplicatePacket(*packet, *node);
                break;
            }
            case PacketType::AudioStreamStats: {
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
            case PacketType::InjectorGainSet:
                parseInjectorGainSet(*packet, node);
                break;
            case PacketType::NodeIgnoreRequest:
                parseNodeIgnoreRequest(packet, node);
                break;
            case PacketType::RadiusIgnoreRequest:
                parseRadiusIgnoreRequest(packet, node);
                break;
            case PacketType::AudioSoloRequest:
                parseSoloRequest(packet, node);
                break;
            case PacketType::StopInjector:
                parseStopInjectorPacket(packet);
                break;
            default:
                Q_UNREACHABLE();
        }

        _packetQueue.pop();
    }
    assert(_packetQueue.empty());

    // now that we have processed all packets for this frame
    // we can prepare the sources from this client to be ready for mixing
    return checkBuffersBeforeFrameSend();
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
                qCDebug(audio) << "Packet passed to optionallyReplicatePacket was not a replicatable type - returning";
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
    QUuid avatarUUID = QUuid::fromRfc4122(message.readWithoutCopy(NUM_BYTES_RFC4122_UUID));
    uint8_t packedGain;
    message.readPrimitive(&packedGain);
    float gain = unpackFloatGainFromByte(packedGain);

    if (avatarUUID.isNull()) {
        // set the MASTER avatar gain
        setMasterAvatarGain(gain);
        qCDebug(audio) << "Setting MASTER avatar gain for" << uuid << "to" << gain;
    } else {
        // set the per-source avatar gain
        setGainForAvatar(avatarUUID, gain);
        qCDebug(audio) << "Setting avatar gain adjustment for hrtf[" << uuid << "][" << avatarUUID << "] to" << gain;
    }
}

void AudioMixerClientData::parseInjectorGainSet(ReceivedMessage& message, const SharedNodePointer& node) {
    QUuid uuid = node->getUUID();

    uint8_t packedGain;
    message.readPrimitive(&packedGain);
    float gain = unpackFloatGainFromByte(packedGain);

    setMasterInjectorGain(gain);
    qCDebug(audio) << "Setting MASTER injector gain for" << uuid << "to" << gain;
}

void AudioMixerClientData::setGainForAvatar(QUuid nodeID, float gain) {
    auto it = std::find_if(_streams.active.cbegin(), _streams.active.cend(), [nodeID](const MixableStream& mixableStream){
        return mixableStream.nodeStreamID.nodeID == nodeID && mixableStream.nodeStreamID.streamID.isNull();
    });

    if (it != _streams.active.cend()) {
        it->hrtf->setGainAdjustment(gain);
    }
}

void AudioMixerClientData::parseNodeIgnoreRequest(QSharedPointer<ReceivedMessage> message, const SharedNodePointer& node) {
    auto ignoredNodesPair = node->parseIgnoreRequestMessage(message);

    // we have a vector of ignored or unignored node UUIDs - update our internal data structures so that
    // streams can be included or excluded next time a mix is being created
    if (ignoredNodesPair.second) {
        // we have newly ignored nodes, add them to our vector
        _newIgnoredNodeIDs.insert(std::end(_newIgnoredNodeIDs),
                                  std::begin(ignoredNodesPair.first), std::end(ignoredNodesPair.first));
    } else {
        // we have newly unignored nodes, add them to our vector
        _newUnignoredNodeIDs.insert(std::end(_newUnignoredNodeIDs),
                                    std::begin(ignoredNodesPair.first), std::end(ignoredNodesPair.first));
    }

    auto nodeList = DependencyManager::get<NodeList>();
    for (auto& nodeID : ignoredNodesPair.first) {
        auto otherNode = nodeList->nodeWithUUID(nodeID);
        if (otherNode) {
            auto otherNodeMixerClientData = static_cast<AudioMixerClientData*>(otherNode->getLinkedData());
            if (otherNodeMixerClientData) {
                if (ignoredNodesPair.second) {
                    otherNodeMixerClientData->ignoredByNode(getNodeID());
                } else {
                    otherNodeMixerClientData->unignoredByNode(getNodeID());
                }
            }
        }
    }
}

void AudioMixerClientData::ignoredByNode(QUuid nodeID) {
    // first add this ID to the concurrent vector for newly ignoring nodes
    _newIgnoringNodeIDs.push_back(nodeID);

    // now take a lock and on the consistent vector of ignoring nodes and make sure this node is in it
    std::lock_guard<std::mutex> lock(_ignoringNodeIDsMutex);
    if (std::find(_ignoringNodeIDs.begin(), _ignoringNodeIDs.end(), nodeID) == _ignoringNodeIDs.end()) {
        _ignoringNodeIDs.push_back(nodeID);
    }
}

void AudioMixerClientData::unignoredByNode(QUuid nodeID) {
    // first add this ID to the concurrent vector for newly unignoring nodes
    _newUnignoringNodeIDs.push_back(nodeID);

    // now take a lock on the consistent vector of ignoring nodes and make sure this node isn't in it
    std::lock_guard<std::mutex> lock(_ignoringNodeIDsMutex);
    auto it = _ignoringNodeIDs.begin();
    while (it != _ignoringNodeIDs.end()) {
        if (*it == nodeID) {
            it = _ignoringNodeIDs.erase(it);
        } else {
            ++it;
        }
    }
}

void AudioMixerClientData::clearStagedIgnoreChanges() {
    _newIgnoredNodeIDs.clear();
    _newUnignoredNodeIDs.clear();
    _newIgnoringNodeIDs.clear();
    _newUnignoringNodeIDs.clear();
}

void AudioMixerClientData::parseRadiusIgnoreRequest(QSharedPointer<ReceivedMessage> message, const SharedNodePointer& node) {
    bool enabled;
    message->readPrimitive(&enabled);

    _isIgnoreRadiusEnabled = enabled;

    auto avatarAudioStream = getAvatarAudioStream();

    // if we have an avatar audio stream, tell it wether its ignore box should be enabled or disabled
    if (avatarAudioStream) {
        if (_isIgnoreRadiusEnabled) {
            avatarAudioStream->enableIgnoreBox();
        } else {
            avatarAudioStream->disableIgnoreBox();
        }
    }
}


void AudioMixerClientData::parseSoloRequest(QSharedPointer<ReceivedMessage> message, const SharedNodePointer& node) {

    uint8_t addToSolo;
    message->readPrimitive(&addToSolo);

    while (message->getBytesLeftToRead()) {
        // parse out the UUID being soloed from the packet
        QUuid soloedUUID = QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID));

        if (addToSolo) {
            _soloedNodes.push_back(soloedUUID);
        } else {
            auto it = std::remove(std::begin(_soloedNodes), std::end(_soloedNodes), soloedUUID);
            _soloedNodes.erase(it, std::end(_soloedNodes));
        }
    }
}

AvatarAudioStream* AudioMixerClientData::getAvatarAudioStream() {
    auto it = std::find_if(_audioStreams.begin(), _audioStreams.end(), [](const SharedStreamPointer& stream){
        return stream->getStreamIdentifier().isNull();
    });

    if (it != _audioStreams.end()) {
        return dynamic_cast<AvatarAudioStream*>(it->get());
    }

    // no mic stream found - return NULL
    return NULL;
}

void AudioMixerClientData::removeAgentAvatarAudioStream() {
    auto it = std::remove_if(_audioStreams.begin(), _audioStreams.end(), [](const SharedStreamPointer& stream){
        return stream->getStreamIdentifier().isNull();
    });

    if (it != _audioStreams.end()) {
        _audioStreams.erase(it);

        // Clear mixing structures so that they get recreated with up to date
        // data if the stream comes back
        setHasReceivedFirstMix(false);
        _streams.skipped.clear();
        _streams.inactive.clear();
        _streams.active.clear();
    }
}

int AudioMixerClientData::parseData(ReceivedMessage& message) {
    PacketType packetType = message.getType();

    if (packetType == PacketType::AudioStreamStats) {
        // skip over header, appendFlag, and num stats packed
        message.seek(sizeof(quint8) + sizeof(quint16));

        if (message.getBytesLeftToRead() != sizeof(AudioStreamStats)) {
            qWarning() << "Received AudioStreamStats of wrong size" << message.getBytesLeftToRead()
                << "instead of" << sizeof(AudioStreamStats) << "from"
                << message.getSourceID() << "at" << message.getSenderSockAddr();
            
            return message.getPosition();
        }

        // read the downstream audio stream stats
        message.readPrimitive(&_downstreamAudioStreamStats);

        return message.getPosition();
    }

    return 0;
}

bool AudioMixerClientData::containsValidPosition(ReceivedMessage& message) const {
    static const int SEQUENCE_NUMBER_BYTES = sizeof(quint16);

    auto posBefore = message.getPosition();

    message.seek(SEQUENCE_NUMBER_BYTES);

    // skip over the codec string
    message.readString();

    switch (message.getType()) {
        case PacketType::MicrophoneAudioNoEcho:
        case PacketType::MicrophoneAudioWithEcho: {
            // skip over the stereo flag
            message.seek(message.getPosition() + sizeof(ChannelFlag));
            break;
        }
        case PacketType::SilentAudioFrame: {
            // skip the number of silent samples
            message.seek(message.getPosition() + sizeof(SilentSamplesBytes));
            break;
        }
        case PacketType::InjectAudio: {
            // skip the stream ID, stereo flag, and loopback flag
            message.seek(message.getPosition() + NUM_STREAM_ID_BYTES + sizeof(ChannelFlag) + sizeof(LoopbackFlag));
            break;
        }
        default:
            Q_UNREACHABLE();
            break;
    }

    glm::vec3 peekPosition;
    message.readPrimitive(&peekPosition);

    // reset the position the message was at before we were called
    message.seek(posBefore);

    if (glm::any(glm::isnan(peekPosition))) {
        return false;
    }

    return true;
}

void AudioMixerClientData::processStreamPacket(ReceivedMessage& message, ConcurrentAddedStreams &addedStreams) {

    if (!containsValidPosition(message)) {
        qDebug() << "Refusing to process audio stream from" << message.getSourceID() << "with invalid position";
        return;
    }

    SharedStreamPointer matchingStream;

    auto packetType = message.getType();
    bool newStream = false;

    if (packetType == PacketType::MicrophoneAudioWithEcho
        || packetType == PacketType::MicrophoneAudioNoEcho
        || packetType == PacketType::SilentAudioFrame) {

        auto micStreamIt = std::find_if(_audioStreams.begin(), _audioStreams.end(), [](const SharedStreamPointer& stream){
            return stream->getStreamIdentifier().isNull();
        });

        if (micStreamIt == _audioStreams.end()) {
            // we don't have a mic stream yet, so add it

            // hop past the sequence number that leads the packet
            message.seek(sizeof(StreamSequenceNumber));

            // pull the codec string from the packet
            auto codecString = message.readString();

            // determine if the stream is stereo or not
            bool isStereo;
            if (packetType == PacketType::SilentAudioFrame || packetType == PacketType::ReplicatedSilentAudioFrame) {
                SilentSamplesBytes numSilentSamples;
                message.readPrimitive(&numSilentSamples);
                isStereo = numSilentSamples == AudioConstants::NETWORK_FRAME_SAMPLES_STEREO;
            } else {
                ChannelFlag channelFlag;
                message.readPrimitive(&channelFlag);
                isStereo = channelFlag == 1;
            }

            auto avatarAudioStream = new AvatarAudioStream(isStereo, AudioMixer::getStaticJitterFrames());
            avatarAudioStream->setupCodec(_codec, _selectedCodecName, isStereo ? AudioConstants::STEREO : AudioConstants::MONO);

            if (_isIgnoreRadiusEnabled) {
                avatarAudioStream->enableIgnoreBox();
            } else {
                avatarAudioStream->disableIgnoreBox();
            }

            qCDebug(audio) << "creating new AvatarAudioStream... codec:" << _selectedCodecName << "isStereo:" << isStereo;

            connect(avatarAudioStream, &InboundAudioStream::mismatchedAudioCodec,
                    this, &AudioMixerClientData::handleMismatchAudioFormat);

            matchingStream = SharedStreamPointer(avatarAudioStream);
            _audioStreams.push_back(matchingStream);

            newStream = true;
        } else {
            matchingStream = *micStreamIt;
        }
    } else if (packetType == PacketType::InjectAudio) {

        // this is injected audio
        // skip the sequence number and codec string and grab the stream identifier for this injected audio
        message.seek(sizeof(StreamSequenceNumber));
        message.readString();

        QUuid streamIdentifier = QUuid::fromRfc4122(message.readWithoutCopy(NUM_BYTES_RFC4122_UUID));

        auto streamIt = std::find_if(_audioStreams.begin(), _audioStreams.end(), [&streamIdentifier](const SharedStreamPointer& stream) {
            return stream->getStreamIdentifier() == streamIdentifier;
        });

        if (streamIt == _audioStreams.end()) {
            bool isStereo;
            message.readPrimitive(&isStereo);

            // we don't have this injected stream yet, so add it
            auto injectorStream = new InjectedAudioStream(streamIdentifier, isStereo, AudioMixer::getStaticJitterFrames());

#if INJECTORS_SUPPORT_CODECS
            injectorStream->setupCodec(_codec, _selectedCodecName, isStereo ? AudioConstants::STEREO : AudioConstants::MONO);
            qCDebug(audio) << "creating new injectorStream... codec:" << _selectedCodecName << "isStereo:" << isStereo;
#endif

            matchingStream = SharedStreamPointer(injectorStream);
            _audioStreams.push_back(matchingStream);

            newStream = true;
        } else {
            matchingStream = *streamIt;
        }
    }

    // seek to the beginning of the packet so that the next reader is in the right spot
    message.seek(0);

    // check the overflow count before we parse data
    auto overflowBefore = matchingStream->getOverflowCount();
    matchingStream->parseData(message);

    if (matchingStream->getOverflowCount() > overflowBefore) {
        qCDebug(audio) << "Just overflowed on stream" << matchingStream->getStreamIdentifier()
            << "from" << message.getSourceID();
    }

    if (newStream) {
        // whenever a stream is added, push it to the concurrent vector of streams added this frame
        addedStreams.push_back(AddedStream(getNodeID(), getNodeLocalID(), matchingStream->getStreamIdentifier(), matchingStream.get()));
    }
}

int AudioMixerClientData::checkBuffersBeforeFrameSend() {
    auto it = _audioStreams.begin();
    while (it != _audioStreams.end()) {
        SharedStreamPointer stream = *it;

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
            emit injectorStreamFinished(stream->getStreamIdentifier());

            // erase the stream to drop our ref to the shared pointer and remove it
            it = _audioStreams.erase(it);
        } else {
            ++it;
        }
    }

    return (int)_audioStreams.size();
}

void AudioMixerClientData::parseStopInjectorPacket(QSharedPointer<ReceivedMessage> packet) {
    auto streamID = QUuid::fromRfc4122(packet->readWithoutCopy(NUM_BYTES_RFC4122_UUID));

    auto it = std::find_if(std::begin(_audioStreams), std::end(_audioStreams), [&](auto stream) {
        return streamID == stream->getStreamIdentifier();
    });

    if (it != std::end(_audioStreams)) {
        _audioStreams.erase(it);
        emit injectorStreamFinished(streamID);
    }
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
            PositionalAudioStream* stream = it->get();

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
    downstreamStats["min_gap_usecs"] = static_cast<double>(streamStats._timeGapMin);
    downstreamStats["max_gap_usecs"] = static_cast<double>(streamStats._timeGapMax);
    downstreamStats["avg_gap_usecs"] = static_cast<double>(streamStats._timeGapAverage);
    downstreamStats["min_gap_30s_usecs"] = static_cast<double>(streamStats._timeGapWindowMin);
    downstreamStats["max_gap_30s_usecs"] = static_cast<double>(streamStats._timeGapWindowMax);
    downstreamStats["avg_gap_30s_usecs"] = static_cast<double>(streamStats._timeGapWindowAverage);

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

        upstreamStats["min_gap_usecs"] = static_cast<double>(streamStats._timeGapMin);
        upstreamStats["max_gap_usecs"] = static_cast<double>(streamStats._timeGapMax);
        upstreamStats["avg_gap_usecs"] = static_cast<double>(streamStats._timeGapAverage);
        upstreamStats["min_gap_30s_usecs"] = static_cast<double>(streamStats._timeGapWindowMin);
        upstreamStats["max_gap_30s_usecs"] = static_cast<double>(streamStats._timeGapWindowMax);
        upstreamStats["avg_gap_30s_usecs"] = static_cast<double>(streamStats._timeGapWindowAverage);

        result["upstream"] = upstreamStats;
    } else {
        result["upstream"] = "mic unknown";
    }

    QJsonArray injectorArray;
    auto streamsCopy = getAudioStreams();
    for (auto& injectorPair : streamsCopy) {
        if (injectorPair->getType() == PositionalAudioStream::Injector) {
            QJsonObject upstreamStats;

            AudioStreamStats streamStats = injectorPair->getAudioStreamStats();
            upstreamStats["inj.desired"]  = streamStats._desiredJitterBufferFrames;
            upstreamStats["desired_calc"] = injectorPair->getCalculatedJitterBufferFrames();
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

            upstreamStats["min_gap_usecs"] = static_cast<double>(streamStats._timeGapMin);
            upstreamStats["max_gap_usecs"] = static_cast<double>(streamStats._timeGapMax);
            upstreamStats["avg_gap_usecs"] = static_cast<double>(streamStats._timeGapAverage);
            upstreamStats["min_gap_30s_usecs"] = static_cast<double>(streamStats._timeGapWindowMin);
            upstreamStats["max_gap_30s_usecs"] = static_cast<double>(streamStats._timeGapWindowMax);
            upstreamStats["avg_gap_30s_usecs"] = static_cast<double>(streamStats._timeGapWindowAverage);
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
        avatarAudioStream->setupCodec(codec, codecName, avatarAudioStream->isStereo() ? AudioConstants::STEREO : AudioConstants::MONO);
        qCDebug(audio) << "setting AvatarAudioStream... codec:" << _selectedCodecName << "isStereo:" << avatarAudioStream->isStereo();
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

void AudioMixerClientData::setupCodecForReplicatedAgent(QSharedPointer<ReceivedMessage> message) {
    // hop past the sequence number that leads the packet
    message->seek(sizeof(quint16));

    // pull the codec string from the packet
    auto codecString = message->readString();

    if (codecString != _selectedCodecName) {
        qCDebug(audio) << "Manually setting codec for replicated agent" << uuidStringWithoutCurlyBraces(getNodeID())
        << "-" << codecString;

        const std::pair<QString, CodecPluginPointer> codec = AudioMixer::negotiateCodec({ codecString });
        setupCodec(codec.second, codec.first);

        // seek back to the beginning of the message so other readers are in the right place
        message->seek(0);
    }
}
