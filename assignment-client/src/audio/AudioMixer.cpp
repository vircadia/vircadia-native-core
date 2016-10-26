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
#include <memory>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>

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
#include <plugins/PluginManager.h>
#include <plugins/CodecPlugin.h>
#include <udt/PacketHeaders.h>
#include <SharedUtil.h>
#include <StDev.h>
#include <UUID.h>

#include "AudioRingBuffer.h"
#include "AudioMixerClientData.h"
#include "AvatarAudioStream.h"
#include "InjectedAudioStream.h"

#include "AudioMixer.h"

static const float LOUDNESS_TO_DISTANCE_RATIO = 0.00001f;
static const float DEFAULT_ATTENUATION_PER_DOUBLING_IN_DISTANCE = 0.5f;    // attenuation = -6dB * log2(distance)
static const float DEFAULT_NOISE_MUTING_THRESHOLD = 0.003f;
static const QString AUDIO_MIXER_LOGGING_TARGET_NAME = "audio-mixer";
static const QString AUDIO_ENV_GROUP_KEY = "audio_env";
static const QString AUDIO_BUFFER_GROUP_KEY = "audio_buffer";

int AudioMixer::_numStaticJitterFrames{ -1 };
bool AudioMixer::_enableFilter = true;

bool AudioMixer::shouldMute(float quietestFrame) {
    return (quietestFrame > _noiseMutingThreshold);
}

AudioMixer::AudioMixer(ReceivedMessage& message) :
    ThreadedAssignment(message),
    _trailingSleepRatio(1.0f),
    _minAudibilityThreshold(LOUDNESS_TO_DISTANCE_RATIO / 2.0f),
    _performanceThrottlingRatio(0.0f),
    _attenuationPerDoublingInDistance(DEFAULT_ATTENUATION_PER_DOUBLING_IN_DISTANCE),
    _noiseMutingThreshold(DEFAULT_NOISE_MUTING_THRESHOLD)
{
    auto nodeList = DependencyManager::get<NodeList>();
    auto& packetReceiver = nodeList->getPacketReceiver();

    packetReceiver.registerListenerForTypes({ PacketType::MicrophoneAudioNoEcho, PacketType::MicrophoneAudioWithEcho,
                                              PacketType::InjectAudio, PacketType::SilentAudioFrame,
                                              PacketType::AudioStreamStats },
                                            this, "handleNodeAudioPacket");
    packetReceiver.registerListener(PacketType::NegotiateAudioFormat, this, "handleNegotiateAudioFormat");
    packetReceiver.registerListener(PacketType::MuteEnvironment, this, "handleMuteEnvironmentPacket");
    packetReceiver.registerListener(PacketType::NodeIgnoreRequest, this, "handleNodeIgnoreRequestPacket");

    connect(nodeList.data(), &NodeList::nodeKilled, this, &AudioMixer::handleNodeKilled);
}

const float ATTENUATION_BEGINS_AT_DISTANCE = 1.0f;

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

