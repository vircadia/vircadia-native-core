//
//  AudioMixer.cpp
//  assignment-client/src/audio
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <math.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif //_WIN32

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QThread>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include <LogHandler.h>
#include <NetworkAccessManager.h>
#include <NodeList.h>
#include <Node.h>
#include <OctreeConstants.h>
#include <udt/PacketHeaders.h>
#include <SharedUtil.h>
#include <StDev.h>
#include <UUID.h>

#include "AudioRingBuffer.h"
#include "AudioMixerClientData.h"
#include "AvatarAudioStream.h"
#include "InjectedAudioStream.h"

#include "AudioMixer.h"

const float LOUDNESS_TO_DISTANCE_RATIO = 0.00001f;
const float DEFAULT_ATTENUATION_PER_DOUBLING_IN_DISTANCE = 0.18f;
const float DEFAULT_NOISE_MUTING_THRESHOLD = 0.003f;
const QString AUDIO_MIXER_LOGGING_TARGET_NAME = "audio-mixer";
const QString AUDIO_ENV_GROUP_KEY = "audio_env";
const QString AUDIO_BUFFER_GROUP_KEY = "audio_buffer";

InboundAudioStream::Settings AudioMixer::_streamSettings;

bool AudioMixer::_printStreamStats = false;

bool AudioMixer::_enableFilter = true;

bool AudioMixer::shouldMute(float quietestFrame) {
    return (quietestFrame > _noiseMutingThreshold);
}

AudioMixer::AudioMixer(NLPacket& packet) :
    ThreadedAssignment(packet),
    _trailingSleepRatio(1.0f),
    _minAudibilityThreshold(LOUDNESS_TO_DISTANCE_RATIO / 2.0f),
    _performanceThrottlingRatio(0.0f),
    _attenuationPerDoublingInDistance(DEFAULT_ATTENUATION_PER_DOUBLING_IN_DISTANCE),
    _noiseMutingThreshold(DEFAULT_NOISE_MUTING_THRESHOLD),
    _numStatFrames(0),
    _sumListeners(0),
    _sumMixes(0),
    _lastPerSecondCallbackTime(usecTimestampNow()),
    _sendAudioStreamStats(false),
    _datagramsReadPerCallStats(0, READ_DATAGRAMS_STATS_WINDOW_SECONDS),
    _timeSpentPerCallStats(0, READ_DATAGRAMS_STATS_WINDOW_SECONDS),
    _timeSpentPerHashMatchCallStats(0, READ_DATAGRAMS_STATS_WINDOW_SECONDS),
    _readPendingCallsPerSecondStats(1, READ_DATAGRAMS_STATS_WINDOW_SECONDS)
{
    // constant defined in AudioMixer.h.  However, we don't want to include this here
    // we will soon find a better common home for these audio-related constants
    // SOON

    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();

    packetReceiver.registerListenerForTypes({ PacketType::MicrophoneAudioNoEcho, PacketType::MicrophoneAudioWithEcho,
                                              PacketType::InjectAudio, PacketType::SilentAudioFrame,
                                              PacketType::AudioStreamStats },
                                            this, "handleNodeAudioPacket");
    packetReceiver.registerListener(PacketType::MuteEnvironment, this, "handleMuteEnvironmentPacket");
}

const float ATTENUATION_BEGINS_AT_DISTANCE = 1.0f;
const float RADIUS_OF_HEAD = 0.076f;

