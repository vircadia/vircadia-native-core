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

InboundAudioStream::Settings AudioMixer::_streamSettings;

bool AudioMixer::_printStreamStats = false;

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
    _lastPerSecondCallbackTime(usecTimestampNow()),
    _sendAudioStreamStats(false),
    _datagramsReadPerCallStats(0, READ_DATAGRAMS_STATS_WINDOW_SECONDS),
    _timeSpentPerCallStats(0, READ_DATAGRAMS_STATS_WINDOW_SECONDS),
    _timeSpentPerHashMatchCallStats(0, READ_DATAGRAMS_STATS_WINDOW_SECONDS),
    _readPendingCallsPerSecondStats(1, READ_DATAGRAMS_STATS_WINDOW_SECONDS)
{
    
}

AudioMixer::~AudioMixer() {
    delete _sourceUnattenuatedZone;
    delete _listenerUnattenuatedZone;
}

const float ATTENUATION_BEGINS_AT_DISTANCE = 1.0f;
const float ATTENUATION_AMOUNT_PER_DOUBLING_IN_DISTANCE = 0.18f;
const float ATTENUATION_EPSILON_DISTANCE = 0.1f;

int AudioMixer::addStreamToMixForListeningNodeWithStream(PositionalAudioStream* streamToAdd,
                                                          AvatarAudioStream* listeningNodeStream) {
    // If repetition with fade is enabled:
    // If streamToAdd could not provide a frame (it was starved), then we'll mix its previously-mixed frame
    // This is preferable to not mixing it at all since that's equivalent to inserting silence.
    // Basically, we'll repeat that last frame until it has a frame to mix.  Depending on how many times
    // we've repeated that frame in a row, we'll gradually fade that repeated frame into silence.
    // This improves the perceived quality of the audio slightly.

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
            return 0;
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
        float attenuationAndFade = attenuationCoefficient * repeatedFrameFadeFactor;
        
        for (int s = 0; s < NETWORK_BUFFER_LENGTH_SAMPLES_STEREO; s += 4) {
            
            // setup the int16_t variables for the two sample sets
            correctStreamSample[0] = streamPopOutput[s / 2] * attenuationAndFade;
            correctStreamSample[1] = streamPopOutput[(s / 2) + 1] * attenuationAndFade;
            
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
            float attenuationAndWeakChannelRatioAndFade = attenuationCoefficient * weakChannelAmplitudeRatio * repeatedFrameFadeFactor;
            AudioRingBuffer::ConstIterator delayStreamPopOutput = streamPopOutput - numSamplesDelay;

            // TODO: delayStreamPopOutput may be inside the last frame written if the ringbuffer is completely full
            // maybe make AudioRingBuffer have 1 extra frame in its buffer
            
            for (int i = 0; i < numSamplesDelay; i++) {
                int parentIndex = i * 2;
                _clientSamples[parentIndex + delayedChannelOffset] += *delayStreamPopOutput * attenuationAndWeakChannelRatioAndFade;
                ++delayStreamPopOutput;
            }
        }
    } else {
        int stereoDivider = streamToAdd->isStereo() ? 1 : 2;

        if (!shouldAttenuate) {
            attenuationCoefficient = 1.0f;
        }

        float attenuationAndFade = attenuationCoefficient * repeatedFrameFadeFactor;

        for (int s = 0; s < NETWORK_BUFFER_LENGTH_SAMPLES_STEREO; s++) {
            _clientSamples[s] = glm::clamp(_clientSamples[s] + (int)(streamPopOutput[s / stereoDivider] * attenuationAndFade),
                                            MIN_SAMPLE_VALUE, MAX_SAMPLE_VALUE);
        }
    }

    return 1;
}

int AudioMixer::prepareMixForListeningNode(Node* node) {
    AvatarAudioStream* nodeAudioStream = ((AudioMixerClientData*) node->getLinkedData())->getAvatarAudioStream();
    
    // zero out the client mix for this node
    memset(_clientSamples, 0, NETWORK_BUFFER_LENGTH_BYTES_STEREO);

    // loop through all other nodes that have sufficient audio to mix
    int streamsMixed = 0;
    foreach (const SharedNodePointer& otherNode, NodeList::getInstance()->getNodeHash()) {
        if (otherNode->getLinkedData()) {
            AudioMixerClientData* otherNodeClientData = (AudioMixerClientData*) otherNode->getLinkedData();

            // enumerate the ARBs attached to the otherNode and add all that should be added to mix

            const QHash<QUuid, PositionalAudioStream*>& otherNodeAudioStreams = otherNodeClientData->getAudioStreams();
            QHash<QUuid, PositionalAudioStream*>::ConstIterator i;
            for (i = otherNodeAudioStreams.constBegin(); i != otherNodeAudioStreams.constEnd(); i++) {
                PositionalAudioStream* otherNodeStream = i.value();
                
                if (*otherNode != *node || otherNodeStream->shouldLoopbackForNode()) {
                    streamsMixed += addStreamToMixForListeningNodeWithStream(otherNodeStream, nodeAudioStream);
                }
            }
        }
    }
    return streamsMixed;
}