float AudioMixer::gainForSource(const PositionalAudioStream& streamToAdd,
                                const AvatarAudioStream& listeningNodeStream, const glm::vec3& relativePosition, bool isEcho) {
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

    float attenuationPerDoublingInDistance = _attenuationPerDoublingInDistance;
    for (int i = 0; i < _zonesSettings.length(); ++i) {
        if (_audioZones[_zonesSettings[i].source].contains(streamToAdd.getPosition()) &&
            _audioZones[_zonesSettings[i].listener].contains(listeningNodeStream.getPosition())) {
            attenuationPerDoublingInDistance = _zonesSettings[i].coefficient;
            break;
        }
    }

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

float AudioMixer::azimuthForSource(const PositionalAudioStream& streamToAdd, const AvatarAudioStream& listeningNodeStream,
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

void AudioMixer::addStreamToMixForListeningNodeWithStream(AudioMixerClientData& listenerNodeData,
                                                          const PositionalAudioStream& streamToAdd,
                                                          const QUuid& sourceNodeID,
                                                          const AvatarAudioStream& listeningNodeStream) {


    // to reduce artifacts we calculate the gain and azimuth for every source for this listener
    // even if we are not going to end up mixing in this source

    ++_totalMixes;

    // this ensures that the tail of any previously mixed audio or the first block of new audio sounds correct

    // check if this is a server echo of a source back to itself
    bool isEcho = (&streamToAdd == &listeningNodeStream);

    glm::vec3 relativePosition = streamToAdd.getPosition() - listeningNodeStream.getPosition();

    // figure out the distance between source and listener
    float distance = glm::max(glm::length(relativePosition), EPSILON);

    // figure out the gain for this source at the listener
    float gain = gainForSource(streamToAdd, listeningNodeStream, relativePosition, isEcho);

    // figure out the azimuth to this source at the listener
    float azimuth = isEcho ? 0.0f : azimuthForSource(streamToAdd, listeningNodeStream, relativePosition);

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

                ++_hrtfSilentRenders;;
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

            ++_manualStereoMixes;
        } else {
            for (int i = 0; i < AudioConstants::NETWORK_FRAME_SAMPLES_STEREO; i += 2) {
                auto monoSample = float(streamPopOutput[i / 2] * gain / AudioConstants::MAX_SAMPLE_VALUE);
                _mixedSamples[i] += monoSample;
                _mixedSamples[i + 1] += monoSample;
            }

            ++_manualEchoMixes;
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

        ++_hrtfSilentRenders;

        return;
    }

    if (_performanceThrottlingRatio > 0.0f
        && streamToAdd.getLastPopOutputTrailingLoudness() / glm::length(relativePosition) <= _minAudibilityThreshold) {
        // the mixer is struggling so we're going to drop off some streams

        // we call renderSilent via the HRTF with the actual frame data and a gain of 0.0
        hrtf.renderSilent(streamBlock, _mixedSamples, HRTF_DATASET_INDEX, azimuth, distance, 0.0f,
                          AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

        ++_hrtfStruggleRenders;

        return;
    }

    ++_hrtfRenders;

    // mono stream, call the HRTF with our block and calculated azimuth and gain
    hrtf.render(streamBlock, _mixedSamples, HRTF_DATASET_INDEX, azimuth, distance, gain,
                AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);
}

bool AudioMixer::prepareMixForListeningNode(Node* node) {
    AvatarAudioStream* nodeAudioStream = static_cast<AudioMixerClientData*>(node->getLinkedData())->getAvatarAudioStream();
    AudioMixerClientData* listenerNodeData = static_cast<AudioMixerClientData*>(node->getLinkedData());

    // zero out the client mix for this node
    memset(_mixedSamples, 0, sizeof(_mixedSamples));

    // loop through all other nodes that have sufficient audio to mix

    DependencyManager::get<NodeList>()->eachNode([&](const SharedNodePointer& otherNode){
        // make sure that we have audio data for this other node and that it isn't being ignored by our listening node
        if (otherNode->getLinkedData() && !node->isIgnoringNodeWithID(otherNode->getUUID())) {
            AudioMixerClientData* otherNodeClientData = (AudioMixerClientData*) otherNode->getLinkedData();

            // enumerate the ARBs attached to the otherNode and add all that should be added to mix
            auto streamsCopy = otherNodeClientData->getAudioStreams();

            for (auto& streamPair : streamsCopy) {

                auto otherNodeStream = streamPair.second;

                if (*otherNode != *node || otherNodeStream->shouldLoopbackForNode()) {
                    addStreamToMixForListeningNodeWithStream(*listenerNodeData, *otherNodeStream, otherNode->getUUID(),
                                                             *nodeAudioStream);
                }
            }
        }
    });

    // use the per listner AudioLimiter to render the mixed data...
    listenerNodeData->audioLimiter.render(_mixedSamples, _clampedSamples, AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);

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

void AudioMixer::handleNodeAudioPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    getOrCreateClientData(sendingNode.data());
    DependencyManager::get<NodeList>()->updateNodeWithDataFromPacket(message, sendingNode);
}

void AudioMixer::handleMuteEnvironmentPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    auto nodeList = DependencyManager::get<NodeList>();

    if (sendingNode->isAllowedEditor()) {
        glm::vec3 position;
        float radius;

        auto newPacket = NLPacket::create(PacketType::MuteEnvironment, sizeof(position) + sizeof(radius));

        // read the position and radius from the sent packet
        message->readPrimitive(&position);
        message->readPrimitive(&radius);

        // write them to our packet
        newPacket->writePrimitive(position);
        newPacket->writePrimitive(radius);

        nodeList->eachNode([&](const SharedNodePointer& node){
            if (node->getType() == NodeType::Agent && node->getActiveSocket() &&
                node->getLinkedData() && node != sendingNode) {
                nodeList->sendUnreliablePacket(*newPacket, *node);
            }
        });
    }
}

DisplayPluginList getDisplayPlugins() {
    DisplayPluginList result;
    return result;
}

InputPluginList getInputPlugins() {
    InputPluginList result;
    return result;
}

void saveInputPluginSettings(const InputPluginList& plugins) {
}


void AudioMixer::handleNegotiateAudioFormat(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    QStringList availableCodecs;
    auto codecPlugins = PluginManager::getInstance()->getCodecPlugins();
    if (codecPlugins.size() > 0) {
        for (auto& plugin : codecPlugins) {
            auto codecName = plugin->getName();
            qDebug() << "Codec available:" << codecName;
            availableCodecs.append(codecName);
        }
    } else {
        qDebug() << "No Codecs available...";
    }

    CodecPluginPointer selectedCodec;
    QString selectedCodecName;

    QStringList codecPreferenceList = _codecPreferenceOrder.split(",");

    // read the codecs requested by the client
    const int MAX_PREFERENCE = 99999;
    int preferredCodecIndex = MAX_PREFERENCE;
    QString preferredCodec;
    quint8 numberOfCodecs = 0;
    message->readPrimitive(&numberOfCodecs);
    qDebug() << "numberOfCodecs:" << numberOfCodecs;
    QStringList codecList;
    for (quint16 i = 0; i < numberOfCodecs; i++) {
        QString requestedCodec = message->readString();
        int preferenceOfThisCodec = codecPreferenceList.indexOf(requestedCodec);
        bool codecAvailable = availableCodecs.contains(requestedCodec);
        qDebug() << "requestedCodec:" << requestedCodec << "preference:" << preferenceOfThisCodec << "available:" << codecAvailable;
        if (codecAvailable) {
            codecList.append(requestedCodec);
            if (preferenceOfThisCodec >= 0 && preferenceOfThisCodec < preferredCodecIndex) {
                qDebug() << "This codec is preferred...";
                selectedCodecName  = requestedCodec;
                preferredCodecIndex = preferenceOfThisCodec;
            }
        }
    }
    qDebug() << "all requested and available codecs:" << codecList;

    // choose first codec
    if (!selectedCodecName.isEmpty()) {
        if (codecPlugins.size() > 0) {
            for (auto& plugin : codecPlugins) {
                if (selectedCodecName == plugin->getName()) {
                    qDebug() << "Selecting codec:" << selectedCodecName;
                    selectedCodec = plugin;
                    break;
                }
            }
        }
    }

    auto clientData = getOrCreateClientData(sendingNode.data());
    clientData->setupCodec(selectedCodec, selectedCodecName);
    qDebug() << "selectedCodecName:" << selectedCodecName;
    clientData->sendSelectAudioFormat(sendingNode, selectedCodecName);
}

void AudioMixer::handleNodeKilled(SharedNodePointer killedNode) {
    // enumerate the connected listeners to remove HRTF objects for the disconnected node
    auto nodeList = DependencyManager::get<NodeList>();

    nodeList->eachNode([](const SharedNodePointer& node) {
        auto clientData = dynamic_cast<AudioMixerClientData*>(node->getLinkedData());
        if (clientData) {
            clientData->removeHRTFsForNode(node->getUUID());
        }
    });
}

void AudioMixer::handleNodeIgnoreRequestPacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer sendingNode) {
    sendingNode->parseIgnoreRequestMessage(packet);
}