int AudioMixer::addStreamToMixForListeningNodeWithStream(AudioMixerClientData* listenerNodeData,
                                                         const QUuid& streamUUID,
                                                         PositionalAudioStream* streamToAdd,
                                                         AvatarAudioStream* listeningNodeStream) {
    // If repetition with fade is enabled:
    // If streamToAdd could not provide a frame (it was starved), then we'll mix its previously-mixed frame
    // This is preferable to not mixing it at all since that's equivalent to inserting silence.
    // Basically, we'll repeat that last frame until it has a frame to mix.  Depending on how many times
    // we've repeated that frame in a row, we'll gradually fade that repeated frame into silence.
    // This improves the perceived quality of the audio slightly.

    bool showDebug = false;  // (randFloat() < 0.05f);

    float repeatedFrameFadeFactor = 1.0f;

    if (!streamToAdd->lastPopSucceeded()) {
        if (_streamSettings._repetitionWithFade && !streamToAdd->getLastPopOutput().isNull()) {
            // reptition with fade is enabled, and we do have a valid previous frame to repeat.
            // calculate its fade factor, which depends on how many times it's already been repeated.
            repeatedFrameFadeFactor = calculateRepeatedFrameFadeFactor(streamToAdd->getConsecutiveNotMixedCount() - 1);
            if (repeatedFrameFadeFactor == 0.0f) {
                return 0;
            }
        } else {
            return 0;
        }
    }

    // at this point, we know streamToAdd's last pop output is valid

    // if the frame we're about to mix is silent, bail
    if (streamToAdd->getLastPopOutputLoudness() == 0.0f) {
        return 0;
    }

    float bearingRelativeAngleToSource = 0.0f;
    float attenuationCoefficient = 1.0f;
    int numSamplesDelay = 0;
    float weakChannelAmplitudeRatio = 1.0f;

    //  Is the source that I am mixing my own?
    bool sourceIsSelf = (streamToAdd == listeningNodeStream);

    glm::vec3 relativePosition = streamToAdd->getPosition() - listeningNodeStream->getPosition();

    float distanceBetween = glm::length(relativePosition);

    if (distanceBetween < EPSILON) {
        distanceBetween = EPSILON;
    }

    if (streamToAdd->getLastPopOutputTrailingLoudness() / distanceBetween <= _minAudibilityThreshold) {
        // according to mixer performance we have decided this does not get to be mixed in
        // bail out
        return 0;
    }

    ++_sumMixes;

    if (streamToAdd->getType() == PositionalAudioStream::Injector) {
        attenuationCoefficient *= reinterpret_cast<InjectedAudioStream*>(streamToAdd)->getAttenuationRatio();
        if (showDebug) {
            qDebug() << "AttenuationRatio: " << reinterpret_cast<InjectedAudioStream*>(streamToAdd)->getAttenuationRatio();
        }
    }

    if (showDebug) {
        qDebug() << "distance: " << distanceBetween;
    }

    glm::quat inverseOrientation = glm::inverse(listeningNodeStream->getOrientation());

    if (!sourceIsSelf && (streamToAdd->getType() == PositionalAudioStream::Microphone)) {
        //  source is another avatar, apply fixed off-axis attenuation to make them quieter as they turn away from listener
        glm::vec3 rotatedListenerPosition = glm::inverse(streamToAdd->getOrientation()) * relativePosition;

        float angleOfDelivery = glm::angle(glm::vec3(0.0f, 0.0f, -1.0f),
                                           glm::normalize(rotatedListenerPosition));

        const float MAX_OFF_AXIS_ATTENUATION = 0.2f;
        const float OFF_AXIS_ATTENUATION_FORMULA_STEP = (1 - MAX_OFF_AXIS_ATTENUATION) / 2.0f;

        float offAxisCoefficient = MAX_OFF_AXIS_ATTENUATION +
                                    (OFF_AXIS_ATTENUATION_FORMULA_STEP * (angleOfDelivery / PI_OVER_TWO));

        if (showDebug) {
            qDebug() << "angleOfDelivery" << angleOfDelivery << "offAxisCoefficient: " << offAxisCoefficient;

        }
        // multiply the current attenuation coefficient by the calculated off axis coefficient

        attenuationCoefficient *= offAxisCoefficient;
    }

    float attenuationPerDoublingInDistance = _attenuationPerDoublingInDistance;
    for (int i = 0; i < _zonesSettings.length(); ++i) {
        if (_audioZones[_zonesSettings[i].source].contains(streamToAdd->getPosition()) &&
            _audioZones[_zonesSettings[i].listener].contains(listeningNodeStream->getPosition())) {
            attenuationPerDoublingInDistance = _zonesSettings[i].coefficient;
            break;
        }
    }

    if (distanceBetween >= ATTENUATION_BEGINS_AT_DISTANCE) {
        // calculate the distance coefficient using the distance to this node
        float distanceCoefficient = 1 - (logf(distanceBetween / ATTENUATION_BEGINS_AT_DISTANCE) / logf(2.0f)
                                         * attenuationPerDoublingInDistance);

        if (distanceCoefficient < 0) {
            distanceCoefficient = 0;
        }

        // multiply the current attenuation coefficient by the distance coefficient
        attenuationCoefficient *= distanceCoefficient;
        if (showDebug) {
            qDebug() << "distanceCoefficient: " << distanceCoefficient;
        }
    }

    if (!sourceIsSelf) {
        //  Compute sample delay for the two ears to create phase panning
        glm::vec3 rotatedSourcePosition = inverseOrientation * relativePosition;

        // project the rotated source position vector onto the XZ plane
        rotatedSourcePosition.y = 0.0f;

        // produce an oriented angle about the y-axis
        bearingRelativeAngleToSource = glm::orientedAngle(glm::vec3(0.0f, 0.0f, -1.0f),
                                                          glm::normalize(rotatedSourcePosition),
                                                          glm::vec3(0.0f, 1.0f, 0.0f));

        const float PHASE_AMPLITUDE_RATIO_AT_90 = 0.5;

        // figure out the number of samples of delay and the ratio of the amplitude
        // in the weak channel for audio spatialization
        float sinRatio = fabsf(sinf(bearingRelativeAngleToSource));
        numSamplesDelay = SAMPLE_PHASE_DELAY_AT_90 * sinRatio;
        weakChannelAmplitudeRatio = 1 - (PHASE_AMPLITUDE_RATIO_AT_90 * sinRatio);

        if (distanceBetween < RADIUS_OF_HEAD) {
            // Diminish phase panning if source would be inside head
            numSamplesDelay *= distanceBetween / RADIUS_OF_HEAD;
            weakChannelAmplitudeRatio += (PHASE_AMPLITUDE_RATIO_AT_90 * sinRatio) * distanceBetween / RADIUS_OF_HEAD;
        }
    }

    if (showDebug) {
        qDebug() << "attenuation: " << attenuationCoefficient;
        qDebug() << "bearingRelativeAngleToSource: " << bearingRelativeAngleToSource << " numSamplesDelay: " << numSamplesDelay;
    }

    AudioRingBuffer::ConstIterator streamPopOutput = streamToAdd->getLastPopOutput();

    if (!streamToAdd->isStereo()) {
        // this is a mono stream, which means it gets full attenuation and spatialization

        // we need to do several things in this process:
        //    1) convert from mono to stereo by copying each input sample into the left and right output samples
        //    2)
        //    2) apply an attenuation AND fade to all samples (left and right)
        //    3) based on the bearing relative angle to the source we will weaken and delay either the left or
        //       right channel of the input into the output
        //    4) because one of these channels is delayed, we will need to use historical samples from
        //       the input stream for that delayed channel

        // Mono input to stereo output (item 1 above)
        int OUTPUT_SAMPLES_PER_INPUT_SAMPLE = 2;
        int inputSampleCount = AudioConstants::NETWORK_FRAME_SAMPLES_STEREO / OUTPUT_SAMPLES_PER_INPUT_SAMPLE;
        int maxOutputIndex = AudioConstants::NETWORK_FRAME_SAMPLES_STEREO;

        // attenuation and fade applied to all samples (item 2 above)
        float attenuationAndFade = attenuationCoefficient * repeatedFrameFadeFactor;

        // determine which side is weak and delayed (item 3 above)
        bool rightSideWeakAndDelayed = (bearingRelativeAngleToSource > 0.0f);

        // since we're converting from mono to stereo, we'll use these two indices to step through
        // the output samples. we'll increment each index independently in the loop
        int leftDestinationIndex = 0;
        int rightDestinationIndex = 1;

        // One of our two channels will be delayed (determined below). We'll use this index to step
        // through filling in our output with the historical samples for the delayed channel.  (item 4 above)
        int delayedChannelHistoricalAudioOutputIndex;

        // All samples will be attenuated by at least this much
        float leftSideAttenuation = attenuationAndFade;
        float rightSideAttenuation = attenuationAndFade;

        // The weak/delayed channel will be attenuated by this additional amount
        float attenuationAndWeakChannelRatioAndFade = attenuationAndFade * weakChannelAmplitudeRatio;

        // Now, based on the determination of which side is weak and delayed, set up our true starting point
        // for our indexes, as well as the appropriate attenuation for each channel
        if (rightSideWeakAndDelayed) {
            delayedChannelHistoricalAudioOutputIndex = rightDestinationIndex;
            rightSideAttenuation = attenuationAndWeakChannelRatioAndFade;
            rightDestinationIndex += (numSamplesDelay * OUTPUT_SAMPLES_PER_INPUT_SAMPLE);
        } else {
            delayedChannelHistoricalAudioOutputIndex = leftDestinationIndex;
            leftSideAttenuation = attenuationAndWeakChannelRatioAndFade;
            leftDestinationIndex += (numSamplesDelay * OUTPUT_SAMPLES_PER_INPUT_SAMPLE);
        }

        // If there was a sample delay for this stream, we need to pull samples prior to the official start of the input
        // and stick those samples at the beginning of the output. We only need to loop through this for the weak/delayed
        // side, since the normal side is fully handled below. (item 4 above)
        if (numSamplesDelay > 0) {

            // TODO: delayStreamSourceSamples may be inside the last frame written if the ringbuffer is completely full
            // maybe make AudioRingBuffer have 1 extra frame in its buffer
            AudioRingBuffer::ConstIterator delayStreamSourceSamples = streamPopOutput - numSamplesDelay;

            for (int i = 0; i < numSamplesDelay; i++) {
                int16_t originalHistoricalSample = *delayStreamSourceSamples;

                _preMixSamples[delayedChannelHistoricalAudioOutputIndex] += originalHistoricalSample
                                                                                 * attenuationAndWeakChannelRatioAndFade;
                ++delayStreamSourceSamples; // move our input pointer
                delayedChannelHistoricalAudioOutputIndex += OUTPUT_SAMPLES_PER_INPUT_SAMPLE; // move our output sample
            }
        }

        // Here's where we copy the MONO input to the STEREO output, and account for delay and weak side attenuation
        for (int inputSample = 0; inputSample < inputSampleCount; inputSample++) {
            int16_t originalSample = streamPopOutput[inputSample];
            int16_t leftSideSample = originalSample * leftSideAttenuation;
            int16_t rightSideSample = originalSample * rightSideAttenuation;

            // since we might be delayed, don't write beyond our maxOutputIndex
            if (leftDestinationIndex <= maxOutputIndex) {
                _preMixSamples[leftDestinationIndex] += leftSideSample;
            }
            if (rightDestinationIndex <= maxOutputIndex) {
                _preMixSamples[rightDestinationIndex] += rightSideSample;
            }

            leftDestinationIndex += OUTPUT_SAMPLES_PER_INPUT_SAMPLE;
            rightDestinationIndex += OUTPUT_SAMPLES_PER_INPUT_SAMPLE;
        }

    } else {
        int stereoDivider = streamToAdd->isStereo() ? 1 : 2;

       float attenuationAndFade = attenuationCoefficient * repeatedFrameFadeFactor;

        for (int s = 0; s < AudioConstants::NETWORK_FRAME_SAMPLES_STEREO; s++) {
            _preMixSamples[s] = glm::clamp(_preMixSamples[s] + (int)(streamPopOutput[s / stereoDivider] * attenuationAndFade),
                                            AudioConstants::MIN_SAMPLE_VALUE,
                                           AudioConstants::MAX_SAMPLE_VALUE);
        }
    }

    if (!sourceIsSelf && _enableFilter && !streamToAdd->ignorePenumbraFilter()) {

        const float TWO_OVER_PI = 2.0f / PI;

        const float ZERO_DB = 1.0f;
        const float NEGATIVE_ONE_DB = 0.891f;
        const float NEGATIVE_THREE_DB = 0.708f;

        const float FILTER_GAIN_AT_0 = ZERO_DB; // source is in front
        const float FILTER_GAIN_AT_90 = NEGATIVE_ONE_DB; // source is incident to left or right ear
        const float FILTER_GAIN_AT_180 = NEGATIVE_THREE_DB; // source is behind

        const float FILTER_CUTOFF_FREQUENCY_HZ = 1000.0f;

        const float penumbraFilterFrequency = FILTER_CUTOFF_FREQUENCY_HZ; // constant frequency
        const float penumbraFilterSlope = NEGATIVE_THREE_DB; // constant slope

        float penumbraFilterGainL;
        float penumbraFilterGainR;

        // variable gain calculation broken down by quadrant
        if (-bearingRelativeAngleToSource < -PI_OVER_TWO && -bearingRelativeAngleToSource > -PI) {
            penumbraFilterGainL = TWO_OVER_PI *
                (FILTER_GAIN_AT_0 - FILTER_GAIN_AT_180) * (-bearingRelativeAngleToSource + PI_OVER_TWO) + FILTER_GAIN_AT_0;
            penumbraFilterGainR = TWO_OVER_PI *
                (FILTER_GAIN_AT_90 - FILTER_GAIN_AT_180) * (-bearingRelativeAngleToSource + PI_OVER_TWO) + FILTER_GAIN_AT_90;
        } else if (-bearingRelativeAngleToSource <= PI && -bearingRelativeAngleToSource > PI_OVER_TWO) {
            penumbraFilterGainL = TWO_OVER_PI *
                (FILTER_GAIN_AT_180 - FILTER_GAIN_AT_90) * (-bearingRelativeAngleToSource - PI) + FILTER_GAIN_AT_180;
            penumbraFilterGainR = TWO_OVER_PI *
                (FILTER_GAIN_AT_180 - FILTER_GAIN_AT_0) * (-bearingRelativeAngleToSource - PI) + FILTER_GAIN_AT_180;
        } else if (-bearingRelativeAngleToSource <= PI_OVER_TWO && -bearingRelativeAngleToSource > 0) {
            penumbraFilterGainL = TWO_OVER_PI *
                (FILTER_GAIN_AT_90 - FILTER_GAIN_AT_0) * (-bearingRelativeAngleToSource - PI_OVER_TWO) + FILTER_GAIN_AT_90;
            penumbraFilterGainR = FILTER_GAIN_AT_0;
        } else {
            penumbraFilterGainL = FILTER_GAIN_AT_0;
            penumbraFilterGainR =  TWO_OVER_PI *
                (FILTER_GAIN_AT_0 - FILTER_GAIN_AT_90) * (-bearingRelativeAngleToSource) + FILTER_GAIN_AT_0;
        }

        if (distanceBetween < RADIUS_OF_HEAD) {
            // Diminish effect if source would be inside head
            penumbraFilterGainL += (1.0f - penumbraFilterGainL) * (1.0f - distanceBetween / RADIUS_OF_HEAD);
            penumbraFilterGainR += (1.0f - penumbraFilterGainR) * (1.0f - distanceBetween / RADIUS_OF_HEAD);
        }

        bool wantDebug = false;
        if (wantDebug) {
            qDebug() << "gainL=" << penumbraFilterGainL
                     << "gainR=" << penumbraFilterGainR
                     << "angle=" << -bearingRelativeAngleToSource;
        }

        // Get our per listener/source data so we can get our filter
        AudioFilterHSF1s& penumbraFilter = listenerNodeData->getListenerSourcePairData(streamUUID)->getPenumbraFilter();

        // set the gain on both filter channels
        penumbraFilter.setParameters(0, 0, AudioConstants::SAMPLE_RATE, penumbraFilterFrequency, penumbraFilterGainL, penumbraFilterSlope);
        penumbraFilter.setParameters(0, 1, AudioConstants::SAMPLE_RATE, penumbraFilterFrequency, penumbraFilterGainR, penumbraFilterSlope);
        penumbraFilter.render(_preMixSamples, _preMixSamples, AudioConstants::NETWORK_FRAME_SAMPLES_STEREO / 2);
    }

    // Actually mix the _preMixSamples into the _mixSamples here.
    for (int s = 0; s < AudioConstants::NETWORK_FRAME_SAMPLES_STEREO; s++) {
        _mixSamples[s] = glm::clamp(_mixSamples[s] + _preMixSamples[s], AudioConstants::MIN_SAMPLE_VALUE,
                                    AudioConstants::MAX_SAMPLE_VALUE);
    }

    return 1;
}

