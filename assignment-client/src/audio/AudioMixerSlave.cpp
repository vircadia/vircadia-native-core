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

#include "AudioMixerSlave.h"

void AudioMixerSlave::mix(const SharedNodePointer& node, unsigned int frame) {
    AudioMixerClientData* data = (AudioMixerClientData*)node->getLinkedData();
    if (data == nullptr) {
        return;
    }

    auto avatarStream = data->getAvatarAudioStream();
    if (avatarStream == nullptr) {
        return;
    }

    auto nodeList = DependencyManager::get<NodeList>();

    // mute the avatar, if necessary
    if (AudioMixer::shouldMute(avatarStream->getQuietestFrameLoudness()) || data->shouldMuteClient()) {
        auto mutePacket = NLPacket::create(PacketType::NoisyMute, 0);
        nodeList->sendPacket(std::move(mutePacket), *node);

        // probably now we just reset the flag, once should do it (?)
        data->setShouldMuteClient(false);
    }

    // generate and send audio packets
    if (node->getType() == NodeType::Agent && node->getActiveSocket()) {

        // mix streams
        bool mixHasAudio = prepareMix(node);

        // write the packet
        std::unique_ptr<NLPacket> mixPacket;
        if (mixHasAudio || data->shouldFlushEncoder()) {
            // encode the audio
            QByteArray encodedBuffer;
            if (mixHasAudio) {
                QByteArray decodedBuffer(reinterpret_cast<char*>(_clampedSamples), AudioConstants::NETWORK_FRAME_BYTES_STEREO);
                data->encode(decodedBuffer, encodedBuffer);
            } else {
                // time to flush, which resets the shouldFlush until next time we encode something
                data->encodeFrameOfZeros(encodedBuffer);
            }

            // write it to a packet
            writeMixPacket(mixPacket, data, encodedBuffer);
        } else {
            writeSilentPacket(mixPacket, data);
        }

        // send audio environment packet
        sendEnvironmentPacket(node);

        // send mixed audio packet
        nodeList->sendPacket(std::move(mixPacket), *node);
        data->incrementOutgoingMixedAudioSequenceNumber();

        // send an audio stream stats packet to the client approximately every second
        static const unsigned int NUM_FRAMES_PER_SEC = (int) ceil(AudioConstants::NETWORK_FRAMES_PER_SEC);
        if (data->shouldSendStats(frame % NUM_FRAMES_PER_SEC)) {
            data->sendAudioStreamStatsPackets(node);
        }

        ++stats.sumListeners;
    }
}

void AudioMixerSlave::writeMixPacket(std::unique_ptr<NLPacket>& mixPacket, AudioMixerClientData* data, QByteArray& buffer) {
    int mixPacketBytes = sizeof(quint16) + AudioConstants::MAX_CODEC_NAME_LENGTH_ON_WIRE
        + AudioConstants::NETWORK_FRAME_BYTES_STEREO;
    mixPacket = NLPacket::create(PacketType::MixedAudio, mixPacketBytes);

    // pack sequence number
    quint16 sequence = data->getOutgoingSequenceNumber();
    mixPacket->writePrimitive(sequence);

    // write the codec
    QString codecInPacket = data->getCodecName();
    mixPacket->writeString(codecInPacket);

    // pack mixed audio samples
    mixPacket->write(buffer.constData(), buffer.size());
}

void AudioMixerSlave::writeSilentPacket(std::unique_ptr<NLPacket>& mixPacket, AudioMixerClientData* data) {
    int silentPacketBytes = sizeof(quint16) + sizeof(quint16) + AudioConstants::MAX_CODEC_NAME_LENGTH_ON_WIRE;
    mixPacket = NLPacket::create(PacketType::SilentAudioFrame, silentPacketBytes);

    // pack sequence number
    quint16 sequence = data->getOutgoingSequenceNumber();
    mixPacket->writePrimitive(sequence);

    // write the codec
    QString codecInPacket = data->getCodecName();
    mixPacket->writeString(codecInPacket);

    // pack number of silent audio samples
    quint16 numSilentSamples = AudioConstants::NETWORK_FRAME_SAMPLES_STEREO;
    mixPacket->writePrimitive(numSilentSamples);
}

