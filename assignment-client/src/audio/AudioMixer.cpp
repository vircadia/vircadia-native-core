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

#include <mmintrin.h>
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
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include <Logging.h>
#include <NetworkAccessManager.h>
#include <NodeList.h>
#include <Node.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <StdDev.h>
#include <UUID.h>

#include "AudioRingBuffer.h"
#include "AudioMixerClientData.h"
#include "AvatarAudioStream.h"
#include "InjectedAudioStream.h"

#include "AudioMixer.h"

const float LOUDNESS_TO_DISTANCE_RATIO = 0.00001f;

const QString AUDIO_MIXER_LOGGING_TARGET_NAME = "audio-mixer";

void attachNewNodeDataToNode(Node *newNode) {
    if (!newNode->getLinkedData()) {
        newNode->setLinkedData(new AudioMixerClientData());
    }
}

bool AudioMixer::_useDynamicJitterBuffers = false;

AudioMixer::AudioMixer(const QByteArray& packet) :
    ThreadedAssignment(packet),
    _trailingSleepRatio(1.0f),
    _minAudibilityThreshold(LOUDNESS_TO_DISTANCE_RATIO / 2.0f),
    _performanceThrottlingRatio(0.0f),
    _numStatFrames(0),
    _sumListeners(0),
    _sumMixes(0),
    _sourceUnattenuatedZone(NULL),
    _listenerUnattenuatedZone(NULL),
    _lastSendAudioStreamStatsTime(usecTimestampNow())
{
    
}

AudioMixer::~AudioMixer() {
    delete _sourceUnattenuatedZone;
    delete _listenerUnattenuatedZone;
}

const float ATTENUATION_BEGINS_AT_DISTANCE = 1.0f;
const float ATTENUATION_AMOUNT_PER_DOUBLING_IN_DISTANCE = 0.18f;
const float ATTENUATION_EPSILON_DISTANCE = 0.1f;