void AudioMixer::removeHRTFsForFinishedInjector(const QUuid& streamID) {
    auto injectorClientData = qobject_cast<AudioMixerClientData*>(sender());
    if (injectorClientData) {
        // enumerate the connected listeners to remove HRTF objects for the disconnected injector
        auto nodeList = DependencyManager::get<NodeList>();

        nodeList->eachNode([injectorClientData, &streamID](const SharedNodePointer& node){
            auto listenerClientData = dynamic_cast<AudioMixerClientData*>(node->getLinkedData());
            if (listenerClientData) {
                listenerClientData->removeHRTFForStream(injectorClientData->getNodeID(), streamID);
            }
        });
    }
}

QString AudioMixer::percentageForMixStats(int counter) {
    if (_totalMixes > 0) {
        float mixPercentage = (float(counter) / _totalMixes) * 100.0f;
        return QString::number(mixPercentage, 'f', 2);
    } else {
        return QString("0.0");
    }
}

void AudioMixer::sendStatsPacket() {
    QJsonObject statsObject;

    if (_numStatFrames == 0) {
        return;
    }

    statsObject["useDynamicJitterBuffers"] = _numStaticJitterFrames == -1;
    statsObject["trailing_sleep_percentage"] = _trailingSleepRatio * 100.0f;
    statsObject["performance_throttling_ratio"] = _performanceThrottlingRatio;

    statsObject["avg_listeners_per_frame"] = (float) _sumListeners / (float) _numStatFrames;

    QJsonObject mixStats;
    mixStats["%_hrtf_mixes"] = percentageForMixStats(_hrtfRenders);
    mixStats["%_hrtf_silent_mixes"] = percentageForMixStats(_hrtfSilentRenders);
    mixStats["%_hrtf_struggle_mixes"] = percentageForMixStats(_hrtfStruggleRenders);
    mixStats["%_manual_stereo_mixes"] = percentageForMixStats(_manualStereoMixes);
    mixStats["%_manual_echo_mixes"] = percentageForMixStats(_manualEchoMixes);

    mixStats["total_mixes"] = _totalMixes;
    mixStats["avg_mixes_per_block"] = _totalMixes / _numStatFrames;

    statsObject["mix_stats"] = mixStats;

    _sumListeners = 0;
    _hrtfRenders = 0;
    _hrtfSilentRenders = 0;
    _hrtfStruggleRenders = 0;
    _manualStereoMixes = 0;
    _manualEchoMixes = 0;
    _totalMixes = 0;
    _numStatFrames = 0;

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
    statsObject["z_listeners"] = listenerStats;

    // send off the stats packets
    ThreadedAssignment::addPacketStatsAndSendStatsPacket(statsObject);
}