void AudioMixerSlave::sendEnvironmentPacket(const SharedNodePointer& node) {
    // Send stream properties
    bool hasReverb = false;
    float reverbTime, wetLevel;

    auto& reverbSettings = AudioMixer::getReverbSettings();
    auto& audioZones = AudioMixer::getAudioZones();

    // find reverb properties
    for (int i = 0; i < reverbSettings.size(); ++i) {
        AudioMixerClientData* data = static_cast<AudioMixerClientData*>(node->getLinkedData());
        glm::vec3 streamPosition = data->getAvatarAudioStream()->getPosition();
        AABox box = audioZones[reverbSettings[i].zone];
        if (box.contains(streamPosition)) {
            hasReverb = true;
            reverbTime = reverbSettings[i].reverbTime;
            wetLevel = reverbSettings[i].wetLevel;

            break;
        }
    }

    AudioMixerClientData* nodeData = static_cast<AudioMixerClientData*>(node->getLinkedData());
    AvatarAudioStream* stream = nodeData->getAvatarAudioStream();
    bool dataChanged = (stream->hasReverb() != hasReverb) ||
    (stream->hasReverb() && (stream->getRevebTime() != reverbTime ||
                             stream->getWetLevel() != wetLevel));
    if (dataChanged) {
        // Update stream
        if (hasReverb) {
            stream->setReverb(reverbTime, wetLevel);
        } else {
            stream->clearReverb();
        }
    }

    // Send at change or every so often
    float CHANCE_OF_SEND = 0.01f;
    bool sendData = dataChanged || (randFloat() < CHANCE_OF_SEND);

    if (sendData) {
        auto nodeList = DependencyManager::get<NodeList>();

        unsigned char bitset = 0;

        int packetSize = sizeof(bitset);

        if (hasReverb) {
            packetSize += sizeof(reverbTime) + sizeof(wetLevel);
        }

        auto envPacket = NLPacket::create(PacketType::AudioEnvironment, packetSize);

        if (hasReverb) {
            setAtBit(bitset, HAS_REVERB_BIT);
        }

        envPacket->writePrimitive(bitset);

        if (hasReverb) {
            envPacket->writePrimitive(reverbTime);
            envPacket->writePrimitive(wetLevel);
        }
        nodeList->sendPacket(std::move(envPacket), *node);
    }
}