void AudioMixer::addStreamToMixForListeningNodeWithStream(PositionalAudioStream* streamToAdd,
                                                          AvatarAudioStream* listeningNodeStream) {
    float bearingRelativeAngleToSource = 0.0f;
    float attenuationCoefficient = 1.0f;
    int numSamplesDelay = 0;
    float weakChannelAmplitudeRatio = 1.0f;
    
    bool shouldAttenuate = (streamToAdd != listeningNodeStream);
    
    if (shouldAttenuate) {
        
        // if the two stream pointers do not match then these are different streams
        glm::vec3 relativePosition = streamToAdd->getPosition() - listeningNodeStream->getPosition();
        
        float distanceBetween = glm::length(relativePosition);
        
        if (distanceBetween < EPSILON) {
            distanceBetween = EPSILON;
        }
        
        if (streamToAdd->getLastPopOutputTrailingLoudness() / distanceBetween <= _minAudibilityThreshold) {
            // according to mixer performance we have decided this does not get to be mixed in
            // bail out
            return;
        }
        
        ++_sumMixes;
        
        if (streamToAdd->getListenerUnattenuatedZone()) {
            shouldAttenuate = !streamToAdd->getListenerUnattenuatedZone()->contains(listeningNodeStream->getPosition());
        }
        
        if (streamToAdd->getType() == PositionalAudioStream::Injector) {
            attenuationCoefficient *= reinterpret_cast<InjectedAudioStream*>(streamToAdd)->getAttenuationRatio();
        }
        
        shouldAttenuate = shouldAttenuate && distanceBetween > ATTENUATION_EPSILON_DISTANCE;
        
        if (shouldAttenuate) {
            glm::quat inverseOrientation = glm::inverse(listeningNodeStream->getOrientation());
            
            float distanceSquareToSource = glm::dot(relativePosition, relativePosition);
            float radius = 0.0f;
            
            if (streamToAdd->getType() == PositionalAudioStream::Injector) {
                radius = reinterpret_cast<InjectedAudioStream*>(streamToAdd)->getRadius();
            }
            
            if (radius == 0 || (distanceSquareToSource > radius * radius)) {
                // this is either not a spherical source, or the listener is outside the sphere
                
                if (radius > 0) {
                    // this is a spherical source - the distance used for the coefficient
                    // needs to be the closest point on the boundary to the source
                    
                    // ovveride the distance to the node with the distance to the point on the
                    // boundary of the sphere
                    distanceSquareToSource -= (radius * radius);
                    
                } else {
                    // calculate the angle delivery for off-axis attenuation
                    glm::vec3 rotatedListenerPosition = glm::inverse(streamToAdd->getOrientation()) * relativePosition;
                    
                    float angleOfDelivery = glm::angle(glm::vec3(0.0f, 0.0f, -1.0f),
                                                       glm::normalize(rotatedListenerPosition));
                    
                    const float MAX_OFF_AXIS_ATTENUATION = 0.2f;
                    const float OFF_AXIS_ATTENUATION_FORMULA_STEP = (1 - MAX_OFF_AXIS_ATTENUATION) / 2.0f;
                    
                    float offAxisCoefficient = MAX_OFF_AXIS_ATTENUATION +
                        (OFF_AXIS_ATTENUATION_FORMULA_STEP * (angleOfDelivery / PI_OVER_TWO));
                    
                    // multiply the current attenuation coefficient by the calculated off axis coefficient
                    attenuationCoefficient *= offAxisCoefficient;
                }
                
                glm::vec3 rotatedSourcePosition = inverseOrientation * relativePosition;
                
                if (distanceBetween >= ATTENUATION_BEGINS_AT_DISTANCE) {
                    // calculate the distance coefficient using the distance to this node
                    float distanceCoefficient = 1 - (logf(distanceBetween / ATTENUATION_BEGINS_AT_DISTANCE) / logf(2.0f)
                                                     * ATTENUATION_AMOUNT_PER_DOUBLING_IN_DISTANCE);
                    
                    if (distanceCoefficient < 0) {
                        distanceCoefficient = 0;
                    }
                    
                    // multiply the current attenuation coefficient by the distance coefficient
                    attenuationCoefficient *= distanceCoefficient;
                }
                
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
            }
        }
    }
    
    AudioRingBuffer::ConstIterator streamPopOutput = streamToAdd->getLastPopOutput();
    
    if (!streamToAdd->isStereo() && shouldAttenuate) {
        // this is a mono stream, which means it gets full attenuation and spatialization
        
        // if the bearing relative angle to source is > 0 then the delayed channel is the right one
        int delayedChannelOffset = (bearingRelativeAngleToSource > 0.0f) ? 1 : 0;
        int goodChannelOffset = delayedChannelOffset == 0 ? 1 : 0;
        
        int16_t correctStreamSample[2], delayStreamSample[2];
        int delayedChannelIndex = 0;
        
        const int SINGLE_STEREO_OFFSET = 2;
        
        for (int s = 0; s < NETWORK_BUFFER_LENGTH_SAMPLES_STEREO; s += 4) {
            
            // setup the int16_t variables for the two sample sets
            correctStreamSample[0] = streamPopOutput[s / 2] * attenuationCoefficient;
            correctStreamSample[1] = streamPopOutput[(s / 2) + 1] * attenuationCoefficient;
            
            delayedChannelIndex = s + (numSamplesDelay * 2) + delayedChannelOffset;
            
            delayStreamSample[0] = correctStreamSample[0] * weakChannelAmplitudeRatio;
            delayStreamSample[1] = correctStreamSample[1] * weakChannelAmplitudeRatio;
            
            _clientSamples[s + goodChannelOffset] += correctStreamSample[0];
            _clientSamples[s + goodChannelOffset + SINGLE_STEREO_OFFSET] += correctStreamSample[1];
            _clientSamples[delayedChannelIndex] += delayStreamSample[0];
            _clientSamples[delayedChannelIndex + SINGLE_STEREO_OFFSET] += delayStreamSample[1];
        }
        
        if (numSamplesDelay > 0) {
            // if there was a sample delay for this stream, we need to pull samples prior to the popped output
            // to stick at the beginning
            float attenuationAndWeakChannelRatio = attenuationCoefficient * weakChannelAmplitudeRatio;
            AudioRingBuffer::ConstIterator delayStreamPopOutput = streamPopOutput - numSamplesDelay;

            // TODO: delayStreamPopOutput may be inside the last frame written if the ringbuffer is completely full
            // maybe make AudioRingBuffer have 1 extra frame in its buffer
            
            for (int i = 0; i < numSamplesDelay; i++) {
                int parentIndex = i * 2;
                _clientSamples[parentIndex + delayedChannelOffset] += *delayStreamPopOutput * attenuationAndWeakChannelRatio;
                ++delayStreamPopOutput;
            }
        }
    } else {
        int stereoDivider = streamToAdd->isStereo() ? 1 : 2;

        if (!shouldAttenuate) {
            attenuationCoefficient = 1.0f;
        }

        for (int s = 0; s < NETWORK_BUFFER_LENGTH_SAMPLES_STEREO; s++) {
            _clientSamples[s] = glm::clamp(_clientSamples[s] + (int)(streamPopOutput[s / stereoDivider] * attenuationCoefficient),
                                            MIN_SAMPLE_VALUE, MAX_SAMPLE_VALUE);
        }
    }
}