void AudioMixer::readPendingDatagrams() {
    quint64 readPendingDatagramsStart = usecTimestampNow();

    QByteArray receivedPacket;
    HifiSockAddr senderSockAddr;
    NodeList* nodeList = NodeList::getInstance();
    
    int datagramsRead = 0;
    while (readAvailableDatagram(receivedPacket, senderSockAddr)) {
        quint64 packetVersionAndHashMatchStart = usecTimestampNow();
        bool match = nodeList->packetVersionAndHashMatch(receivedPacket);
        _timeSpentPerHashMatchCallStats.update(usecTimestampNow() - packetVersionAndHashMatchStart);
        if (match) {
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
        datagramsRead++;
    }

    _timeSpentPerCallStats.update(usecTimestampNow() - readPendingDatagramsStart);
    _datagramsReadPerCallStats.update(datagramsRead);
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
    
    QString property = "readPendingDatagramsStats";
    QString value = getReadPendingDatagramsStatsString();
    statsObject2[qPrintable(property)] = value;
    somethingToSend = true;
    sizeOfStats += property.size() + value.size();

    NodeList* nodeList = NodeList::getInstance();
    int clientNumber = 0;
    foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {

        // if we're too large, send the packet
        if (sizeOfStats > TOO_BIG_FOR_MTU) {
            nodeList->sendStatsToDomainServer(statsObject2);
            sizeOfStats = 0;
            statsObject2 = QJsonObject(); // clear it
            somethingToSend = false;
        }

        clientNumber++;
        AudioMixerClientData* clientData = static_cast<AudioMixerClientData*>(node->getLinkedData());
        if (clientData) {
            QString property = "jitterStats." + node->getUUID().toString();
            QString value = clientData->getAudioStreamStatsString();
            statsObject2[qPrintable(property)] = value;
            somethingToSend = true;
            sizeOfStats += property.size() + value.size();
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
    const QString AUDIO_GROUP_KEY = "audio";
    
    if (settingsObject.contains(AUDIO_GROUP_KEY)) {
        QJsonObject audioGroupObject = settingsObject[AUDIO_GROUP_KEY].toObject();
        
        bool ok;

        // check the payload to see if we have asked for dynamicJitterBuffer support
        const QString DYNAMIC_JITTER_BUFFER_JSON_KEY = "A-dynamic-jitter-buffer";
        _streamSettings._dynamicJitterBuffers = audioGroupObject[DYNAMIC_JITTER_BUFFER_JSON_KEY].toBool();
        if (_streamSettings._dynamicJitterBuffers) {
            qDebug() << "Enable dynamic jitter buffers.";
        } else {
            qDebug() << "Dynamic jitter buffers disabled.";
        }

        const QString DESIRED_JITTER_BUFFER_FRAMES_KEY = "B-static-desired-jitter-buffer-frames";
        _streamSettings._staticDesiredJitterBufferFrames = audioGroupObject[DESIRED_JITTER_BUFFER_FRAMES_KEY].toString().toInt(&ok);
        if (!ok) {
            _streamSettings._staticDesiredJitterBufferFrames = DEFAULT_STATIC_DESIRED_JITTER_BUFFER_FRAMES;
        }
        qDebug() << "Static desired jitter buffer frames:" << _streamSettings._staticDesiredJitterBufferFrames;

        const QString MAX_FRAMES_OVER_DESIRED_JSON_KEY = "C-max-frames-over-desired";
        _streamSettings._maxFramesOverDesired = audioGroupObject[MAX_FRAMES_OVER_DESIRED_JSON_KEY].toString().toInt(&ok);
        if (!ok) {
            _streamSettings._maxFramesOverDesired = DEFAULT_MAX_FRAMES_OVER_DESIRED;
        }
        qDebug() << "Max frames over desired:" << _streamSettings._maxFramesOverDesired;
        
        const QString USE_STDEV_FOR_DESIRED_CALC_JSON_KEY = "D-use-stdev-for-desired-calc";
        _streamSettings._useStDevForJitterCalc = audioGroupObject[USE_STDEV_FOR_DESIRED_CALC_JSON_KEY].toBool();
        if (_streamSettings._useStDevForJitterCalc) {
            qDebug() << "Using Philip's stdev method for jitter calc if dynamic jitter buffers enabled";
        } else {
            qDebug() << "Using Fred's max-gap method for jitter calc if dynamic jitter buffers enabled";
        }

        const QString WINDOW_STARVE_THRESHOLD_JSON_KEY = "E-window-starve-threshold";
        _streamSettings._windowStarveThreshold = audioGroupObject[WINDOW_STARVE_THRESHOLD_JSON_KEY].toString().toInt(&ok);
        if (!ok) {
            _streamSettings._windowStarveThreshold = DEFAULT_WINDOW_STARVE_THRESHOLD;
        }
        qDebug() << "Window A starve threshold:" << _streamSettings._windowStarveThreshold;

        const QString WINDOW_SECONDS_FOR_DESIRED_CALC_ON_TOO_MANY_STARVES_JSON_KEY = "F-window-seconds-for-desired-calc-on-too-many-starves";
        _streamSettings._windowSecondsForDesiredCalcOnTooManyStarves = audioGroupObject[WINDOW_SECONDS_FOR_DESIRED_CALC_ON_TOO_MANY_STARVES_JSON_KEY].toString().toInt(&ok);
        if (!ok) {
            _streamSettings._windowSecondsForDesiredCalcOnTooManyStarves = DEFAULT_WINDOW_SECONDS_FOR_DESIRED_CALC_ON_TOO_MANY_STARVES;
        }
        qDebug() << "Window A length:" << _streamSettings._windowSecondsForDesiredCalcOnTooManyStarves << "seconds";

        const QString WINDOW_SECONDS_FOR_DESIRED_REDUCTION_JSON_KEY = "G-window-seconds-for-desired-reduction";
        _streamSettings._windowSecondsForDesiredReduction = audioGroupObject[WINDOW_SECONDS_FOR_DESIRED_REDUCTION_JSON_KEY].toString().toInt(&ok);
        if (!ok) {
            _streamSettings._windowSecondsForDesiredReduction = DEFAULT_WINDOW_SECONDS_FOR_DESIRED_REDUCTION;
        }
        qDebug() << "Window B length:" << _streamSettings._windowSecondsForDesiredReduction << "seconds";

        const QString REPETITION_WITH_FADE_JSON_KEY = "H-repetition-with-fade";
        _streamSettings._repetitionWithFade = audioGroupObject[REPETITION_WITH_FADE_JSON_KEY].toBool();
        if (_streamSettings._repetitionWithFade) {
            qDebug() << "Repetition with fade enabled";
        } else {
            qDebug() << "Repetition with fade disabled";
        }
        
        const QString PRINT_STREAM_STATS_JSON_KEY = "I-print-stream-stats";
        _printStreamStats = audioGroupObject[PRINT_STREAM_STATS_JSON_KEY].toBool();
        if (_printStreamStats) {
            qDebug() << "Stream stats will be printed to stdout";
        }

        const QString UNATTENUATED_ZONE_KEY = "Z-unattenuated-zone";

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
    }
    
    int nextFrame = 0;
    QElapsedTimer timer;
    timer.start();

    char clientMixBuffer[MAX_PACKET_SIZE];
    
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

        quint64 now = usecTimestampNow();
        if (now - _lastPerSecondCallbackTime > USECS_PER_SECOND) {
            perSecondActions();
            _lastPerSecondCallbackTime = now;
        }

        bool streamStatsPrinted = false;
        foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
            if (node->getLinkedData()) {
                AudioMixerClientData* nodeData = (AudioMixerClientData*)node->getLinkedData();

                // this function will attempt to pop a frame from each audio stream.
                // a pointer to the popped data is stored as a member in InboundAudioStream.
                // That's how the popped audio data will be read for mixing (but only if the pop was successful)
                nodeData->checkBuffersBeforeFrameSend(_sourceUnattenuatedZone, _listenerUnattenuatedZone);
            
                if (node->getType() == NodeType::Agent && node->getActiveSocket()
                    && nodeData->getAvatarAudioStream()) {

                    int streamsMixed = prepareMixForListeningNode(node.data());

                    char* dataAt;
                    if (streamsMixed > 0) {
                        // pack header
                        int numBytesPacketHeader = populatePacketHeader(clientMixBuffer, PacketTypeMixedAudio);
                        dataAt = clientMixBuffer + numBytesPacketHeader;

                        // pack sequence number
                        quint16 sequence = nodeData->getOutgoingSequenceNumber();
                        memcpy(dataAt, &sequence, sizeof(quint16));
                        dataAt += sizeof(quint16);

                        // pack mixed audio samples
                        memcpy(dataAt, _clientSamples, NETWORK_BUFFER_LENGTH_BYTES_STEREO);
                        dataAt += NETWORK_BUFFER_LENGTH_BYTES_STEREO;
                    } else {
                        // pack header
                        int numBytesPacketHeader = populatePacketHeader(clientMixBuffer, PacketTypeSilentAudioFrame);
                        dataAt = clientMixBuffer + numBytesPacketHeader;

                        // pack sequence number
                        quint16 sequence = nodeData->getOutgoingSequenceNumber();
                        memcpy(dataAt, &sequence, sizeof(quint16));
                        dataAt += sizeof(quint16);

                        // pack number of silent audio samples
                        quint16 numSilentSamples = NETWORK_BUFFER_LENGTH_SAMPLES_STEREO;
                        memcpy(dataAt, &numSilentSamples, sizeof(quint16));
                        dataAt += sizeof(quint16);
                    }

                    // send mixed audio packet
                    nodeList->writeDatagram(clientMixBuffer, dataAt - clientMixBuffer, node);
                    nodeData->incrementOutgoingMixedAudioSequenceNumber();

                    // send an audio stream stats packet if it's time
                    if (_sendAudioStreamStats) {
                        nodeData->sendAudioStreamStatsPackets(node);
                        _sendAudioStreamStats = false;
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

        foreach(const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
            if (node->getLinkedData()) {
                AudioMixerClientData* nodeData = (AudioMixerClientData*)node->getLinkedData();

                if (node->getType() == NodeType::Agent && node->getActiveSocket()) {
                    printf("\nStats for agent %s --------------------------------\n",
                        node->getUUID().toString().toLatin1().data());
                    nodeData->printUpstreamDownstreamStats();
                }
            }
        }
    }

    _datagramsReadPerCallStats.currentIntervalComplete();
    _timeSpentPerCallStats.currentIntervalComplete();
    _timeSpentPerHashMatchCallStats.currentIntervalComplete();
}

QString AudioMixer::getReadPendingDatagramsStatsString() const {
    QString result
        = "calls_per_sec_avg_30s: " + QString::number(_readPendingCallsPerSecondStats.getWindowAverage(), 'f', 2)
        + "  calls_last_sec: " + QString::number(_readPendingCallsPerSecondStats.getLastCompleteIntervalStats().getSum() + 0.5, 'f', 0)
        + "  pkts_per_call_avg_30s: " + QString::number(_datagramsReadPerCallStats.getWindowAverage(), 'f', 2)
        + "  pkts_per_call_avg_1s: " + QString::number(_datagramsReadPerCallStats.getLastCompleteIntervalStats().getAverage(), 'f', 2)
        + "  usecs_per_call_avg_30s: " + QString::number(_timeSpentPerCallStats.getWindowAverage(), 'f', 2)
        + "  usecs_per_call_avg_1s: " + QString::number(_timeSpentPerCallStats.getLastCompleteIntervalStats().getAverage(), 'f', 2)
        + "  usecs_per_hashmatch_avg_30s: " + QString::number(_timeSpentPerHashMatchCallStats.getWindowAverage(), 'f', 2)
        + "  usecs_per_hashmatch_avg_1s: " + QString::number(_timeSpentPerHashMatchCallStats.getLastCompleteIntervalStats().getAverage(), 'f', 2)
        + "  prct_time_in_call_30s: " + QString::number(_timeSpentPerCallStats.getWindowSum() / (READ_DATAGRAMS_STATS_WINDOW_SECONDS*USECS_PER_SECOND) * 100.0, 'f', 6) + "%"
        + "  prct_time_in_call_1s: " + QString::number(_timeSpentPerCallStats.getLastCompleteIntervalStats().getSum() / USECS_PER_SECOND * 100.0, 'f', 6) + "%"
        + "  prct_time_in_hashmatch_30s: " + QString::number(_timeSpentPerHashMatchCallStats.getWindowSum() / (READ_DATAGRAMS_STATS_WINDOW_SECONDS*USECS_PER_SECOND) * 100.0, 'f', 6) + "%"
        + "  prct_time_in_hashmatch_1s: " + QString::number(_timeSpentPerHashMatchCallStats.getLastCompleteIntervalStats().getSum() / USECS_PER_SECOND * 100.0, 'f', 6) + "%";
    return result;
}