bool AudioMixerSlave::prepareMix(const SharedNodePointer& node) {
    AvatarAudioStream* nodeAudioStream = static_cast<AudioMixerClientData*>(node->getLinkedData())->getAvatarAudioStream();
    AudioMixerClientData* nodeData = static_cast<AudioMixerClientData*>(node->getLinkedData());

    // zero out the client mix for this node
    memset(_mixedSamples, 0, sizeof(_mixedSamples));

    // loop through all other nodes that have sufficient audio to mix
    DependencyManager::get<NodeList>()->eachNode([&](const SharedNodePointer& otherNode){
        // make sure that we have audio data for this other node
        // and that it isn't being ignored by our listening node
        // and that it isn't ignoring our listening node
        if (otherNode->getLinkedData()
            && !node->isIgnoringNodeWithID(otherNode->getUUID()) && !otherNode->isIgnoringNodeWithID(node->getUUID()))  {
            AudioMixerClientData* otherData = static_cast<AudioMixerClientData*>(otherNode->getLinkedData());

            // check if distance is inside ignore radius
            if (node->isIgnoreRadiusEnabled() || otherNode->isIgnoreRadiusEnabled()) {
                float ignoreRadius = glm::min(node->getIgnoreRadius(), otherNode->getIgnoreRadius());
                if (glm::distance(nodeData->getPosition(), otherData->getPosition()) < ignoreRadius) {
                    // skip, distance is inside ignore radius
                    return;
                }
            }

            // enumerate the ARBs attached to the otherNode and add all that should be added to mix
            auto streamsCopy = otherData->getAudioStreams();
            for (auto& streamPair : streamsCopy) {
                auto otherNodeStream = streamPair.second;
                if (*otherNode != *node || otherNodeStream->shouldLoopbackForNode()) {
                    addStreamToMix(*nodeData, otherNode->getUUID(), *nodeAudioStream, *otherNodeStream);
                }
            }
        }
    });

    // use the per listner AudioLimiter to render the mixed data...
    nodeData->audioLimiter.render(_mixedSamples, _clampedSamples, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

    // check for silent audio after the peak limitor has converted the samples
    bool hasAudio = false;
    for (int i = 0; i < AudioConstants::NETWORK_FRAME_SAMPLES_STEREO; ++i) {
        if (_clampedSamples[i] != 0) {
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
                hrtf.renderSilent(silentMonoBlock, _mixedSamples, HRTF_DATASET_INDEX, azimuth, distance, gain,
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
                _mixedSamples[i] += float(streamPopOutput[i] * gain / AudioConstants::MAX_SAMPLE_VALUE);
            }

            ++stats.manualStereoMixes;
        } else {
            for (int i = 0; i < AudioConstants::NETWORK_FRAME_SAMPLES_STEREO; i += 2) {
                auto monoSample = float(streamPopOutput[i / 2] * gain / AudioConstants::MAX_SAMPLE_VALUE);
                _mixedSamples[i] += monoSample;
                _mixedSamples[i + 1] += monoSample;
            }

            ++stats.manualEchoMixes;
        }

        return;
    }

    // get the existing listener-source HRTF object, or create a new one
    auto& hrtf = listenerNodeData.hrtfForStream(sourceNodeID, streamToAdd.getStreamIdentifier());

    static int16_t streamBlock[AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL];

    streamPopOutput.readSamples(streamBlock, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

    // if the frame we're about to mix is silent, simply call render silent and move on
    if (streamToAdd.getLastPopOutputLoudness() == 0.0f) {
        // silent frame from source

        // we still need to call renderSilent via the HRTF for mono source
        hrtf.renderSilent(streamBlock, _mixedSamples, HRTF_DATASET_INDEX, azimuth, distance, gain,
                          AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

        ++stats.hrtfSilentRenders;

        return;
    }

    float audibilityThreshold = AudioMixer::getMinimumAudibilityThreshold();
    if (audibilityThreshold > 0.0f &&
        streamToAdd.getLastPopOutputTrailingLoudness() / glm::length(relativePosition) <= audibilityThreshold) {
        // the mixer is struggling so we're going to drop off some streams

        // we call renderSilent via the HRTF with the actual frame data and a gain of 0.0
        hrtf.renderSilent(streamBlock, _mixedSamples, HRTF_DATASET_INDEX, azimuth, distance, 0.0f,
                          AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

        ++stats.hrtfStruggleRenders;

        return;
    }

    ++stats.hrtfRenders;

    // mono stream, call the HRTF with our block and calculated azimuth and gain
    hrtf.render(streamBlock, _mixedSamples, HRTF_DATASET_INDEX, azimuth, distance, gain,
                AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);
}

const int IEEE754_MANT_BITS = 23;
const int IEEE754_EXPN_BIAS = 127;

//
// for x  > 0.0f, returns log2(x)
// for x <= 0.0f, returns large negative value
//
// abs |error| < 8e-3, smooth (exact for x=2^N) for NPOLY=3
// abs |error| < 2e-4, smooth (exact for x=2^N) for NPOLY=5
// rel |error| < 0.4 from precision loss very close to 1.0f
//
static inline float fastlog2(float x) {

    union { float f; int32_t i; } mant, bits = { x };

    // split into mantissa and exponent
    mant.i = (bits.i & ((1 << IEEE754_MANT_BITS) - 1)) | (IEEE754_EXPN_BIAS << IEEE754_MANT_BITS);
    int32_t expn = (bits.i >> IEEE754_MANT_BITS) - IEEE754_EXPN_BIAS;

    mant.f -= 1.0f;

    // polynomial for log2(1+x) over x=[0,1]
    //x = (-0.346555386f * mant.f + 1.346555386f) * mant.f;
    x = (((-0.0821307180f * mant.f + 0.321188984f) * mant.f - 0.677784014f) * mant.f + 1.43872575f) * mant.f;

    return x + expn;
}

//
// for -126 <= x < 128, returns exp2(x)
//
// rel |error| < 3e-3, smooth (exact for x=N) for NPOLY=3
// rel |error| < 9e-6, smooth (exact for x=N) for NPOLY=5
//
static inline float fastexp2(float x) {

    union { float f; int32_t i; } xi;

    // bias such that x > 0
    x += IEEE754_EXPN_BIAS;
    //x = MAX(x, 1.0f);
    //x = MIN(x, 254.9999f);

    // split into integer and fraction
    xi.i = (int32_t)x;
    x -= xi.i;

    // construct exp2(xi) as a float
    xi.i <<= IEEE754_MANT_BITS;

    // polynomial for exp2(x) over x=[0,1]
    //x = (0.339766028f * x + 0.660233972f) * x + 1.0f;
    x = (((0.0135557472f * x + 0.0520323690f) * x + 0.241379763f) * x + 0.693032121f) * x + 1.0f;

    return x * xi.f;
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
        float distanceCoefficient = fastexp2(fastlog2(g) * fastlog2(distanceBetween/ATTENUATION_BEGINS_AT_DISTANCE));

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