void AudioMixer::prepareMixForListeningNode(Node* node) {
    AvatarAudioStream* nodeAudioStream = ((AudioMixerClientData*) node->getLinkedData())->getAvatarAudioStream();

    // zero out the client mix for this node
    memset(_clientSamples, 0, NETWORK_BUFFER_LENGTH_BYTES_STEREO);

    // loop through all other nodes that have sufficient audio to mix
    foreach (const SharedNodePointer& otherNode, NodeList::getInstance()->getNodeHash()) {
        if (otherNode->getLinkedData()) {

            AudioMixerClientData* otherNodeClientData = (AudioMixerClientData*) otherNode->getLinkedData();

            // enumerate the ARBs attached to the otherNode and add all that should be added to mix

            const QHash<QUuid, PositionalAudioStream*>& otherNodeAudioStreams = otherNodeClientData->getAudioStreams();
            QHash<QUuid, PositionalAudioStream*>::ConstIterator i;
            for (i = otherNodeAudioStreams.begin(); i != otherNodeAudioStreams.constEnd(); i++) {
                PositionalAudioStream* otherNodeStream = i.value();

                if ((*otherNode != *node || otherNodeStream->shouldLoopbackForNode())
                    && otherNodeStream->lastPopSucceeded()
                    && otherNodeStream->getLastPopOutputTrailingLoudness() > 0.0f) {

                    addStreamToMixForListeningNodeWithStream(otherNodeStream, nodeAudioStream);
                }
            }
        }
    }
}

void AudioMixer::readPendingDatagrams() {
    QByteArray receivedPacket;
    HifiSockAddr senderSockAddr;
    NodeList* nodeList = NodeList::getInstance();
    
    while (readAvailableDatagram(receivedPacket, senderSockAddr)) {
        if (nodeList->packetVersionAndHashMatch(receivedPacket)) {
            // pull any new audio data from nodes off of the network stack
            PacketType mixerPacketType = packetTypeForPacket(receivedPacket);
            if (mixerPacketType == PacketTypeMicrophoneAudioNoEcho
                || mixerPacketType == PacketTypeMicrophoneAudioWithEcho
                || mixerPacketType == PacketTypeInjectAudio
                || mixerPacketType == PacketTypeSilentAudioFrame
                || mixerPacketType == PacketTypeAudioStreamStats) {
                
                nodeList->findNodeAndUpdateWithDataFromPacket(receivedPacket);
            } else if (mixerPacketType == PacketTypeMuteEnvironment) {
                QByteArray packet = receivedPacket;
                populatePacketHeader(packet, PacketTypeMuteEnvironment);
                
                foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
                    if (node->getType() == NodeType::Agent && node->getActiveSocket() && node->getLinkedData() && node != nodeList->sendingNodeForPacket(receivedPacket)) {
                        nodeList->writeDatagram(packet, packet.size(), node);
                    }
                }

            } else {
                // let processNodeData handle it.
                nodeList->processNodeData(senderSockAddr, receivedPacket);
            }
        }
    }
}