void AudioMixer::run() {

    qDebug() << "Waiting for connection to domain to request settings from domain-server.";

    // wait until we have the domain-server settings, otherwise we bail
    DomainHandler& domainHandler = DependencyManager::get<NodeList>()->getDomainHandler();
    connect(&domainHandler, &DomainHandler::settingsReceived, this, &AudioMixer::domainSettingsRequestComplete);
    connect(&domainHandler, &DomainHandler::settingsReceiveFail, this, &AudioMixer::domainSettingsRequestFailed);

    ThreadedAssignment::commonInit(AUDIO_MIXER_LOGGING_TARGET_NAME, NodeType::AudioMixer);
}

AudioMixerClientData* AudioMixer::getOrCreateClientData(Node* node) {
    auto clientData = dynamic_cast<AudioMixerClientData*>(node->getLinkedData());

    if (!clientData) {
        node->setLinkedData(std::unique_ptr<NodeData> { new AudioMixerClientData(node->getUUID()) });
        clientData = dynamic_cast<AudioMixerClientData*>(node->getLinkedData());
        connect(clientData, &AudioMixerClientData::injectorStreamFinished, this, &AudioMixer::removeHRTFsForFinishedInjector);
    }

    return clientData;
}

void AudioMixer::domainSettingsRequestComplete() {
    auto nodeList = DependencyManager::get<NodeList>();

    nodeList->addNodeTypeToInterestSet(NodeType::Agent);

    nodeList->linkedDataCreateCallback = [&](Node* node) { getOrCreateClientData(node); };

    DomainHandler& domainHandler = nodeList->getDomainHandler();
    const QJsonObject& settingsObject = domainHandler.getSettingsObject();

    // check the settings object to see if we have anything we can parse out
    parseSettingsObject(settingsObject);

    // queue up a connection to start broadcasting mixes now that we're ready to go
    QMetaObject::invokeMethod(this, "broadcastMixes", Qt::QueuedConnection);
}