int AudioMixer::prepareMixForListeningNode(Node* node) {
    AvatarAudioStream* nodeAudioStream = static_cast<AudioMixerClientData*>(node->getLinkedData())->getAvatarAudioStream();
    AudioMixerClientData* listenerNodeData = static_cast<AudioMixerClientData*>(node->getLinkedData());

    // zero out the client mix for this node
    memset(_preMixSamples, 0, sizeof(_preMixSamples));
    memset(_mixSamples, 0, sizeof(_mixSamples));

    // loop through all other nodes that have sufficient audio to mix
    int streamsMixed = 0;

    DependencyManager::get<NodeList>()->eachNode([&](const SharedNodePointer& otherNode){
        if (otherNode->getLinkedData()) {
            AudioMixerClientData* otherNodeClientData = (AudioMixerClientData*) otherNode->getLinkedData();

            // enumerate the ARBs attached to the otherNode and add all that should be added to mix

            const QHash<QUuid, PositionalAudioStream*>& otherNodeAudioStreams = otherNodeClientData->getAudioStreams();
            QHash<QUuid, PositionalAudioStream*>::ConstIterator i;
            for (i = otherNodeAudioStreams.constBegin(); i != otherNodeAudioStreams.constEnd(); i++) {
                PositionalAudioStream* otherNodeStream = i.value();
                QUuid streamUUID = i.key();

                if (otherNodeStream->getType() == PositionalAudioStream::Microphone) {
                    streamUUID = otherNode->getUUID();
                }

                if (*otherNode != *node || otherNodeStream->shouldLoopbackForNode()) {
                    streamsMixed += addStreamToMixForListeningNodeWithStream(listenerNodeData, streamUUID,
                                                                             otherNodeStream, nodeAudioStream);
                }
            }
        }
    });

    return streamsMixed;
}