void AudioMixer::sendStatsPacket() {
    static QJsonObject statsObject;
    
    statsObject["useDynamicJitterBuffers"] = _useDynamicJitterBuffers;
    statsObject["trailing_sleep_percentage"] = _trailingSleepRatio * 100.0f;
    statsObject["performance_throttling_ratio"] = _performanceThrottlingRatio;

    statsObject["average_listeners_per_frame"] = (float) _sumListeners / (float) _numStatFrames;
    
    if (_sumListeners > 0) {
        statsObject["average_mixes_per_listener"] = (float) _sumMixes / (float) _sumListeners;
    } else {
        statsObject["average_mixes_per_listener"] = 0.0;
    }

    ThreadedAssignment::addPacketStatsAndSendStatsPacket(statsObject);
    _sumListeners = 0;
    _sumMixes = 0;
    _numStatFrames = 0;


    // NOTE: These stats can be too large to fit in an MTU, so we break it up into multiple packts...
    QJsonObject statsObject2;

    // add stats for each listerner
    bool somethingToSend = false;
    int sizeOfStats = 0;
    int TOO_BIG_FOR_MTU = 1200; // some extra space for JSONification
    
    NodeList* nodeList = NodeList::getInstance();
    int clientNumber = 0;
    foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
        clientNumber++;
        AudioMixerClientData* clientData = static_cast<AudioMixerClientData*>(node->getLinkedData());
        if (clientData) {
            QString property = "jitterStats." + node->getUUID().toString();
            QString value = clientData->getAudioStreamStatsString();
            statsObject2[qPrintable(property)] = value;
            somethingToSend = true;
            sizeOfStats += property.size() + value.size();
        }
        
        // if we're too large, send the packet
        if (sizeOfStats > TOO_BIG_FOR_MTU) {
            nodeList->sendStatsToDomainServer(statsObject2);
            sizeOfStats = 0;
            statsObject2 = QJsonObject(); // clear it
            somethingToSend = false;
        }
    }

    if (somethingToSend) {
        nodeList->sendStatsToDomainServer(statsObject2);
    }
}