void AudioMixer::broadcastMixes() {
    const int TRAILING_AVERAGE_FRAMES = 100;
    const float CURRENT_FRAME_RATIO = 1.0f / TRAILING_AVERAGE_FRAMES;
    const float PREVIOUS_FRAMES_RATIO = 1.0f - CURRENT_FRAME_RATIO;

    const float STRUGGLE_TRIGGER_SLEEP_PERCENTAGE_THRESHOLD = 0.10f;
    const float BACK_OFF_TRIGGER_SLEEP_PERCENTAGE_THRESHOLD = 0.20f;

    const float RATIO_BACK_OFF = 0.02f;

    auto nodeList = DependencyManager::get<NodeList>();

    auto nextFrameTimestamp = p_high_resolution_clock::now();
    auto timeToSleep = std::chrono::microseconds(0);

    int currentFrame = 1;
    int numFramesPerSecond = (int) ceil(AudioConstants::NETWORK_FRAMES_PER_SEC);
    int framesSinceCutoffEvent = TRAILING_AVERAGE_FRAMES;

    while (!_isFinished) {
        _trailingSleepRatio = (PREVIOUS_FRAMES_RATIO * _trailingSleepRatio)
            + (timeToSleep.count() * CURRENT_FRAME_RATIO / (float) AudioConstants::NETWORK_FRAME_USECS);

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

                    bool mixHasAudio = prepareMixForListeningNode(node.data());

                    std::unique_ptr<NLPacket> mixPacket;

                    if (mixHasAudio) {
                        int mixPacketBytes = sizeof(quint16) + AudioConstants::MAX_CODEC_NAME_LENGTH_ON_WIRE 
                                                             + AudioConstants::NETWORK_FRAME_BYTES_STEREO;
                        mixPacket = NLPacket::create(PacketType::MixedAudio, mixPacketBytes);

                        // pack sequence number
                        quint16 sequence = nodeData->getOutgoingSequenceNumber();
                        mixPacket->writePrimitive(sequence);

                        // write the codec
                        QString codecInPacket = nodeData->getCodecName();
                        mixPacket->writeString(codecInPacket);

                        QByteArray decodedBuffer(reinterpret_cast<char*>(_clampedSamples), AudioConstants::NETWORK_FRAME_BYTES_STEREO);
                        QByteArray encodedBuffer;
                        nodeData->encode(decodedBuffer, encodedBuffer);

                        // pack mixed audio samples
                        mixPacket->write(encodedBuffer.constData(), encodedBuffer.size());
                    } else {
                        int silentPacketBytes = sizeof(quint16) + sizeof(quint16) + AudioConstants::MAX_CODEC_NAME_LENGTH_ON_WIRE;
                        mixPacket = NLPacket::create(PacketType::SilentAudioFrame, silentPacketBytes);

                        // pack sequence number
                        quint16 sequence = nodeData->getOutgoingSequenceNumber();
                        mixPacket->writePrimitive(sequence);

                        // write the codec
                        QString codecInPacket = nodeData->getCodecName();
                        mixPacket->writeString(codecInPacket);

                        // pack number of silent audio samples
                        quint16 numSilentSamples = AudioConstants::NETWORK_FRAME_SAMPLES_STEREO;
                        mixPacket->writePrimitive(numSilentSamples);
                    }

                    // Send audio environment
                    sendAudioEnvironmentPacket(node);

                    // send mixed audio packet
                    nodeList->sendPacket(std::move(mixPacket), *node);
                    nodeData->incrementOutgoingMixedAudioSequenceNumber();

                    // send an audio stream stats packet to the client approximately every second
                    ++currentFrame;
                    currentFrame %= numFramesPerSecond;

                    if (nodeData->shouldSendStats(currentFrame)) {
                        nodeData->sendAudioStreamStatsPackets(node);
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

        // sleep until the next frame, if necessary
        {
            nextFrameTimestamp += std::chrono::microseconds(AudioConstants::NETWORK_FRAME_USECS);

            auto now = p_high_resolution_clock::now();
            timeToSleep = std::chrono::duration_cast<std::chrono::microseconds>(nextFrameTimestamp - now);

            if (timeToSleep.count() < 0) {
                nextFrameTimestamp = now;
                timeToSleep = std::chrono::microseconds(0);
            }

            std::this_thread::sleep_for(timeToSleep);
        }
    }
}

void AudioMixer::parseSettingsObject(const QJsonObject &settingsObject) {
    if (settingsObject.contains(AUDIO_BUFFER_GROUP_KEY)) {
        QJsonObject audioBufferGroupObject = settingsObject[AUDIO_BUFFER_GROUP_KEY].toObject();

        // check the payload to see if we have asked for dynamicJitterBuffer support
        const QString DYNAMIC_JITTER_BUFFER_JSON_KEY = "dynamic_jitter_buffer";
        bool enableDynamicJitterBuffer = audioBufferGroupObject[DYNAMIC_JITTER_BUFFER_JSON_KEY].toBool();
        if (enableDynamicJitterBuffer) {
            qDebug() << "Enabling dynamic jitter buffers.";

            bool ok;
            const QString DESIRED_JITTER_BUFFER_FRAMES_KEY = "static_desired_jitter_buffer_frames";
            _numStaticJitterFrames = audioBufferGroupObject[DESIRED_JITTER_BUFFER_FRAMES_KEY].toString().toInt(&ok);
            if (!ok) {
                _numStaticJitterFrames = InboundAudioStream::DEFAULT_STATIC_JITTER_FRAMES;
            }
            qDebug() << "Static desired jitter buffer frames:" << _numStaticJitterFrames;
        } else {
            qDebug() << "Disabling dynamic jitter buffers.";
            _numStaticJitterFrames = -1;
        }

        // check for deprecated audio settings
        auto deprecationNotice = [](const QString& setting, const QString& value) {
            qInfo().nospace() << "[DEPRECATION NOTICE] " << setting << "(" << value << ") has been deprecated, and has no effect";
        };
        bool ok;

        const QString MAX_FRAMES_OVER_DESIRED_JSON_KEY = "max_frames_over_desired";
        int maxFramesOverDesired = audioBufferGroupObject[MAX_FRAMES_OVER_DESIRED_JSON_KEY].toString().toInt(&ok);
        if (ok && maxFramesOverDesired != InboundAudioStream::MAX_FRAMES_OVER_DESIRED) {
            deprecationNotice(MAX_FRAMES_OVER_DESIRED_JSON_KEY, QString::number(maxFramesOverDesired));
        }

        const QString WINDOW_STARVE_THRESHOLD_JSON_KEY = "window_starve_threshold";
        int windowStarveThreshold = audioBufferGroupObject[WINDOW_STARVE_THRESHOLD_JSON_KEY].toString().toInt(&ok);
        if (ok && windowStarveThreshold != InboundAudioStream::WINDOW_STARVE_THRESHOLD) {
            deprecationNotice(WINDOW_STARVE_THRESHOLD_JSON_KEY, QString::number(windowStarveThreshold));
        }

        const QString WINDOW_SECONDS_FOR_DESIRED_CALC_ON_TOO_MANY_STARVES_JSON_KEY = "window_seconds_for_desired_calc_on_too_many_starves";
        int windowSecondsForDesiredCalcOnTooManyStarves = audioBufferGroupObject[WINDOW_SECONDS_FOR_DESIRED_CALC_ON_TOO_MANY_STARVES_JSON_KEY].toString().toInt(&ok);
        if (ok && windowSecondsForDesiredCalcOnTooManyStarves != InboundAudioStream::WINDOW_SECONDS_FOR_DESIRED_CALC_ON_TOO_MANY_STARVES) {
            deprecationNotice(WINDOW_SECONDS_FOR_DESIRED_CALC_ON_TOO_MANY_STARVES_JSON_KEY, QString::number(windowSecondsForDesiredCalcOnTooManyStarves));
        }

        const QString WINDOW_SECONDS_FOR_DESIRED_REDUCTION_JSON_KEY = "window_seconds_for_desired_reduction";
        int windowSecondsForDesiredReduction = audioBufferGroupObject[WINDOW_SECONDS_FOR_DESIRED_REDUCTION_JSON_KEY].toString().toInt(&ok);
        if (ok && windowSecondsForDesiredReduction != InboundAudioStream::WINDOW_SECONDS_FOR_DESIRED_REDUCTION) {
            deprecationNotice(WINDOW_SECONDS_FOR_DESIRED_REDUCTION_JSON_KEY, QString::number(windowSecondsForDesiredReduction));
        }

        const QString USE_STDEV_FOR_JITTER_JSON_KEY = "use_stdev_for_desired_calc";
        bool useStDevForJitterCalc = audioBufferGroupObject[USE_STDEV_FOR_JITTER_JSON_KEY].toBool();
        if (useStDevForJitterCalc != InboundAudioStream::USE_STDEV_FOR_JITTER) {
            deprecationNotice(USE_STDEV_FOR_JITTER_JSON_KEY, useStDevForJitterCalc ? "true" : "false");
        }

        const QString REPETITION_WITH_FADE_JSON_KEY = "repetition_with_fade";
        bool repetitionWithFade = audioBufferGroupObject[REPETITION_WITH_FADE_JSON_KEY].toBool();
        if (repetitionWithFade != InboundAudioStream::REPETITION_WITH_FADE) {
            deprecationNotice(REPETITION_WITH_FADE_JSON_KEY, repetitionWithFade ? "true" : "false");
        }
    }

    if (settingsObject.contains(AUDIO_ENV_GROUP_KEY)) {
        QJsonObject audioEnvGroupObject = settingsObject[AUDIO_ENV_GROUP_KEY].toObject();

        const QString CODEC_PREFERENCE_ORDER = "codec_preference_order";
        if (audioEnvGroupObject[CODEC_PREFERENCE_ORDER].isString()) {
            _codecPreferenceOrder = audioEnvGroupObject[CODEC_PREFERENCE_ORDER].toString();
            qDebug() << "Codec preference order changed to" << _codecPreferenceOrder;
        }

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

            const QString X_MIN = "x_min";
            const QString X_MAX = "x_max";
            const QString Y_MIN = "y_min";
            const QString Y_MAX = "y_max";
            const QString Z_MIN = "z_min";
            const QString Z_MAX = "z_max";
            foreach (const QString& zone, zones.keys()) {
                QJsonObject zoneObject = zones[zone].toObject();

                if (zoneObject.contains(X_MIN) && zoneObject.contains(X_MAX) && zoneObject.contains(Y_MIN) &&
                    zoneObject.contains(Y_MAX) && zoneObject.contains(Z_MIN) && zoneObject.contains(Z_MAX)) {

                    float xMin, xMax, yMin, yMax, zMin, zMax;
                    bool ok, allOk = true;
                    xMin = zoneObject.value(X_MIN).toString().toFloat(&ok);
                    allOk &= ok;
                    xMax = zoneObject.value(X_MAX).toString().toFloat(&ok);
                    allOk &= ok;
                    yMin = zoneObject.value(Y_MIN).toString().toFloat(&ok);
                    allOk &= ok;
                    yMax = zoneObject.value(Y_MAX).toString().toFloat(&ok);
                    allOk &= ok;
                    zMin = zoneObject.value(Z_MIN).toString().toFloat(&ok);
                    allOk &= ok;
                    zMax = zoneObject.value(Z_MAX).toString().toFloat(&ok);
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