void AudioMixer::sendAudioEnvironmentPacket(SharedNodePointer node) {
    // Send stream properties
    bool hasReverb = false;
    float reverbTime, wetLevel;
    // find reverb properties
    for (int i = 0; i < _zoneReverbSettings.size(); ++i) {
        AudioMixerClientData* data = static_cast<AudioMixerClientData*>(node->getLinkedData());
        glm::vec3 streamPosition = data->getAvatarAudioStream()->getPosition();
        AABox box = _audioZones[_zoneReverbSettings[i].zone];
        if (box.contains(streamPosition)) {
            hasReverb = true;
            reverbTime = _zoneReverbSettings[i].reverbTime;
            wetLevel = _zoneReverbSettings[i].wetLevel;

            // Modulate wet level with distance to wall
            float MIN_ATTENUATION_DISTANCE = 2.0f;
            float MAX_ATTENUATION = -12; // dB
            glm::vec3 distanceToWalls = (box.getDimensions() / 2.0f) - glm::abs(streamPosition - box.calcCenter());
            float distanceToClosestWall = glm::min(distanceToWalls.x, distanceToWalls.z);
            if (distanceToClosestWall < MIN_ATTENUATION_DISTANCE) {
                wetLevel += MAX_ATTENUATION * (1.0f - distanceToClosestWall / MIN_ATTENUATION_DISTANCE);
            }
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

void AudioMixer::handleNodeAudioPacket(QSharedPointer<NLPacket> packet, SharedNodePointer sendingNode) {
    DependencyManager::get<NodeList>()->updateNodeWithDataFromPacket(packet, sendingNode);
}

void AudioMixer::handleMuteEnvironmentPacket(QSharedPointer<NLPacket> packet, SharedNodePointer sendingNode) {
    auto nodeList = DependencyManager::get<NodeList>();
    
    if (sendingNode->getCanAdjustLocks()) {
        auto newPacket = NLPacket::create(PacketType::MuteEnvironment, packet->getPayloadSize());
        // Copy payload
        newPacket->write(packet->getPayload(), packet->getPayloadSize());

        nodeList->eachNode([&](const SharedNodePointer& node){
            if (node->getType() == NodeType::Agent && node->getActiveSocket() &&
                node->getLinkedData() && node != sendingNode) {
                nodeList->sendPacket(std::move(newPacket), *node);
            }
        });
    }
}

void AudioMixer::sendStatsPacket() {
    static QJsonObject statsObject;

    statsObject["useDynamicJitterBuffers"] = _streamSettings._dynamicJitterBuffers;
    statsObject["trailing_sleep_percentage"] = _trailingSleepRatio * 100.0f;
    statsObject["performance_throttling_ratio"] = _performanceThrottlingRatio;

    statsObject["average_listeners_per_frame"] = (float) _sumListeners / (float) _numStatFrames;

    if (_sumListeners > 0) {
        statsObject["average_mixes_per_listener"] = (float) _sumMixes / (float) _sumListeners;
    } else {
        statsObject["average_mixes_per_listener"] = 0.0;
    }

    _sumListeners = 0;
    _sumMixes = 0;
    _numStatFrames = 0;

    QJsonObject readPendingDatagramStats;

    QJsonObject rpdCallsStats;
    rpdCallsStats["calls_per_sec_avg_30s"] = _readPendingCallsPerSecondStats.getWindowAverage();
    rpdCallsStats["calls_last_sec"] = _readPendingCallsPerSecondStats.getLastCompleteIntervalStats().getSum() + 0.5;

    readPendingDatagramStats["calls"] = rpdCallsStats;

    QJsonObject packetsPerCallStats;
    packetsPerCallStats["avg_30s"] = _datagramsReadPerCallStats.getWindowAverage();
    packetsPerCallStats["avg_1s"] = _datagramsReadPerCallStats.getLastCompleteIntervalStats().getAverage();

    readPendingDatagramStats["packets_per_call"] = packetsPerCallStats;

    QJsonObject packetsTimePerCallStats;
    packetsTimePerCallStats["usecs_per_call_avg_30s"] = _timeSpentPerCallStats.getWindowAverage();
    packetsTimePerCallStats["usecs_per_call_avg_1s"] = _timeSpentPerCallStats.getLastCompleteIntervalStats().getAverage();
    packetsTimePerCallStats["prct_time_in_call_30s"] =
        _timeSpentPerCallStats.getWindowSum() / (READ_DATAGRAMS_STATS_WINDOW_SECONDS * USECS_PER_SECOND) * 100.0;
    packetsTimePerCallStats["prct_time_in_call_1s"] =
        _timeSpentPerCallStats.getLastCompleteIntervalStats().getSum() / USECS_PER_SECOND * 100.0;

    readPendingDatagramStats["packets_time_per_call"] = packetsTimePerCallStats;

    QJsonObject hashMatchTimePerCallStats;
    hashMatchTimePerCallStats["usecs_per_hashmatch_avg_30s"] = _timeSpentPerHashMatchCallStats.getWindowAverage();
    hashMatchTimePerCallStats["usecs_per_hashmatch_avg_1s"]
        = _timeSpentPerHashMatchCallStats.getLastCompleteIntervalStats().getAverage();
    hashMatchTimePerCallStats["prct_time_in_hashmatch_30s"]
        = _timeSpentPerHashMatchCallStats.getWindowSum() / (READ_DATAGRAMS_STATS_WINDOW_SECONDS*USECS_PER_SECOND) * 100.0;
    hashMatchTimePerCallStats["prct_time_in_hashmatch_1s"]
        = _timeSpentPerHashMatchCallStats.getLastCompleteIntervalStats().getSum() / USECS_PER_SECOND * 100.0;
    readPendingDatagramStats["hashmatch_time_per_call"] = hashMatchTimePerCallStats;

    statsObject["read_pending_datagrams"] = readPendingDatagramStats;

    // add stats for each listerner
    auto nodeList = DependencyManager::get<NodeList>();
    QJsonObject listenerStats;

    nodeList->eachNode([&](const SharedNodePointer& node) {
        AudioMixerClientData* clientData = static_cast<AudioMixerClientData*>(node->getLinkedData());
        if (clientData) {
            QJsonObject nodeStats;
            QString uuidString = uuidStringWithoutCurlyBraces(node->getUUID());

            nodeStats["outbound_kbps"] = node->getOutboundBandwidth();
            nodeStats[USERNAME_UUID_REPLACEMENT_STATS_KEY] = uuidString;

            nodeStats["jitter"] = clientData->getAudioStreamStats();

            listenerStats[uuidString] = nodeStats;
        }
    });

    // add the listeners object to the root object
    statsObject["listeners"] = listenerStats;

    // send off the stats packets
    ThreadedAssignment::addPacketStatsAndSendStatsPacket(statsObject);
}

void AudioMixer::run() {

    ThreadedAssignment::commonInit(AUDIO_MIXER_LOGGING_TARGET_NAME, NodeType::AudioMixer);

    auto nodeList = DependencyManager::get<NodeList>();

    nodeList->addNodeTypeToInterestSet(NodeType::Agent);

    nodeList->linkedDataCreateCallback = [](Node* node) {
        node->setLinkedData(new AudioMixerClientData());
    };

    // wait until we have the domain-server settings, otherwise we bail
    DomainHandler& domainHandler = nodeList->getDomainHandler();

    qDebug() << "Waiting for domain settings from domain-server.";

    // block until we get the settingsRequestComplete signal
    QEventLoop loop;
    connect(&domainHandler, &DomainHandler::settingsReceived, &loop, &QEventLoop::quit);
    connect(&domainHandler, &DomainHandler::settingsReceiveFail, &loop, &QEventLoop::quit);
    domainHandler.requestDomainSettings();
    loop.exec();
    
    if (domainHandler.getSettingsObject().isEmpty()) {
        qDebug() << "Failed to retreive settings object from domain-server. Bailing on assignment.";
        setFinished(true);
        return;
    }

    const QJsonObject& settingsObject = domainHandler.getSettingsObject();

    // check the settings object to see if we have anything we can parse out
    parseSettingsObject(settingsObject);

    int nextFrame = 0;
    QElapsedTimer timer;
    timer.start();

    int usecToSleep = AudioConstants::NETWORK_FRAME_USECS;

    const int TRAILING_AVERAGE_FRAMES = 100;
    int framesSinceCutoffEvent = TRAILING_AVERAGE_FRAMES;

    while (!_isFinished) {
        const float STRUGGLE_TRIGGER_SLEEP_PERCENTAGE_THRESHOLD = 0.10f;
        const float BACK_OFF_TRIGGER_SLEEP_PERCENTAGE_THRESHOLD = 0.20f;

        const float RATIO_BACK_OFF = 0.02f;

        const float CURRENT_FRAME_RATIO = 1.0f / TRAILING_AVERAGE_FRAMES;
        const float PREVIOUS_FRAMES_RATIO = 1.0f - CURRENT_FRAME_RATIO;

        if (usecToSleep < 0) {
            usecToSleep = 0;
        }

        _trailingSleepRatio = (PREVIOUS_FRAMES_RATIO * _trailingSleepRatio)
            + (usecToSleep * CURRENT_FRAME_RATIO / (float) AudioConstants::NETWORK_FRAME_USECS);

        float lastCutoffRatio = _performanceThrottlingRatio;
        bool hasRatioChanged = false;

        if (framesSinceCutoffEvent >= TRAILING_AVERAGE_FRAMES) {
            if (_trailingSleepRatio <= STRUGGLE_TRIGGER_SLEEP_PERCENTAGE_THRESHOLD) {
                // we're struggling - change our min required loudness to reduce some load
                _performanceThrottlingRatio = _performanceThrottlingRatio + (0.5f * (1.0f - _performanceThrottlingRatio));

                qDebug() << "Mixer is struggling, sleeping" << _trailingSleepRatio * 100 << "% of frame time. Old cutoff was"
                    << lastCutoffRatio << "and is now" << _performanceThrottlingRatio;
                hasRatioChanged = true;
            } else if (_trailingSleepRatio >= BACK_OFF_TRIGGER_SLEEP_PERCENTAGE_THRESHOLD && _performanceThrottlingRatio != 0) {
                // we've recovered and can back off the required loudness
                _performanceThrottlingRatio = _performanceThrottlingRatio - RATIO_BACK_OFF;

                if (_performanceThrottlingRatio < 0) {
                    _performanceThrottlingRatio = 0;
                }

                qDebug() << "Mixer is recovering, sleeping" << _trailingSleepRatio * 100 << "% of frame time. Old cutoff was"
                    << lastCutoffRatio << "and is now" << _performanceThrottlingRatio;
                hasRatioChanged = true;
            }

            if (hasRatioChanged) {
                // set out min audability threshold from the new ratio
                _minAudibilityThreshold = LOUDNESS_TO_DISTANCE_RATIO / (2.0f * (1.0f - _performanceThrottlingRatio));
                qDebug() << "Minimum audability required to be mixed is now" << _minAudibilityThreshold;

                framesSinceCutoffEvent = 0;
            }
        }

        if (!hasRatioChanged) {
            ++framesSinceCutoffEvent;
        }

        quint64 now = usecTimestampNow();
        if (now - _lastPerSecondCallbackTime > USECS_PER_SECOND) {
            perSecondActions();
            _lastPerSecondCallbackTime = now;
        }

        nodeList->eachNode([&](const SharedNodePointer& node) {

            if (node->getLinkedData()) {
                AudioMixerClientData* nodeData = (AudioMixerClientData*)node->getLinkedData();

                // this function will attempt to pop a frame from each audio stream.
                // a pointer to the popped data is stored as a member in InboundAudioStream.
                // That's how the popped audio data will be read for mixing (but only if the pop was successful)
                nodeData->checkBuffersBeforeFrameSend();

                // if the stream should be muted, send mute packet
                if (nodeData->getAvatarAudioStream()
                    && shouldMute(nodeData->getAvatarAudioStream()->getQuietestFrameLoudness())) {
                    auto mutePacket = NLPacket::create(PacketType::NoisyMute, 0);
                    nodeList->sendPacket(std::move(mutePacket), *node);
                }

                if (node->getType() == NodeType::Agent && node->getActiveSocket()
                    && nodeData->getAvatarAudioStream()) {

                    int streamsMixed = prepareMixForListeningNode(node.data());

                    std::unique_ptr<NLPacket> mixPacket;

                    if (streamsMixed > 0) {
                        int mixPacketBytes = sizeof(quint16) + AudioConstants::NETWORK_FRAME_BYTES_STEREO;
                        mixPacket = NLPacket::create(PacketType::MixedAudio, mixPacketBytes);

                        // pack sequence number
                        quint16 sequence = nodeData->getOutgoingSequenceNumber();
                        mixPacket->writePrimitive(sequence);

                        // pack mixed audio samples
                        mixPacket->write(reinterpret_cast<char*>(_mixSamples),
                                         AudioConstants::NETWORK_FRAME_BYTES_STEREO);
                    } else {
                        int silentPacketBytes = sizeof(quint16) + sizeof(quint16);
                        mixPacket = NLPacket::create(PacketType::SilentAudioFrame, silentPacketBytes);

                        // pack sequence number
                        quint16 sequence = nodeData->getOutgoingSequenceNumber();
                        mixPacket->writePrimitive(sequence);

                        // pack number of silent audio samples
                        quint16 numSilentSamples = AudioConstants::NETWORK_FRAME_SAMPLES_STEREO;
                        mixPacket->writePrimitive(numSilentSamples);
                    }

                    // Send audio environment
                    sendAudioEnvironmentPacket(node);

                    // send mixed audio packet
                    nodeList->sendPacket(std::move(mixPacket), *node);
                    nodeData->incrementOutgoingMixedAudioSequenceNumber();

                    // send an audio stream stats packet if it's time
                    if (_sendAudioStreamStats) {
                        nodeData->sendAudioStreamStatsPackets(node);
                        _sendAudioStreamStats = false;
                    }

                    ++_sumListeners;
                }
            }
        });

        ++_numStatFrames;

        // since we're a while loop we need to help Qt's event processing
        QCoreApplication::processEvents();

        if (_isFinished) {
            // at this point the audio-mixer is done
            // check if we have a deferred delete event to process (which we should once finished)
            QCoreApplication::sendPostedEvents(this, QEvent::DeferredDelete);
            break;
        }

        usecToSleep = (++nextFrame * AudioConstants::NETWORK_FRAME_USECS) - timer.nsecsElapsed() / 1000; // ns to us

        if (usecToSleep > 0) {
            usleep(usecToSleep);
        }
    }
}

void AudioMixer::perSecondActions() {
    _sendAudioStreamStats = true;

    int callsLastSecond = _datagramsReadPerCallStats.getCurrentIntervalSamples();
    _readPendingCallsPerSecondStats.update(callsLastSecond);

    if (_printStreamStats) {

        printf("\n================================================================================\n\n");

        printf("            readPendingDatagram() calls per second | avg: %.2f, avg_30s: %.2f, last_second: %d\n",
            _readPendingCallsPerSecondStats.getAverage(),
            _readPendingCallsPerSecondStats.getWindowAverage(),
            callsLastSecond);

        printf("                           Datagrams read per call | avg: %.2f, avg_30s: %.2f, last_second: %.2f\n",
            _datagramsReadPerCallStats.getAverage(),
            _datagramsReadPerCallStats.getWindowAverage(),
            _datagramsReadPerCallStats.getCurrentIntervalAverage());

        printf("        Usecs spent per readPendingDatagram() call | avg: %.2f, avg_30s: %.2f, last_second: %.2f\n",
            _timeSpentPerCallStats.getAverage(),
            _timeSpentPerCallStats.getWindowAverage(),
            _timeSpentPerCallStats.getCurrentIntervalAverage());

        printf("  Usecs spent per packetVersionAndHashMatch() call | avg: %.2f, avg_30s: %.2f, last_second: %.2f\n",
            _timeSpentPerHashMatchCallStats.getAverage(),
            _timeSpentPerHashMatchCallStats.getWindowAverage(),
            _timeSpentPerHashMatchCallStats.getCurrentIntervalAverage());

        double WINDOW_LENGTH_USECS = READ_DATAGRAMS_STATS_WINDOW_SECONDS * USECS_PER_SECOND;

        printf("       %% time spent in readPendingDatagram() calls | avg_30s: %.6f%%, last_second: %.6f%%\n",
            _timeSpentPerCallStats.getWindowSum() / WINDOW_LENGTH_USECS * 100.0,
            _timeSpentPerCallStats.getCurrentIntervalSum() / USECS_PER_SECOND * 100.0);

        printf("%% time spent in packetVersionAndHashMatch() calls: | avg_30s: %.6f%%, last_second: %.6f%%\n",
            _timeSpentPerHashMatchCallStats.getWindowSum() / WINDOW_LENGTH_USECS * 100.0,
            _timeSpentPerHashMatchCallStats.getCurrentIntervalSum() / USECS_PER_SECOND * 100.0);

        DependencyManager::get<NodeList>()->eachNode([](const SharedNodePointer& node) {
            if (node->getLinkedData()) {
                AudioMixerClientData* nodeData = (AudioMixerClientData*)node->getLinkedData();

                if (node->getType() == NodeType::Agent && node->getActiveSocket()) {
                    printf("\nStats for agent %s --------------------------------\n",
                        node->getUUID().toString().toLatin1().data());
                    nodeData->printUpstreamDownstreamStats();
                }
            }
        });
    }

    _datagramsReadPerCallStats.currentIntervalComplete();
    _timeSpentPerCallStats.currentIntervalComplete();
    _timeSpentPerHashMatchCallStats.currentIntervalComplete();
}

void AudioMixer::parseSettingsObject(const QJsonObject &settingsObject) {
    if (settingsObject.contains(AUDIO_BUFFER_GROUP_KEY)) {
        QJsonObject audioBufferGroupObject = settingsObject[AUDIO_BUFFER_GROUP_KEY].toObject();

        // check the payload to see if we have asked for dynamicJitterBuffer support
        const QString DYNAMIC_JITTER_BUFFER_JSON_KEY = "dynamic_jitter_buffer";
        _streamSettings._dynamicJitterBuffers = audioBufferGroupObject[DYNAMIC_JITTER_BUFFER_JSON_KEY].toBool();
        if (_streamSettings._dynamicJitterBuffers) {
            qDebug() << "Enable dynamic jitter buffers.";
        } else {
            qDebug() << "Dynamic jitter buffers disabled.";
        }

        bool ok;
        const QString DESIRED_JITTER_BUFFER_FRAMES_KEY = "static_desired_jitter_buffer_frames";
        _streamSettings._staticDesiredJitterBufferFrames = audioBufferGroupObject[DESIRED_JITTER_BUFFER_FRAMES_KEY].toString().toInt(&ok);
        if (!ok) {
            _streamSettings._staticDesiredJitterBufferFrames = DEFAULT_STATIC_DESIRED_JITTER_BUFFER_FRAMES;
        }
        qDebug() << "Static desired jitter buffer frames:" << _streamSettings._staticDesiredJitterBufferFrames;

        const QString MAX_FRAMES_OVER_DESIRED_JSON_KEY = "max_frames_over_desired";
        _streamSettings._maxFramesOverDesired = audioBufferGroupObject[MAX_FRAMES_OVER_DESIRED_JSON_KEY].toString().toInt(&ok);
        if (!ok) {
            _streamSettings._maxFramesOverDesired = DEFAULT_MAX_FRAMES_OVER_DESIRED;
        }
        qDebug() << "Max frames over desired:" << _streamSettings._maxFramesOverDesired;

        const QString USE_STDEV_FOR_DESIRED_CALC_JSON_KEY = "use_stdev_for_desired_calc";
        _streamSettings._useStDevForJitterCalc = audioBufferGroupObject[USE_STDEV_FOR_DESIRED_CALC_JSON_KEY].toBool();
        if (_streamSettings._useStDevForJitterCalc) {
            qDebug() << "Using stdev method for jitter calc if dynamic jitter buffers enabled";
        } else {
            qDebug() << "Using max-gap method for jitter calc if dynamic jitter buffers enabled";
        }

        const QString WINDOW_STARVE_THRESHOLD_JSON_KEY = "window_starve_threshold";
        _streamSettings._windowStarveThreshold = audioBufferGroupObject[WINDOW_STARVE_THRESHOLD_JSON_KEY].toString().toInt(&ok);
        if (!ok) {
            _streamSettings._windowStarveThreshold = DEFAULT_WINDOW_STARVE_THRESHOLD;
        }
        qDebug() << "Window A starve threshold:" << _streamSettings._windowStarveThreshold;

        const QString WINDOW_SECONDS_FOR_DESIRED_CALC_ON_TOO_MANY_STARVES_JSON_KEY = "window_seconds_for_desired_calc_on_too_many_starves";
        _streamSettings._windowSecondsForDesiredCalcOnTooManyStarves = audioBufferGroupObject[WINDOW_SECONDS_FOR_DESIRED_CALC_ON_TOO_MANY_STARVES_JSON_KEY].toString().toInt(&ok);
        if (!ok) {
            _streamSettings._windowSecondsForDesiredCalcOnTooManyStarves = DEFAULT_WINDOW_SECONDS_FOR_DESIRED_CALC_ON_TOO_MANY_STARVES;
        }
        qDebug() << "Window A length:" << _streamSettings._windowSecondsForDesiredCalcOnTooManyStarves << "seconds";

        const QString WINDOW_SECONDS_FOR_DESIRED_REDUCTION_JSON_KEY = "window_seconds_for_desired_reduction";
        _streamSettings._windowSecondsForDesiredReduction = audioBufferGroupObject[WINDOW_SECONDS_FOR_DESIRED_REDUCTION_JSON_KEY].toString().toInt(&ok);
        if (!ok) {
            _streamSettings._windowSecondsForDesiredReduction = DEFAULT_WINDOW_SECONDS_FOR_DESIRED_REDUCTION;
        }
        qDebug() << "Window B length:" << _streamSettings._windowSecondsForDesiredReduction << "seconds";

        const QString REPETITION_WITH_FADE_JSON_KEY = "repetition_with_fade";
        _streamSettings._repetitionWithFade = audioBufferGroupObject[REPETITION_WITH_FADE_JSON_KEY].toBool();
        if (_streamSettings._repetitionWithFade) {
            qDebug() << "Repetition with fade enabled";
        } else {
            qDebug() << "Repetition with fade disabled";
        }

        const QString PRINT_STREAM_STATS_JSON_KEY = "print_stream_stats";
        _printStreamStats = audioBufferGroupObject[PRINT_STREAM_STATS_JSON_KEY].toBool();
        if (_printStreamStats) {
            qDebug() << "Stream stats will be printed to stdout";
        }
    }

    if (settingsObject.contains(AUDIO_ENV_GROUP_KEY)) {
        QJsonObject audioEnvGroupObject = settingsObject[AUDIO_ENV_GROUP_KEY].toObject();

        const QString ATTENATION_PER_DOULING_IN_DISTANCE = "attenuation_per_doubling_in_distance";
        if (audioEnvGroupObject[ATTENATION_PER_DOULING_IN_DISTANCE].isString()) {
            bool ok = false;
            float attenuation = audioEnvGroupObject[ATTENATION_PER_DOULING_IN_DISTANCE].toString().toFloat(&ok);
            if (ok) {
                _attenuationPerDoublingInDistance = attenuation;
                qDebug() << "Attenuation per doubling in distance changed to" << _attenuationPerDoublingInDistance;
            }
        }

        const QString NOISE_MUTING_THRESHOLD = "noise_muting_threshold";
        if (audioEnvGroupObject[NOISE_MUTING_THRESHOLD].isString()) {
            bool ok = false;
            float noiseMutingThreshold = audioEnvGroupObject[NOISE_MUTING_THRESHOLD].toString().toFloat(&ok);
            if (ok) {
                _noiseMutingThreshold = noiseMutingThreshold;
                qDebug() << "Noise muting threshold changed to" << _noiseMutingThreshold;
            }
        }

        const QString FILTER_KEY = "enable_filter";
        if (audioEnvGroupObject[FILTER_KEY].isBool()) {
            _enableFilter = audioEnvGroupObject[FILTER_KEY].toBool();
        }
        if (_enableFilter) {
            qDebug() << "Filter enabled";
        }

        const QString AUDIO_ZONES = "zones";
        if (audioEnvGroupObject[AUDIO_ZONES].isObject()) {
            const QJsonObject& zones = audioEnvGroupObject[AUDIO_ZONES].toObject();

            const QString X_RANGE = "x_range";
            const QString Y_RANGE = "y_range";
            const QString Z_RANGE = "z_range";
            foreach (const QString& zone, zones.keys()) {
                QJsonObject zoneObject = zones[zone].toObject();

                if (zoneObject.contains(X_RANGE) && zoneObject.contains(Y_RANGE) && zoneObject.contains(Z_RANGE)) {
                    QStringList xRange = zoneObject.value(X_RANGE).toString().split("-", QString::SkipEmptyParts);
                    QStringList yRange = zoneObject.value(Y_RANGE).toString().split("-", QString::SkipEmptyParts);
                    QStringList zRange = zoneObject.value(Z_RANGE).toString().split("-", QString::SkipEmptyParts);

                    if (xRange.size() == 2 && yRange.size() == 2 && zRange.size() == 2) {
                        float xMin, xMax, yMin, yMax, zMin, zMax;
                        bool ok, allOk = true;
                        xMin = xRange[0].toFloat(&ok);
                        allOk &= ok;
                        xMax = xRange[1].toFloat(&ok);
                        allOk &= ok;
                        yMin = yRange[0].toFloat(&ok);
                        allOk &= ok;
                        yMax = yRange[1].toFloat(&ok);
                        allOk &= ok;
                        zMin = zRange[0].toFloat(&ok);
                        allOk &= ok;
                        zMax = zRange[1].toFloat(&ok);
                        allOk &= ok;

                        if (allOk) {
                            glm::vec3 corner(xMin, yMin, zMin);
                            glm::vec3 dimensions(xMax - xMin, yMax - yMin, zMax - zMin);
                            AABox zoneAABox(corner, dimensions);
                            _audioZones.insert(zone, zoneAABox);
                            qDebug() << "Added zone:" << zone << "(corner:" << corner
                                     << ", dimensions:" << dimensions << ")";
                        }
                    }
                }
            }
        }

        const QString ATTENUATION_COEFFICIENTS = "attenuation_coefficients";
        if (audioEnvGroupObject[ATTENUATION_COEFFICIENTS].isArray()) {
            const QJsonArray& coefficients = audioEnvGroupObject[ATTENUATION_COEFFICIENTS].toArray();

            const QString SOURCE = "source";
            const QString LISTENER = "listener";
            const QString COEFFICIENT = "coefficient";
            for (int i = 0; i < coefficients.count(); ++i) {
                QJsonObject coefficientObject = coefficients[i].toObject();

                if (coefficientObject.contains(SOURCE) &&
                    coefficientObject.contains(LISTENER) &&
                    coefficientObject.contains(COEFFICIENT)) {

                    ZonesSettings settings;

                    bool ok;
                    settings.source = coefficientObject.value(SOURCE).toString();
                    settings.listener = coefficientObject.value(LISTENER).toString();
                    settings.coefficient = coefficientObject.value(COEFFICIENT).toString().toFloat(&ok);

                    if (ok && settings.coefficient >= 0.0f && settings.coefficient <= 1.0f &&
                        _audioZones.contains(settings.source) && _audioZones.contains(settings.listener)) {

                        _zonesSettings.push_back(settings);
                        qDebug() << "Added Coefficient:" << settings.source << settings.listener << settings.coefficient;
                    }
                }
            }
        }

        const QString REVERB = "reverb";
        if (audioEnvGroupObject[REVERB].isArray()) {
            const QJsonArray& reverb = audioEnvGroupObject[REVERB].toArray();

            const QString ZONE = "zone";
            const QString REVERB_TIME = "reverb_time";
            const QString WET_LEVEL = "wet_level";
            for (int i = 0; i < reverb.count(); ++i) {
                QJsonObject reverbObject = reverb[i].toObject();

                if (reverbObject.contains(ZONE) &&
                    reverbObject.contains(REVERB_TIME) &&
                    reverbObject.contains(WET_LEVEL)) {

                    bool okReverbTime, okWetLevel;
                    QString zone = reverbObject.value(ZONE).toString();
                    float reverbTime = reverbObject.value(REVERB_TIME).toString().toFloat(&okReverbTime);
                    float wetLevel = reverbObject.value(WET_LEVEL).toString().toFloat(&okWetLevel);

                    if (okReverbTime && okWetLevel && _audioZones.contains(zone)) {
                        ReverbSettings settings;
                        settings.zone = zone;
                        settings.reverbTime = reverbTime;
                        settings.wetLevel = wetLevel;

                        _zoneReverbSettings.push_back(settings);
                        qDebug() << "Added Reverb:" << zone << reverbTime << wetLevel;
                    }
                }
            }
        }
    }
}


