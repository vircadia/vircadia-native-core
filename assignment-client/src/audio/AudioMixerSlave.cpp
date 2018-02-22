//
//  AudioMixerSlave.cpp
//  assignment-client/src/audio
//
//  Created by Zach Pomerantz on 11/22/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <LogHandler.h>
#include <NetworkAccessManager.h>
#include <NodeList.h>
#include <Node.h>
#include <OctreeConstants.h>
#include <plugins/PluginManager.h>
#include <plugins/CodecPlugin.h>
#include <udt/PacketHeaders.h>
#include <SharedUtil.h>
#include <StDev.h>
#include <UUID.h>

#include "AudioRingBuffer.h"
#include "AudioMixer.h"
#include "AudioMixerClientData.h"
#include "AvatarAudioStream.h"
#include "InjectedAudioStream.h"
#include "AudioHelpers.h"

#include "AudioMixerSlave.h"

using AudioStreamMap = AudioMixerClientData::AudioStreamMap;

// packet helpers
std::unique_ptr<NLPacket> createAudioPacket(PacketType type, int size, quint16 sequence, QString codec);
void sendMixPacket(const SharedNodePointer& node, AudioMixerClientData& data, QByteArray& buffer);
void sendSilentPacket(const SharedNodePointer& node, AudioMixerClientData& data);
void sendMutePacket(const SharedNodePointer& node, AudioMixerClientData&);
void sendEnvironmentPacket(const SharedNodePointer& node, AudioMixerClientData& data);

// mix helpers
inline float approximateGain(const AvatarAudioStream& listeningNodeStream, const PositionalAudioStream& streamToAdd,
        const glm::vec3& relativePosition);
inline float computeGain(const AudioMixerClientData& listenerNodeData, const AvatarAudioStream& listeningNodeStream,
        const PositionalAudioStream& streamToAdd, const glm::vec3& relativePosition, bool isEcho);
inline float computeAzimuth(const AvatarAudioStream& listeningNodeStream, const PositionalAudioStream& streamToAdd,
        const glm::vec3& relativePosition);

void AudioMixerSlave::processPackets(const SharedNodePointer& node) {
    AudioMixerClientData* data = (AudioMixerClientData*)node->getLinkedData();
    if (data) {
        data->processPackets();
    }
}

void AudioMixerSlave::configureMix(ConstIter begin, ConstIter end, unsigned int frame, float throttlingRatio) {
    _begin = begin;
    _end = end;
    _frame = frame;
    _throttlingRatio = throttlingRatio;
}

void AudioMixerSlave::mix(const SharedNodePointer& node) {
    // check that the node is valid
    AudioMixerClientData* data = (AudioMixerClientData*)node->getLinkedData();
    if (data == nullptr) {
        return;
    }

    if (node->isUpstream()) {
        return;
    }

    // check that the stream is valid
    auto avatarStream = data->getAvatarAudioStream();
    if (avatarStream == nullptr) {
        return;
    }

    // send mute packet, if necessary
    if (AudioMixer::shouldMute(avatarStream->getQuietestFrameLoudness()) || data->shouldMuteClient()) {
        sendMutePacket(node, *data);
    }

    // send audio packets, if necessary
    if (node->getType() == NodeType::Agent && node->getActiveSocket()) {
        ++stats.sumListeners;

        // mix the audio
        bool mixHasAudio = prepareMix(node);

        // send audio packet
        if (mixHasAudio || data->shouldFlushEncoder()) {
            QByteArray encodedBuffer;
            if (mixHasAudio) {
                // encode the audio
                QByteArray decodedBuffer(reinterpret_cast<char*>(_bufferSamples), AudioConstants::NETWORK_FRAME_BYTES_STEREO);
                data->encode(decodedBuffer, encodedBuffer);
            } else {
                // time to flush (resets shouldFlush until the next encode)
                data->encodeFrameOfZeros(encodedBuffer);
            }

            sendMixPacket(node, *data, encodedBuffer);
        } else {
            ++stats.sumListenersSilent;
            sendSilentPacket(node, *data);
        }

        // send environment packet
        sendEnvironmentPacket(node, *data);

        // send stats packet (about every second)
        const unsigned int NUM_FRAMES_PER_SEC = (int)ceil(AudioConstants::NETWORK_FRAMES_PER_SEC);
        if (data->shouldSendStats(_frame % NUM_FRAMES_PER_SEC)) {
            data->sendAudioStreamStatsPackets(node);
        }
    }
}