void AudioMixer::run() {

    ThreadedAssignment::commonInit(AUDIO_MIXER_LOGGING_TARGET_NAME, NodeType::AudioMixer);

    NodeList* nodeList = NodeList::getInstance();

    nodeList->addNodeTypeToInterestSet(NodeType::Agent);

    nodeList->linkedDataCreateCallback = attachNewNodeDataToNode;
    
    // setup a NetworkAccessManager to ask the domain-server for our settings
    NetworkAccessManager& networkManager = NetworkAccessManager::getInstance();
    
    QUrl settingsJSONURL;
    settingsJSONURL.setScheme("http");
    settingsJSONURL.setHost(nodeList->getDomainHandler().getHostname());
    settingsJSONURL.setPort(DOMAIN_SERVER_HTTP_PORT);
    settingsJSONURL.setPath("/settings.json");
    settingsJSONURL.setQuery(QString("type=%1").arg(_type));
    
    QNetworkReply *reply = NULL;
    
    int failedAttempts = 0;
    const int MAX_SETTINGS_REQUEST_FAILED_ATTEMPTS = 5;
    
    qDebug() << "Requesting settings for assignment from domain-server at" << settingsJSONURL.toString();
    
    while (!reply || reply->error() != QNetworkReply::NoError) {
        reply = networkManager.get(QNetworkRequest(settingsJSONURL));
        
        QEventLoop loop;
        QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        
        loop.exec();
        
        ++failedAttempts;
        
        if (failedAttempts == MAX_SETTINGS_REQUEST_FAILED_ATTEMPTS) {
            qDebug() << "Failed to get settings from domain-server. Bailing on assignment.";
            setFinished(true);
            return;
        }
    }
    
    QJsonObject settingsObject = QJsonDocument::fromJson(reply->readAll()).object();
    
    // check the settings object to see if we have anything we can parse out
    const QString AUDIO_GROUP_KEY = "audio";
    
    if (settingsObject.contains(AUDIO_GROUP_KEY)) {
        QJsonObject audioGroupObject = settingsObject[AUDIO_GROUP_KEY].toObject();
        
        const QString UNATTENUATED_ZONE_KEY = "unattenuated-zone";
        
        QString unattenuatedZoneString = audioGroupObject[UNATTENUATED_ZONE_KEY].toString();
        if (!unattenuatedZoneString.isEmpty()) {
            QStringList zoneStringList = unattenuatedZoneString.split(',');
            
            glm::vec3 sourceCorner(zoneStringList[0].toFloat(), zoneStringList[1].toFloat(), zoneStringList[2].toFloat());
            glm::vec3 sourceDimensions(zoneStringList[3].toFloat(), zoneStringList[4].toFloat(), zoneStringList[5].toFloat());
            
            glm::vec3 listenerCorner(zoneStringList[6].toFloat(), zoneStringList[7].toFloat(), zoneStringList[8].toFloat());
            glm::vec3 listenerDimensions(zoneStringList[9].toFloat(), zoneStringList[10].toFloat(), zoneStringList[11].toFloat());
            
            _sourceUnattenuatedZone = new AABox(sourceCorner, sourceDimensions);
            _listenerUnattenuatedZone = new AABox(listenerCorner, listenerDimensions);
            
            glm::vec3 sourceCenter = _sourceUnattenuatedZone->calcCenter();
            glm::vec3 destinationCenter = _listenerUnattenuatedZone->calcCenter();
            
            qDebug() << "There is an unattenuated zone with source center at"
            << QString("%1, %2, %3").arg(sourceCenter.x).arg(sourceCenter.y).arg(sourceCenter.z);
            qDebug() << "Buffers inside this zone will not be attenuated inside a box with center at"
            << QString("%1, %2, %3").arg(destinationCenter.x).arg(destinationCenter.y).arg(destinationCenter.z);
        }
        
        // check the payload to see if we have asked for dynamicJitterBuffer support
        const QString DYNAMIC_JITTER_BUFFER_JSON_KEY = "dynamic-jitter-buffer";
        bool shouldUseDynamicJitterBuffers = audioGroupObject[DYNAMIC_JITTER_BUFFER_JSON_KEY].toBool();
        if (shouldUseDynamicJitterBuffers) {
            qDebug() << "Enable dynamic jitter buffers.";
            _useDynamicJitterBuffers = true;
        } else {
            qDebug() << "Dynamic jitter buffers disabled, using old behavior.";
            _useDynamicJitterBuffers = false;
        }
    }
    
    int nextFrame = 0;
    QElapsedTimer timer;
    timer.start();
    
    char* clientMixBuffer = new char[NETWORK_BUFFER_LENGTH_BYTES_STEREO + sizeof(quint16)
                                     + numBytesForPacketHeaderGivenPacketType(PacketTypeMixedAudio)];
    
    int usecToSleep = BUFFER_SEND_INTERVAL_USECS;
    
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
            + (usecToSleep * CURRENT_FRAME_RATIO / (float) BUFFER_SEND_INTERVAL_USECS);
        
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
        
        bool sendAudioStreamStats = false;
        quint64 now = usecTimestampNow();
        if (now - _lastSendAudioStreamStatsTime > TOO_LONG_SINCE_LAST_SEND_AUDIO_STREAM_STATS) {
            _lastSendAudioStreamStatsTime = now;
            sendAudioStreamStats = true;
        }

        foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
            if (node->getLinkedData()) {
                AudioMixerClientData* nodeData = (AudioMixerClientData*)node->getLinkedData();

                // this function will attempt to pop a frame from each audio stream.
                // a pointer to the popped data is stored as a member in InboundAudioStream.
                // That's how the popped audio data will be read for mixing (but only if the pop was successful)
                nodeData->checkBuffersBeforeFrameSend(_sourceUnattenuatedZone, _listenerUnattenuatedZone);
            
                if (node->getType() == NodeType::Agent && node->getActiveSocket()
                    && nodeData->getAvatarAudioStream()) {

                    prepareMixForListeningNode(node.data());

                    // pack header
                    int numBytesPacketHeader = populatePacketHeader(clientMixBuffer, PacketTypeMixedAudio);
                    char* dataAt = clientMixBuffer + numBytesPacketHeader;

                    // pack sequence number
                    quint16 sequence = nodeData->getOutgoingSequenceNumber();
                    memcpy(dataAt, &sequence, sizeof(quint16));
                    dataAt += sizeof(quint16);

                    // pack mixed audio samples
                    memcpy(dataAt, _clientSamples, NETWORK_BUFFER_LENGTH_BYTES_STEREO);
                    dataAt += NETWORK_BUFFER_LENGTH_BYTES_STEREO;

                    // send mixed audio packet
                    nodeList->writeDatagram(clientMixBuffer, dataAt - clientMixBuffer, node);
                    nodeData->incrementOutgoingMixedAudioSequenceNumber();

                    // send an audio stream stats packet if it's time
                    if (sendAudioStreamStats) {
                        nodeData->sendAudioStreamStatsPackets(node);
                    }

                    ++_sumListeners;
                }
            }
        }
        
        ++_numStatFrames;
        
        QCoreApplication::processEvents();
        
        if (_isFinished) {
            break;
        }

        usecToSleep = (++nextFrame * BUFFER_SEND_INTERVAL_USECS) - timer.nsecsElapsed() / 1000; // ns to us

        if (usecToSleep > 0) {
            usleep(usecToSleep);
        }
    }
    
    delete[] clientMixBuffer;
}
