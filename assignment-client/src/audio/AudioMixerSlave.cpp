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

#include "AudioMixerSlave.h"

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

using AudioStreamVector = AudioMixerClientData::AudioStreamVector;

// packet helpers
std::unique_ptr<NLPacket> createAudioPacket(PacketType type, int size, quint16 sequence, QString codec);
void sendMixPacket(const SharedNodePointer& node, AudioMixerClientData& data, QByteArray& buffer);
void sendSilentPacket(const SharedNodePointer& node, AudioMixerClientData& data);
void sendMutePacket(const SharedNodePointer& node, AudioMixerClientData&);
void sendEnvironmentPacket(const SharedNodePointer& node, AudioMixerClientData& data);

// mix helpers
inline float approximateGain(const AvatarAudioStream& listeningNodeStream, const PositionalAudioStream& streamToAdd);
inline float computeGain(float masterListenerGain, const AvatarAudioStream& listeningNodeStream,
        const PositionalAudioStream& streamToAdd, const glm::vec3& relativePosition, float distance, bool isEcho);
inline float computeAzimuth(const AvatarAudioStream& listeningNodeStream, const PositionalAudioStream& streamToAdd,
        const glm::vec3& relativePosition);

void AudioMixerSlave::processPackets(const SharedNodePointer& node) {
    AudioMixerClientData* data = (AudioMixerClientData*)node->getLinkedData();
    if (data) {
        // process packets and collect the number of streams available for this frame
        stats.sumStreams += data->processPackets(_sharedData.addedStreams);
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

template<typename V>
bool containsNodeID(const V& vector, QUuid nodeID) {
    return std::any_of(std::begin(vector), std::end(vector), [&nodeID](const QUuid& vectorID){
        return vectorID == nodeID;
    });
}

bool AudioMixerSlave::prepareMix(const SharedNodePointer& listener) {
    AvatarAudioStream* listenerAudioStream = static_cast<AudioMixerClientData*>(listener->getLinkedData())->getAvatarAudioStream();
    AudioMixerClientData* listenerData = static_cast<AudioMixerClientData*>(listener->getLinkedData());

    // zero out the mix for this listener
    memset(_mixSamples, 0, sizeof(_mixSamples));

    bool isThrottling = _throttlingRatio > 0.0f;

    auto nodeList = DependencyManager::get<NodeList>();

#ifdef HIFI_AUDIO_MIXER_DEBUG
    auto mixStart = p_high_resolution_clock::now();
#endif

    auto& mixableStreams = listenerData->getMixableStreams();
    auto& ignoredNodeIDs = listener->getIgnoredNodeIDs();
    auto& ignoringNodeIDs = listenerData->getIgnoringNodeIDs();

    // add data for newly created streams to our vector
    if (!listenerData->getHasReceivedFirstMix()) {
        // when this listener is new, we need to fill its added streams object with all available streams
        std::for_each(_begin, _end, [&](const SharedNodePointer& node) {
            AudioMixerClientData* nodeData = static_cast<AudioMixerClientData*>(node->getLinkedData());
            if (nodeData) {
                for (auto& stream : nodeData->getAudioStreams()) {
                    mixableStreams.emplace_back(node->getUUID(), node->getLocalID(),
                                                stream->getStreamIdentifier(), &(*stream));
                    
                    // pre-populate ignored and ignoring flags for this stream
                    mixableStreams.back().ignoredByListener = containsNodeID(ignoredNodeIDs, node->getUUID());
                    mixableStreams.back().ignoringListener = containsNodeID(ignoringNodeIDs, node->getUUID());
                }
            }
        });

        // flag this listener as having received their first mix so we know we don't need to enumerate all nodes again
        listenerData->setHasReceivedFirstMix(true);
    } else {
        for (const auto& newStream : _sharedData.addedStreams) {
            mixableStreams.emplace_back(newStream.nodeIDStreamID, newStream.positionalStream);

            // pre-populate ignored and ignoring flags for this stream
            mixableStreams.back().ignoredByListener = containsNodeID(ignoredNodeIDs, newStream.nodeIDStreamID.nodeID);
            mixableStreams.back().ignoringListener = containsNodeID(ignoringNodeIDs, newStream.nodeIDStreamID.nodeID);
        }
    }

    // grab the unprocessed ignores and unignores from and for this listener
    const auto& nodesIgnoredByListener = listenerData->getNewIgnoredNodeIDs();
    const auto& nodesUnignoredByListener = listenerData->getNewUnignoredNodeIDs();
    const auto& nodesIgnoringListener = listenerData->getNewIgnoringNodeIDs();
    const auto& nodesUnignoringListener = listenerData->getNewUnignoringNodeIDs();

    // enumerate the available streams
    auto it = mixableStreams.begin();
    auto end = mixableStreams.end();
    while (it != end) {

        // check if this node (and therefore all of the node's streams) has been removed
        auto& nodeIDStreamID = it->nodeStreamID;
        auto matchedRemovedNode = std::find(_sharedData.removedNodes.cbegin(), _sharedData.removedNodes.cend(),
                                            nodeIDStreamID.nodeLocalID);
        bool streamRemoved = matchedRemovedNode != _sharedData.removedNodes.cend();

        // if the node wasn't removed, check if this stream was specifically removed
        if (!streamRemoved) {
            auto matchedRemovedStream = std::find(_sharedData.removedStreams.cbegin(), _sharedData.removedStreams.cend(),
                                                  nodeIDStreamID);
            streamRemoved = matchedRemovedStream != _sharedData.removedStreams.cend();
        }

        if (streamRemoved) {
            // this stream was removed, so swap it with the last item and decrease the end iterator
            --end;
            std::swap(*it, *end);

            // process the it element (which is now the element that was the last item before the swap)
            continue;
        }

        if (it->nodeStreamID.nodeLocalID == listener->getLocalID()) {
            // streams from this node should be skipped unless loopback is specifically requested
            if (it->positionalStream->shouldLoopbackForNode()) {
                it->skippedStream = false;
            } else {
                it->approximateVolume = 0.0f;
                it->skippedStream = true;
                it->completedSilentRender = true;

                // if we know we're skipping this stream, no more processing is required
                // since we don't do silent HRTF renders for echo streams
                ++it;
                continue;
            }
        } else {
            if (it->ignoredByListener && nodesUnignoredByListener.size() > 0) {
                // this stream was previously ignored by the listener and we have some unignored streams
                // check now if it is one of the unignored streams and flag it as such
                it->ignoredByListener = !containsNodeID(nodesUnignoredByListener, nodeIDStreamID.nodeID);

            } else if (!it->ignoredByListener && nodesIgnoredByListener.size() > 0) {
                // this stream was previously not ignored by the listener and we have some newly ignored streams
                // check now if it is one of the ignored streams and flag it as such
                it->ignoredByListener = containsNodeID(nodesIgnoredByListener, nodeIDStreamID.nodeID);
            }

            if (it->ignoringListener && nodesUnignoringListener.size() > 0) {
                // this stream was previously ignoring the listener and we have some new un-ignoring nodes
                // check now if it is one of the unignoring streams and flag it as such
                it->ignoringListener = !containsNodeID(nodesUnignoringListener, nodeIDStreamID.nodeID);
            } else if (!it->ignoringListener && nodesIgnoringListener.size() > 0) {
                it->ignoringListener = containsNodeID(nodesIgnoringListener, nodeIDStreamID.nodeID);
            }

            if (it->ignoredByListener
                || (it->ignoringListener && !(listenerData->getRequestsDomainListData() && listener->getCanKick()))) {
                // this is a stream ignoring by the listener
                // or ignoring the listener (and the listener is not an admin asking for (the poorly named) "domain list" data)
                // mark it skipped and move on
                it->skippedStream = true;
            } else {
                it->skippedStream = false;
            }

            if (!it->skippedStream) {
                if ((listenerAudioStream->isIgnoreBoxEnabled() || it->positionalStream->isIgnoreBoxEnabled())
                    && listenerAudioStream->getIgnoreBox().touches(it->positionalStream->getIgnoreBox())) {
                    // the listener is ignoring audio sources within a radius, and this source is in that radius
                    // so we mark it skipped
                    it->skippedStream = true;
                } else {
                    it->skippedStream = false;
                }
            }
        }

        if (!isThrottling) {
            // we aren't throttling, so we already know that we can add this stream to the mix
            addStream(*it, *listenerAudioStream, listenerData->getMasterAvatarGain(), false);
        } else {
            // we're throttling, so we need to update the approximate volume for any un-skipped streams
            // unless this is simply for an echo (in which case the approx volume is 1.0)
            if (!it->skippedStream && it->positionalStream->getLastPopOutputTrailingLoudness() > 0.0f) {
                if (it->positionalStream != listenerAudioStream) {
                    // approximate the gain
                    float gain = approximateGain(*listenerAudioStream, *(it->positionalStream));

                    // for avatar streams, modify by the set gain adjustment
                    if (nodeIDStreamID.streamID.isNull()) {
                        gain *= it->hrtf->getGainAdjustment();
                    }

                    it->approximateVolume = it->positionalStream->getLastPopOutputTrailingLoudness() * gain;
                } else {
                    it->approximateVolume = 1.0f;
                }
            } else {
                it->approximateVolume = 0.0f;
            }
        }

        ++it;
    }

    // erase any removed streams that were swapped to the end
    mixableStreams.erase(end, mixableStreams.end());

    if (isThrottling) {
        // since we're throttling, we need to partition the mixable into throttled and unthrottled streams
        auto numToRetain = std::distance(_begin, _end) * (1 - _throttlingRatio);
        auto throttlePoint = mixableStreams.begin() + numToRetain;

        std::nth_element(mixableStreams.begin(), throttlePoint, mixableStreams.end(),
                         [](const auto& a, const auto& b)
        {
            return a.approximateVolume > b.approximateVolume;
        });

        for (auto it = mixableStreams.begin(); it != mixableStreams.end(); ++it) {
            // add this stream, it is throttled if it is at or past the throttle iterator in the vector
            addStream(*it, *listenerAudioStream, listenerData->getMasterAvatarGain(), it >= throttlePoint);
        }
    }

    // clear the newly ignored, un-ignored, ignoring, and un-ignoring streams now that we've processed them
    listenerData->clearStagedIgnoreChanges();

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

void AudioMixerSlave::addStream(AudioMixerClientData::MixableStream& mixableStream, AvatarAudioStream& listeningNodeStream,
                                float masterListenerGain, bool throttle) {

    if (mixableStream.skippedStream) {
        // any skipped stream gets no processing and no silent render - early return
        return;
    }

    ++stats.totalMixes;

    auto streamToAdd = mixableStream.positionalStream;

    // to reduce artifacts we still call the HRTF functor for every silent or throttled source
    // for the first frame where the source becomes throttled or silent
    // this ensures the correct tail from last mixed block and the correct spatialization of next first block
    if (throttle || mixableStream.skippedStream || streamToAdd->getLastPopOutputLoudness() == 0.0f) {
        if (mixableStream.completedSilentRender) {

            if (throttle) {
                ++stats.hrtfThrottleRenders;
            }

            return;
        } else {
            mixableStream.completedSilentRender = true;
        }
    } else if (mixableStream.completedSilentRender) {
        // a stream that is no longer throttled or silent should have its silent render flag reset to false
        // so that we complete a silent render for the stream next time it is throttled or otherwise goes silent
        mixableStream.completedSilentRender = false;
    }

    // check if this is a server echo of a source back to itself
    bool isEcho = (streamToAdd == &listeningNodeStream);

    glm::vec3 relativePosition = streamToAdd->getPosition() - listeningNodeStream.getPosition();

    float distance = glm::max(glm::length(relativePosition), EPSILON);
    float gain = computeGain(masterListenerGain, listeningNodeStream, *streamToAdd, relativePosition, distance, isEcho);
    float azimuth = isEcho ? 0.0f : computeAzimuth(listeningNodeStream, listeningNodeStream, relativePosition);
    const int HRTF_DATASET_INDEX = 1;

    if (!streamToAdd->lastPopSucceeded()) {
        bool forceSilentBlock = true;

        if (!streamToAdd->getLastPopOutput().isNull()) {
            bool isInjector = dynamic_cast<const InjectedAudioStream*>(streamToAdd);

            // in an injector, just go silent - the injector has likely ended
            // in other inputs (microphone, &c.), repeat with fade to avoid the harsh jump to silence
            if (!isInjector) {
                // calculate its fade factor, which depends on how many times it's already been repeated.
                float fadeFactor = calculateRepeatedFrameFadeFactor(streamToAdd->getConsecutiveNotMixedCount() - 1);
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
            if (!streamToAdd->isStereo() && !isEcho) {
                static int16_t silentMonoBlock[AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL] = {};
                mixableStream.hrtf->renderSilent(silentMonoBlock, _mixSamples, HRTF_DATASET_INDEX, azimuth, distance, gain,
                                                 AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

                ++stats.hrtfSilentRenders;
            }

            return;
        }
    }

    // grab the stream from the ring buffer
    AudioRingBuffer::ConstIterator streamPopOutput = streamToAdd->getLastPopOutput();

    // stereo sources are not passed through HRTF
    if (streamToAdd->isStereo()) {

        // apply the avatar gain adjustment
        gain *= mixableStream.hrtf->getGainAdjustment();

        const float scale = 1 / 32768.0f; // int16_t to float

        for (int i = 0; i < AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL; i++) {
            _mixSamples[2*i+0] += (float)streamPopOutput[2*i+0] * gain * scale;
            _mixSamples[2*i+1] += (float)streamPopOutput[2*i+1] * gain * scale;
        }

        ++stats.manualStereoMixes;
        return;
    }

    // echo sources are not passed through HRTF
    if (isEcho) {

        const float scale = 1/32768.0f; // int16_t to float

        for (int i = 0; i < AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL; i++) {
            float sample = (float)streamPopOutput[i] * gain * scale;
            _mixSamples[2*i+0] += sample;
            _mixSamples[2*i+1] += sample;
        }

        ++stats.manualEchoMixes;
        return;
    }


    streamPopOutput.readSamples(_bufferSamples, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

    if (streamToAdd->getLastPopOutputLoudness() == 0.0f || mixableStream.skippedStream) {
        // call renderSilent to reduce artifacts
        mixableStream.hrtf->renderSilent(_bufferSamples, _mixSamples, HRTF_DATASET_INDEX, azimuth, distance, gain,
                                         AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

        ++stats.hrtfSilentRenders;
        return;
    }

    if (throttle) {
        // call renderSilent with actual frame data and a gain of 0.0f to reduce artifacts
        mixableStream.hrtf->renderSilent(_bufferSamples, _mixSamples, HRTF_DATASET_INDEX, azimuth, distance, 0.0f,
                                         AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

        ++stats.hrtfThrottleRenders;
        return;
    }

    mixableStream.hrtf->render(_bufferSamples, _mixSamples, HRTF_DATASET_INDEX, azimuth, distance, gain,
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
    for (const auto& settings : reverbSettings) {
        AABox box = audioZones[settings.zone].area;
        if (box.contains(streamPosition)) {
            hasReverb = true;
            reverbTime = settings.reverbTime;
            wetLevel = settings.wetLevel;
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

float approximateGain(const AvatarAudioStream& listeningNodeStream, const PositionalAudioStream& streamToAdd) {
    float gain = 1.0f;

    // injector: apply attenuation
    if (streamToAdd.getType() == PositionalAudioStream::Injector) {
        gain *= reinterpret_cast<const InjectedAudioStream*>(&streamToAdd)->getAttenuationRatio();
    }

    // avatar: skip attenuation - it is too costly to approximate

    // distance attenuation: approximate, ignore zone-specific attenuations
    glm::vec3 relativePosition = streamToAdd.getPosition() - listeningNodeStream.getPosition();
    float distance = glm::length(relativePosition);
    return gain / distance;

    // avatar: skip master gain - it is constant for all streams
}

float computeGain(float masterListenerGain, const AvatarAudioStream& listeningNodeStream,
        const PositionalAudioStream& streamToAdd, const glm::vec3& relativePosition, float distance, bool isEcho) {
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
        gain *= masterListenerGain;
    }

    auto& audioZones = AudioMixer::getAudioZones();
    auto& zoneSettings = AudioMixer::getZoneSettings();

    // find distance attenuation coefficient
    float attenuationPerDoublingInDistance = AudioMixer::getAttenuationPerDoublingInDistance();
    for (const auto& settings : zoneSettings) {
        if (audioZones[settings.source].area.contains(streamToAdd.getPosition()) &&
            audioZones[settings.listener].area.contains(listeningNodeStream.getPosition())) {
            attenuationPerDoublingInDistance = settings.coefficient;
            break;
        }
    }
    // translate the zone setting to gain per log2(distance)
    float g = glm::clamp(1.0f - attenuationPerDoublingInDistance, EPSILON, 1.0f);

    // calculate the attenuation using the distance to this node
    // reference attenuation of 0dB at distance = 1.0m
    gain *= fastExp2f(fastLog2f(g) * fastLog2f(std::max(distance, HRTF_NEARFIELD_MIN)));
    gain = std::min(gain, 1.0f / HRTF_NEARFIELD_MIN);

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