bool AudioMixerSlave::prepareMix(const SharedNodePointer& listener) {
    AvatarAudioStream* listenerAudioStream = static_cast<AudioMixerClientData*>(listener->getLinkedData())->getAvatarAudioStream();
    AudioMixerClientData* listenerData = static_cast<AudioMixerClientData*>(listener->getLinkedData());

    // if we received an invalid position from this listener, then refuse to make them a mix
    // because we don't know how to do it properly
    if (!listenerAudioStream->hasValidPosition()) {
        return false;
    }

    // zero out the mix for this listener
    memset(_mixSamples, 0, sizeof(_mixSamples));

    bool isThrottling = _throttlingRatio > 0.0f;
    std::vector<std::pair<float, SharedNodePointer>> throttledNodes;

    typedef void (AudioMixerSlave::*MixFunctor)(
            AudioMixerClientData&, const QUuid&, const AvatarAudioStream&, const PositionalAudioStream&);
    auto forAllStreams = [&](const SharedNodePointer& node, AudioMixerClientData* nodeData, MixFunctor mixFunctor) {
        auto nodeID = node->getUUID();
        for (auto& streamPair : nodeData->getAudioStreams()) {
            auto nodeStream = streamPair.second;
            (this->*mixFunctor)(*listenerData, nodeID, *listenerAudioStream, *nodeStream);
        }
    };

#ifdef HIFI_AUDIO_MIXER_DEBUG
    auto mixStart = p_high_resolution_clock::now();
#endif

    std::for_each(_begin, _end, [&](const SharedNodePointer& node) {
        AudioMixerClientData* nodeData = static_cast<AudioMixerClientData*>(node->getLinkedData());
        if (!nodeData) {
            return;
        }

        if (*node == *listener) {
            // only mix the echo, if requested
            for (auto& streamPair : nodeData->getAudioStreams()) {
                auto nodeStream = streamPair.second;
                if (nodeStream->shouldLoopbackForNode()) {
                    mixStream(*listenerData, node->getUUID(), *listenerAudioStream, *nodeStream);
                }
            }
        } else if (!listenerData->shouldIgnore(listener, node, _frame)) {
            if (!isThrottling) {
                forAllStreams(node, nodeData, &AudioMixerSlave::mixStream);
            } else {
                auto nodeID = node->getUUID();

                // compute the node's max relative volume
                float nodeVolume;
                for (auto& streamPair : nodeData->getAudioStreams()) {
                    auto nodeStream = streamPair.second;

                    // approximate the gain
                    glm::vec3 relativePosition = nodeStream->getPosition() - listenerAudioStream->getPosition();
                    float gain = approximateGain(*listenerAudioStream, *nodeStream, relativePosition);

                    // modify by hrtf gain adjustment
                    auto& hrtf = listenerData->hrtfForStream(nodeID, nodeStream->getStreamIdentifier());
                    gain *= hrtf.getGainAdjustment();

                    auto streamVolume = nodeStream->getLastPopOutputTrailingLoudness() * gain;
                    nodeVolume = std::max(streamVolume, nodeVolume);
                }

                // max-heapify the nodes by relative volume
                throttledNodes.push_back(std::make_pair(nodeVolume, node));
                if (!throttledNodes.empty()) {
                    std::push_heap(throttledNodes.begin(), throttledNodes.end());
                }
            }
        }
    });

    if (isThrottling) {
        // pop the loudest nodes off the heap and mix their streams
        int numToRetain = (int)(std::distance(_begin, _end) * (1 - _throttlingRatio));
        for (int i = 0; i < numToRetain; i++) {
            if (throttledNodes.empty()) {
                break;
            }

            std::pop_heap(throttledNodes.begin(), throttledNodes.end());

            auto& node = throttledNodes.back().second;
            AudioMixerClientData* nodeData = static_cast<AudioMixerClientData*>(node->getLinkedData());
            forAllStreams(node, nodeData, &AudioMixerSlave::mixStream);

            throttledNodes.pop_back();
        }

        // throttle the remaining nodes' streams
        for (const std::pair<float, SharedNodePointer>& nodePair : throttledNodes) {
            auto& node = nodePair.second;
            AudioMixerClientData* nodeData = static_cast<AudioMixerClientData*>(node->getLinkedData());
            forAllStreams(node, nodeData, &AudioMixerSlave::throttleStream);
        }
    }

#ifdef HIFI_AUDIO_MIXER_DEBUG
    auto mixEnd = p_high_resolution_clock::now();
    auto mixTime = std::chrono::duration_cast<std::chrono::nanoseconds>(mixEnd - mixStart);
    stats.mixTime += mixTime.count();
#endif

    // check for silent audio before limiting
    // limiting uses a dither and can only guarantee abs(sample) <= 1
    bool hasAudio = false;
    for (int i = 0; i < AudioConstants::NETWORK_FRAME_SAMPLES_STEREO; ++i) {
        if (_mixSamples[i] != 0.0f) {
            hasAudio = true;
            break;
        }
    }

    // use the per listener AudioLimiter to render the mixed data
    listenerData->audioLimiter.render(_mixSamples, _bufferSamples, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

    return hasAudio;
}

void AudioMixerSlave::throttleStream(AudioMixerClientData& listenerNodeData, const QUuid& sourceNodeID,
        const AvatarAudioStream& listeningNodeStream, const PositionalAudioStream& streamToAdd) {
    // only throttle this stream to the mix if it has a valid position, we won't know how to mix it otherwise
    if (streamToAdd.hasValidPosition()) {
        addStream(listenerNodeData, sourceNodeID, listeningNodeStream, streamToAdd, true);
    }
}

void AudioMixerSlave::mixStream(AudioMixerClientData& listenerNodeData, const QUuid& sourceNodeID,
        const AvatarAudioStream& listeningNodeStream, const PositionalAudioStream& streamToAdd) {
    // only add the stream to the mix if it has a valid position, we won't know how to mix it otherwise
    if (streamToAdd.hasValidPosition()) {
        addStream(listenerNodeData, sourceNodeID, listeningNodeStream, streamToAdd, false);
    }
}

void AudioMixerSlave::addStream(AudioMixerClientData& listenerNodeData, const QUuid& sourceNodeID,
        const AvatarAudioStream& listeningNodeStream, const PositionalAudioStream& streamToAdd,
        bool throttle) {
    ++stats.totalMixes;

    // to reduce artifacts we call the HRTF functor for every source, even if throttled or silent
    // this ensures the correct tail from last mixed block and the correct spatialization of next first block

    // check if this is a server echo of a source back to itself
    bool isEcho = (&streamToAdd == &listeningNodeStream);

    glm::vec3 relativePosition = streamToAdd.getPosition() - listeningNodeStream.getPosition();

    float distance = glm::max(glm::length(relativePosition), EPSILON);
    float gain = computeGain(listenerNodeData, listeningNodeStream, streamToAdd, relativePosition, isEcho);
    float azimuth = isEcho ? 0.0f : computeAzimuth(listeningNodeStream, listeningNodeStream, relativePosition);
    const int HRTF_DATASET_INDEX = 1;

    if (!streamToAdd.lastPopSucceeded()) {
        bool forceSilentBlock = true;

        if (!streamToAdd.getLastPopOutput().isNull()) {
            bool isInjector = dynamic_cast<const InjectedAudioStream*>(&streamToAdd);

            // in an injector, just go silent - the injector has likely ended
            // in other inputs (microphone, &c.), repeat with fade to avoid the harsh jump to silence
            if (!isInjector) {
                // calculate its fade factor, which depends on how many times it's already been repeated.
                float fadeFactor = calculateRepeatedFrameFadeFactor(streamToAdd.getConsecutiveNotMixedCount() - 1);
                if (fadeFactor > 0.0f) {
                    // apply the fadeFactor to the gain
                    gain *= fadeFactor;
                    forceSilentBlock = false;
                }
            }
        }

        if (forceSilentBlock) {
            // call renderSilent with a forced silent block to reduce artifacts
            // (this is not done for stereo streams since they do not go through the HRTF)
            if (!streamToAdd.isStereo() && !isEcho) {
                // get the existing listener-source HRTF object, or create a new one
                auto& hrtf = listenerNodeData.hrtfForStream(sourceNodeID, streamToAdd.getStreamIdentifier());

                static int16_t silentMonoBlock[AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL] = {};
                hrtf.renderSilent(silentMonoBlock, _mixSamples, HRTF_DATASET_INDEX, azimuth, distance, gain,
                                  AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

                ++stats.hrtfSilentRenders;
            }

            return;
        }
    }

    // grab the stream from the ring buffer
    AudioRingBuffer::ConstIterator streamPopOutput = streamToAdd.getLastPopOutput();

    // stereo sources are not passed through HRTF
    if (streamToAdd.isStereo()) {
        for (int i = 0; i < AudioConstants::NETWORK_FRAME_SAMPLES_STEREO; ++i) {
            _mixSamples[i] += float(streamPopOutput[i] * gain / AudioConstants::MAX_SAMPLE_VALUE);
        }

        ++stats.manualStereoMixes;
        return;
    }

    // echo sources are not passed through HRTF
    if (isEcho) {
        for (int i = 0; i < AudioConstants::NETWORK_FRAME_SAMPLES_STEREO; i += 2) {
            auto monoSample = float(streamPopOutput[i / 2] * gain / AudioConstants::MAX_SAMPLE_VALUE);
            _mixSamples[i] += monoSample;
            _mixSamples[i + 1] += monoSample;
        }

        ++stats.manualEchoMixes;
        return;
    }

    // get the existing listener-source HRTF object, or create a new one
    auto& hrtf = listenerNodeData.hrtfForStream(sourceNodeID, streamToAdd.getStreamIdentifier());

    streamPopOutput.readSamples(_bufferSamples, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

    if (streamToAdd.getLastPopOutputLoudness() == 0.0f) {
        // call renderSilent to reduce artifacts
        hrtf.renderSilent(_bufferSamples, _mixSamples, HRTF_DATASET_INDEX, azimuth, distance, gain,
                          AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

        ++stats.hrtfSilentRenders;
        return;
    }

    if (throttle) {
        // call renderSilent with actual frame data and a gain of 0.0f to reduce artifacts
        hrtf.renderSilent(_bufferSamples, _mixSamples, HRTF_DATASET_INDEX, azimuth, distance, 0.0f,
                          AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

        ++stats.hrtfThrottleRenders;
        return;
    }

    hrtf.render(_bufferSamples, _mixSamples, HRTF_DATASET_INDEX, azimuth, distance, gain,
                AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

    ++stats.hrtfRenders;
}

std::unique_ptr<NLPacket> createAudioPacket(PacketType type, int size, quint16 sequence, QString codec) {
    auto audioPacket = NLPacket::create(type, size);
    audioPacket->writePrimitive(sequence);
    audioPacket->writeString(codec);
    return audioPacket;
}

void sendMixPacket(const SharedNodePointer& node, AudioMixerClientData& data, QByteArray& buffer) {
    const int MIX_PACKET_SIZE =
        sizeof(quint16) + AudioConstants::MAX_CODEC_NAME_LENGTH_ON_WIRE + AudioConstants::NETWORK_FRAME_BYTES_STEREO;
    quint16 sequence = data.getOutgoingSequenceNumber();
    QString codec = data.getCodecName();
    auto mixPacket = createAudioPacket(PacketType::MixedAudio, MIX_PACKET_SIZE, sequence, codec);

    // pack samples
    mixPacket->write(buffer.constData(), buffer.size());

    // send packet
    DependencyManager::get<NodeList>()->sendPacket(std::move(mixPacket), *node);
    data.incrementOutgoingMixedAudioSequenceNumber();
}

void sendSilentPacket(const SharedNodePointer& node, AudioMixerClientData& data) {
    const int SILENT_PACKET_SIZE =
        sizeof(quint16) + AudioConstants::MAX_CODEC_NAME_LENGTH_ON_WIRE + sizeof(quint16);
    quint16 sequence = data.getOutgoingSequenceNumber();
    QString codec = data.getCodecName();
    auto mixPacket = createAudioPacket(PacketType::SilentAudioFrame, SILENT_PACKET_SIZE, sequence, codec);

    // pack number of samples
    mixPacket->writePrimitive(AudioConstants::NETWORK_FRAME_SAMPLES_STEREO);

    // send packet
    DependencyManager::get<NodeList>()->sendPacket(std::move(mixPacket), *node);
    data.incrementOutgoingMixedAudioSequenceNumber();
}

void sendMutePacket(const SharedNodePointer& node, AudioMixerClientData& data) {
    auto mutePacket = NLPacket::create(PacketType::NoisyMute, 0);
    DependencyManager::get<NodeList>()->sendPacket(std::move(mutePacket), *node);

    // probably now we just reset the flag, once should do it (?)
    data.setShouldMuteClient(false);
}

void sendEnvironmentPacket(const SharedNodePointer& node, AudioMixerClientData& data) {
    bool hasReverb = false;
    float reverbTime, wetLevel;

    auto& reverbSettings = AudioMixer::getReverbSettings();
    auto& audioZones = AudioMixer::getAudioZones();

    AvatarAudioStream* stream = data.getAvatarAudioStream();
    glm::vec3 streamPosition = stream->getPosition();

    // find reverb properties
    for (int i = 0; i < reverbSettings.size(); ++i) {
        AABox box = audioZones[reverbSettings[i].zone];
        if (box.contains(streamPosition)) {
            hasReverb = true;
            reverbTime = reverbSettings[i].reverbTime;
            wetLevel = reverbSettings[i].wetLevel;
            break;
        }
    }

    // check if data changed
    bool dataChanged = (stream->hasReverb() != hasReverb) ||
        (stream->hasReverb() && (stream->getRevebTime() != reverbTime || stream->getWetLevel() != wetLevel));
    if (dataChanged) {
        // update stream
        if (hasReverb) {
            stream->setReverb(reverbTime, wetLevel);
        } else {
            stream->clearReverb();
        }
    }

    // send packet at change or every so often
    float CHANCE_OF_SEND = 0.01f;
    bool sendData = dataChanged || (randFloat() < CHANCE_OF_SEND);

    if (sendData) {
        // size the packet
        unsigned char bitset = 0;
        int packetSize = sizeof(bitset);
        if (hasReverb) {
            packetSize += sizeof(reverbTime) + sizeof(wetLevel);
        }

        // write the packet
        auto envPacket = NLPacket::create(PacketType::AudioEnvironment, packetSize);
        if (hasReverb) {
            setAtBit(bitset, HAS_REVERB_BIT);
        }
        envPacket->writePrimitive(bitset);
        if (hasReverb) {
            envPacket->writePrimitive(reverbTime);
            envPacket->writePrimitive(wetLevel);
        }

        // send the packet
        DependencyManager::get<NodeList>()->sendPacket(std::move(envPacket), *node);
    }
}

float approximateGain(const AvatarAudioStream& listeningNodeStream, const PositionalAudioStream& streamToAdd,
        const glm::vec3& relativePosition) {
    float gain = 1.0f;

    // injector: apply attenuation
    if (streamToAdd.getType() == PositionalAudioStream::Injector) {
        gain *= reinterpret_cast<const InjectedAudioStream*>(&streamToAdd)->getAttenuationRatio();
    }

    // avatar: skip attenuation - it is too costly to approximate

    // distance attenuation: approximate, ignore zone-specific attenuations
    // this is a good approximation for streams further than ATTENUATION_START_DISTANCE
    // those streams closer will be amplified; amplifying close streams is acceptable
    // when throttling, as close streams are expected to be heard by a user
    float distance = glm::length(relativePosition);
    return gain / distance;

    // avatar: skip master gain - it is constant for all streams
}

float computeGain(const AudioMixerClientData& listenerNodeData, const AvatarAudioStream& listeningNodeStream,
        const PositionalAudioStream& streamToAdd, const glm::vec3& relativePosition, bool isEcho) {
    float gain = 1.0f;

    // injector: apply attenuation
    if (streamToAdd.getType() == PositionalAudioStream::Injector) {
        gain *= reinterpret_cast<const InjectedAudioStream*>(&streamToAdd)->getAttenuationRatio();

    // avatar: apply fixed off-axis attenuation to make them quieter as they turn away
    } else if (!isEcho && (streamToAdd.getType() == PositionalAudioStream::Microphone)) {
        glm::vec3 rotatedListenerPosition = glm::inverse(streamToAdd.getOrientation()) * relativePosition;

        // source directivity is based on angle of emission, in local coordinates
        glm::vec3 direction = glm::normalize(rotatedListenerPosition);
        float angleOfDelivery = fastAcosf(glm::clamp(-direction.z, -1.0f, 1.0f));   // UNIT_NEG_Z is "forward"

        const float MAX_OFF_AXIS_ATTENUATION = 0.2f;
        const float OFF_AXIS_ATTENUATION_STEP = (1 - MAX_OFF_AXIS_ATTENUATION) / 2.0f;
        float offAxisCoefficient = MAX_OFF_AXIS_ATTENUATION + (angleOfDelivery * (OFF_AXIS_ATTENUATION_STEP / PI_OVER_TWO));

        gain *= offAxisCoefficient;

        // apply master gain, only to avatars
        gain *= listenerNodeData.getMasterAvatarGain();
    }

    auto& audioZones = AudioMixer::getAudioZones();
    auto& zoneSettings = AudioMixer::getZoneSettings();

    // find distance attenuation coefficient
    float attenuationPerDoublingInDistance = AudioMixer::getAttenuationPerDoublingInDistance();
    for (int i = 0; i < zoneSettings.length(); ++i) {
        if (audioZones[zoneSettings[i].source].contains(streamToAdd.getPosition()) &&
            audioZones[zoneSettings[i].listener].contains(listeningNodeStream.getPosition())) {
            attenuationPerDoublingInDistance = zoneSettings[i].coefficient;
            break;
        }
    }

    // distance attenuation
    const float ATTENUATION_START_DISTANCE = 1.0f;
    float distance = glm::length(relativePosition);
    assert(ATTENUATION_START_DISTANCE > EPSILON);
    if (distance >= ATTENUATION_START_DISTANCE) {

        // translate the zone setting to gain per log2(distance)
        float g = 1.0f - attenuationPerDoublingInDistance;
        g = glm::clamp(g, EPSILON, 1.0f);

        // calculate the distance coefficient using the distance to this node
        float distanceCoefficient = fastExp2f(fastLog2f(g) * fastLog2f(distance/ATTENUATION_START_DISTANCE));

        // multiply the current attenuation coefficient by the distance coefficient
        gain *= distanceCoefficient;
    }

    return gain;
}

float computeAzimuth(const AvatarAudioStream& listeningNodeStream, const PositionalAudioStream& streamToAdd,
        const glm::vec3& relativePosition) {
    glm::quat inverseOrientation = glm::inverse(listeningNodeStream.getOrientation());

    glm::vec3 rotatedSourcePosition = inverseOrientation * relativePosition;

    // project the rotated source position vector onto the XZ plane
    rotatedSourcePosition.y = 0.0f;

    const float SOURCE_DISTANCE_THRESHOLD = 1e-30f;

    float rotatedSourcePositionLength2 = glm::length2(rotatedSourcePosition);
    if (rotatedSourcePositionLength2 > SOURCE_DISTANCE_THRESHOLD) {
        
        // produce an oriented angle about the y-axis
        glm::vec3 direction = rotatedSourcePosition * (1.0f / fastSqrtf(rotatedSourcePositionLength2));
        float angle = fastAcosf(glm::clamp(-direction.z, -1.0f, 1.0f)); // UNIT_NEG_Z is "forward"
        return (direction.x < 0.0f) ? -angle : angle;

    } else {   
        // no azimuth if they are in same spot
        return 0.0f; 
    }
}
