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

std::unique_ptr<NLPacket> createAudioPacket(PacketType type, int size, quint16 sequence, QString codec) {
    auto audioPacket = NLPacket::create(type, size);
    audioPacket->writePrimitive(sequence);
    audioPacket->writeString(codec);
    return audioPacket;
}

void sendMixPacket(const SharedNodePointer& node, AudioMixerClientData& data, QByteArray& buffer) {
    static const int MIX_PACKET_SIZE =
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
    static const int SILENT_PACKET_SIZE =
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

void AudioMixerSlave::configure(ConstIter begin, ConstIter end, unsigned int frame) {
    _begin = begin;
    _end = end;
    _frame = frame;
}

void AudioMixerSlave::mix(const SharedNodePointer& node) {
    // check that the node is valid
    AudioMixerClientData* data = (AudioMixerClientData*)node->getLinkedData();
    if (data == nullptr) {
        return;
    }

    auto avatarStream = data->getAvatarAudioStream();
    if (avatarStream == nullptr) {
        return;
    }

    // send mute packet, if necessary
    if (AudioMixer::shouldMute(avatarStream->getQuietestFrameLoudness()) || data->shouldMuteClient()) {
        auto mutePacket = NLPacket::create(PacketType::NoisyMute, 0);
        DependencyManager::get<NodeList>()->sendPacket(std::move(mutePacket), *node);

        // probably now we just reset the flag, once should do it (?)
        data->setShouldMuteClient(false);
    }

    // send audio packets, if necessary
    if (node->getType() == NodeType::Agent && node->getActiveSocket()) {
        ++stats.sumListeners;

        // mix the audio
        bool mixHasAudio = prepareMix(node);

        // send audio packet
        if (mixHasAudio || data->shouldFlushEncoder()) {
            // encode the audio
            QByteArray encodedBuffer;
            if (mixHasAudio) {
                QByteArray decodedBuffer(reinterpret_cast<char*>(_bufferSamples), AudioConstants::NETWORK_FRAME_BYTES_STEREO);
                data->encode(decodedBuffer, encodedBuffer);
            } else {
                // time to flush, which resets the shouldFlush until next time we encode something
                data->encodeFrameOfZeros(encodedBuffer);
            }

            sendMixPacket(node, *data, encodedBuffer);
        } else {
            sendSilentPacket(node, *data);
        }

        // send environment packet
        sendEnvironmentPacket(node, *data);

        // send stats packet (about every second)
        static const unsigned int NUM_FRAMES_PER_SEC = (int) ceil(AudioConstants::NETWORK_FRAMES_PER_SEC);
        if (data->shouldSendStats(_frame % NUM_FRAMES_PER_SEC)) {
            data->sendAudioStreamStatsPackets(node);
        }
    }
}

bool AudioMixerSlave::prepareMix(const SharedNodePointer& node) {
    AvatarAudioStream* nodeAudioStream = static_cast<AudioMixerClientData*>(node->getLinkedData())->getAvatarAudioStream();
    AudioMixerClientData* nodeData = static_cast<AudioMixerClientData*>(node->getLinkedData());

    // zero out the client mix for this node
    memset(_mixSamples, 0, sizeof(_mixSamples));

    // loop through all other nodes that have sufficient audio to mix
    std::for_each(_begin, _end, [&](const SharedNodePointer& otherNode){
        // make sure that we have audio data for this other node
        // and that it isn't being ignored by our listening node
        // and that it isn't ignoring our listening node
        AudioMixerClientData* otherData = static_cast<AudioMixerClientData*>(otherNode->getLinkedData());

        // When this is true, the AudioMixer will send Audio data to a client about avatars that have ignored them
        bool getsAnyIgnored = nodeData->getRequestsDomainListData() && node->getCanKick();

        if (otherData
            && !node->isIgnoringNodeWithID(otherNode->getUUID())
            && (!otherNode->isIgnoringNodeWithID(node->getUUID()) || getsAnyIgnored))  {

            // check to see if we're ignoring in radius
            bool insideIgnoreRadius = false;
            // If the otherNode equals the node, we're doing a comparison on ourselves
            if (*otherNode == *node) {
                // We'll always be inside the radius in that case.
                insideIgnoreRadius = true;
            // Check to see if the space bubble is enabled
            } else if ((node->isIgnoreRadiusEnabled() || otherNode->isIgnoreRadiusEnabled())) {
                // Define the minimum bubble size
                static const glm::vec3 minBubbleSize = glm::vec3(0.3f, 1.3f, 0.3f);
                AudioMixerClientData* nodeData = reinterpret_cast<AudioMixerClientData*>(node->getLinkedData());
                // Set up the bounding box for the current node
                AABox nodeBox(nodeData->getAvatarBoundingBoxCorner(), nodeData->getAvatarBoundingBoxScale());
                // Clamp the size of the bounding box to a minimum scale
                if (glm::any(glm::lessThan(nodeData->getAvatarBoundingBoxScale(), minBubbleSize))) {
                    nodeBox.setScaleStayCentered(minBubbleSize);
                }
                // Set up the bounding box for the current other node
                AABox otherNodeBox(otherData->getAvatarBoundingBoxCorner(), otherData->getAvatarBoundingBoxScale());
                // Clamp the size of the bounding box to a minimum scale
                if (glm::any(glm::lessThan(otherData->getAvatarBoundingBoxScale(), minBubbleSize))) {
                    otherNodeBox.setScaleStayCentered(minBubbleSize);
                }
                // Quadruple the scale of both bounding boxes
                nodeBox.embiggen(4.0f);
                otherNodeBox.embiggen(4.0f);

                // Perform the collision check between the two bounding boxes
                if (nodeBox.touches(otherNodeBox)) {
                    insideIgnoreRadius = true;
                }
            }

            // Enumerate the audio streams attached to the otherNode
            auto streamsCopy = otherData->getAudioStreams();
            for (auto& streamPair : streamsCopy) {
                auto otherNodeStream = streamPair.second;
                bool isSelfWithEcho = ((*otherNode == *node) && (otherNodeStream->shouldLoopbackForNode()));
                // Add all audio streams that should be added to the mix
                if (isSelfWithEcho || (!isSelfWithEcho && !insideIgnoreRadius && getsAnyIgnored)) {
                    addStreamToMix(*nodeData, otherNode->getUUID(), *nodeAudioStream, *otherNodeStream);
                }
            }
        }
    });

    // use the per listener AudioLimiter to render the mixed data...
    nodeData->audioLimiter.render(_mixSamples, _bufferSamples, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

    // check for silent audio after the peak limiter has converted the samples
    bool hasAudio = false;
    for (int i = 0; i < AudioConstants::NETWORK_FRAME_SAMPLES_STEREO; ++i) {
        if (_bufferSamples[i] != 0) {
            hasAudio = true;
            break;
        }
    }
    return hasAudio;
}

void AudioMixerSlave::addStreamToMix(AudioMixerClientData& listenerNodeData, const QUuid& sourceNodeID,
        const AvatarAudioStream& listeningNodeStream, const PositionalAudioStream& streamToAdd) {
    // to reduce artifacts we calculate the gain and azimuth for every source for this listener
    // even if we are not going to end up mixing in this source

    ++stats.totalMixes;

    // this ensures that the tail of any previously mixed audio or the first block of new audio sounds correct

    // check if this is a server echo of a source back to itself
    bool isEcho = (&streamToAdd == &listeningNodeStream);

    glm::vec3 relativePosition = streamToAdd.getPosition() - listeningNodeStream.getPosition();

    // figure out the distance between source and listener
    float distance = glm::max(glm::length(relativePosition), EPSILON);

    // figure out the gain for this source at the listener
    float gain = gainForSource(listeningNodeStream, streamToAdd, relativePosition, isEcho);

    // figure out the azimuth to this source at the listener
    float azimuth = isEcho ? 0.0f : azimuthForSource(listeningNodeStream, listeningNodeStream, relativePosition);

    float repeatedFrameFadeFactor = 1.0f;

    static const int HRTF_DATASET_INDEX = 1;

    if (!streamToAdd.lastPopSucceeded()) {
        bool forceSilentBlock = true;

        if (!streamToAdd.getLastPopOutput().isNull()) {
            bool isInjector = dynamic_cast<const InjectedAudioStream*>(&streamToAdd);

            // in an injector, just go silent - the injector has likely ended
            // in other inputs (microphone, &c.), repeat with fade to avoid the harsh jump to silence

            // we'll repeat the last block until it has a block to mix
            // and we'll gradually fade that repeated block into silence.

            // calculate its fade factor, which depends on how many times it's already been repeated.
            repeatedFrameFadeFactor = calculateRepeatedFrameFadeFactor(streamToAdd.getConsecutiveNotMixedCount() - 1);
            if (!isInjector && repeatedFrameFadeFactor > 0.0f) {
                // apply the repeatedFrameFadeFactor to the gain
                gain *= repeatedFrameFadeFactor;

                forceSilentBlock = false;
            }
        }

        if (forceSilentBlock) {
            // we're deciding not to repeat either since we've already done it enough times or repetition with fade is disabled
            // in this case we will call renderSilent with a forced silent block
            // this ensures the correct tail from the previously mixed block and the correct spatialization of first block
            // of any upcoming audio

            if (!streamToAdd.isStereo() && !isEcho) {
                // get the existing listener-source HRTF object, or create a new one
                auto& hrtf = listenerNodeData.hrtfForStream(sourceNodeID, streamToAdd.getStreamIdentifier());

                // this is not done for stereo streams since they do not go through the HRTF
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

    if (streamToAdd.isStereo() || isEcho) {
        // this is a stereo source or server echo so we do not pass it through the HRTF
        // simply apply our calculated gain to each sample
        if (streamToAdd.isStereo()) {
            for (int i = 0; i < AudioConstants::NETWORK_FRAME_SAMPLES_STEREO; ++i) {
                _mixSamples[i] += float(streamPopOutput[i] * gain / AudioConstants::MAX_SAMPLE_VALUE);
            }

            ++stats.manualStereoMixes;
        } else {
            for (int i = 0; i < AudioConstants::NETWORK_FRAME_SAMPLES_STEREO; i += 2) {
                auto monoSample = float(streamPopOutput[i / 2] * gain / AudioConstants::MAX_SAMPLE_VALUE);
                _mixSamples[i] += monoSample;
                _mixSamples[i + 1] += monoSample;
            }

            ++stats.manualEchoMixes;
        }

        return;
    }

    // get the existing listener-source HRTF object, or create a new one
    auto& hrtf = listenerNodeData.hrtfForStream(sourceNodeID, streamToAdd.getStreamIdentifier());

    streamPopOutput.readSamples(_bufferSamples, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

    // if the frame we're about to mix is silent, simply call render silent and move on
    if (streamToAdd.getLastPopOutputLoudness() == 0.0f) {
        // silent frame from source

        // we still need to call renderSilent via the HRTF for mono source
        hrtf.renderSilent(_bufferSamples, _mixSamples, HRTF_DATASET_INDEX, azimuth, distance, gain,
                          AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

        ++stats.hrtfSilentRenders;

        return;
    }

    float audibilityThreshold = AudioMixer::getMinimumAudibilityThreshold();
    if (audibilityThreshold > 0.0f &&
        streamToAdd.getLastPopOutputTrailingLoudness() / glm::length(relativePosition) <= audibilityThreshold) {
        // the mixer is struggling so we're going to drop off some streams

        // we call renderSilent via the HRTF with the actual frame data and a gain of 0.0
        hrtf.renderSilent(_bufferSamples, _mixSamples, HRTF_DATASET_INDEX, azimuth, distance, 0.0f,
                          AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

        ++stats.hrtfStruggleRenders;

        return;
    }

    ++stats.hrtfRenders;

    // mono stream, call the HRTF with our block and calculated azimuth and gain
    hrtf.render(_bufferSamples, _mixSamples, HRTF_DATASET_INDEX, azimuth, distance, gain,
                AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);
}

float AudioMixerSlave::gainForSource(const AvatarAudioStream& listeningNodeStream, const PositionalAudioStream& streamToAdd,
        const glm::vec3& relativePosition, bool isEcho) {
    float gain = 1.0f;

    float distanceBetween = glm::length(relativePosition);

    if (distanceBetween < EPSILON) {
        distanceBetween = EPSILON;
    }

    if (streamToAdd.getType() == PositionalAudioStream::Injector) {
        gain *= reinterpret_cast<const InjectedAudioStream*>(&streamToAdd)->getAttenuationRatio();
    }

    if (!isEcho && (streamToAdd.getType() == PositionalAudioStream::Microphone)) {
        //  source is another avatar, apply fixed off-axis attenuation to make them quieter as they turn away from listener
        glm::vec3 rotatedListenerPosition = glm::inverse(streamToAdd.getOrientation()) * relativePosition;

        float angleOfDelivery = glm::angle(glm::vec3(0.0f, 0.0f, -1.0f),
                                           glm::normalize(rotatedListenerPosition));

        const float MAX_OFF_AXIS_ATTENUATION = 0.2f;
        const float OFF_AXIS_ATTENUATION_FORMULA_STEP = (1 - MAX_OFF_AXIS_ATTENUATION) / 2.0f;

        float offAxisCoefficient = MAX_OFF_AXIS_ATTENUATION +
        (OFF_AXIS_ATTENUATION_FORMULA_STEP * (angleOfDelivery / PI_OVER_TWO));

        // multiply the current attenuation coefficient by the calculated off axis coefficient
        gain *= offAxisCoefficient;
    }

    float attenuationPerDoublingInDistance = AudioMixer::getAttenuationPerDoublingInDistance();
    auto& zoneSettings = AudioMixer::getZoneSettings();
    auto& audioZones = AudioMixer::getAudioZones();
    for (int i = 0; i < zoneSettings.length(); ++i) {
        if (audioZones[zoneSettings[i].source].contains(streamToAdd.getPosition()) &&
            audioZones[zoneSettings[i].listener].contains(listeningNodeStream.getPosition())) {
            attenuationPerDoublingInDistance = zoneSettings[i].coefficient;
            break;
        }
    }

    const float ATTENUATION_BEGINS_AT_DISTANCE = 1.0f;
    if (distanceBetween >= ATTENUATION_BEGINS_AT_DISTANCE) {

        // translate the zone setting to gain per log2(distance)
        float g = 1.0f - attenuationPerDoublingInDistance;
        g = (g < EPSILON) ? EPSILON : g;
        g = (g > 1.0f) ? 1.0f : g;

        // calculate the distance coefficient using the distance to this node
        float distanceCoefficient = fastExp2f(fastLog2f(g) * fastLog2f(distanceBetween/ATTENUATION_BEGINS_AT_DISTANCE));

        // multiply the current attenuation coefficient by the distance coefficient
        gain *= distanceCoefficient;
    }

    return gain;
}

float AudioMixerSlave::azimuthForSource(const AvatarAudioStream& listeningNodeStream, const PositionalAudioStream& streamToAdd,
        const glm::vec3& relativePosition) {
    glm::quat inverseOrientation = glm::inverse(listeningNodeStream.getOrientation());

    //  Compute sample delay for the two ears to create phase panning
    glm::vec3 rotatedSourcePosition = inverseOrientation * relativePosition;

    // project the rotated source position vector onto the XZ plane
    rotatedSourcePosition.y = 0.0f;

    static const float SOURCE_DISTANCE_THRESHOLD = 1e-30f;

    if (glm::length2(rotatedSourcePosition) > SOURCE_DISTANCE_THRESHOLD) {
        // produce an oriented angle about the y-axis
        return glm::orientedAngle(glm::vec3(0.0f, 0.0f, -1.0f), glm::normalize(rotatedSourcePosition), glm::vec3(0.0f, -1.0f, 0.0f));
    } else {
        // there is no distance between listener and source - return no azimuth
        return 0;
    }
}
