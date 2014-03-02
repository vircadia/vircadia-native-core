//
//  AudioMixer.cpp
//  hifi
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
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
#include "Syssocket.h"
#include "Systime.h"
#include <math.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/socket.h>
#endif //_WIN32

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>

#include <Logging.h>
#include <NodeList.h>
#include <Node.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <StdDev.h>
#include <UUID.h>

#include "AudioRingBuffer.h"
#include "AudioMixerClientData.h"
#include "AvatarAudioRingBuffer.h"
#include "InjectedAudioRingBuffer.h"

#include "AudioMixer.h"

const short JITTER_BUFFER_MSECS = 12;
const short JITTER_BUFFER_SAMPLES = JITTER_BUFFER_MSECS * (SAMPLE_RATE / 1000.0);

const QString AUDIO_MIXER_LOGGING_TARGET_NAME = "audio-mixer";

void attachNewBufferToNode(Node *newNode) {
    if (!newNode->getLinkedData()) {
        newNode->setLinkedData(new AudioMixerClientData());
    }
}

AudioMixer::AudioMixer(const QByteArray& packet) :
    ThreadedAssignment(packet),
    _clientMixBuffer(NETWORK_BUFFER_LENGTH_BYTES_STEREO + numBytesForPacketHeaderGivenPacketType(PacketTypeMixedAudio), 0)
{
    connect(NodeList::getInstance(), &NodeList::uuidChanged, this, &AudioMixer::receivedSessionUUID);
}

void AudioMixer::addBufferToMixForListeningNodeWithBuffer(PositionalAudioRingBuffer* bufferToAdd,
                                                          AvatarAudioRingBuffer* listeningNodeBuffer) {
    float bearingRelativeAngleToSource = 0.0f;
    float attenuationCoefficient = 1.0f;
    int numSamplesDelay = 0;
    float weakChannelAmplitudeRatio = 1.0f;

    const int PHASE_DELAY_AT_90 = 20;

    if (bufferToAdd != listeningNodeBuffer) {
        // if the two buffer pointers do not match then these are different buffers

        glm::vec3 listenerPosition = listeningNodeBuffer->getPosition();
        glm::vec3 relativePosition = bufferToAdd->getPosition() - listeningNodeBuffer->getPosition();
        glm::quat inverseOrientation = glm::inverse(listeningNodeBuffer->getOrientation());

        float distanceSquareToSource = glm::dot(relativePosition, relativePosition);
        float radius = 0.0f;

        if (bufferToAdd->getType() == PositionalAudioRingBuffer::Injector) {
            InjectedAudioRingBuffer* injectedBuffer = (InjectedAudioRingBuffer*) bufferToAdd;
            radius = injectedBuffer->getRadius();
            attenuationCoefficient *= injectedBuffer->getAttenuationRatio();
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
                glm::vec3 rotatedListenerPosition = glm::inverse(bufferToAdd->getOrientation()) * relativePosition;

                float angleOfDelivery = glm::angle(glm::vec3(0.0f, 0.0f, -1.0f),
                                                   glm::normalize(rotatedListenerPosition));

                const float MAX_OFF_AXIS_ATTENUATION = 0.2f;
                const float OFF_AXIS_ATTENUATION_FORMULA_STEP = (1 - MAX_OFF_AXIS_ATTENUATION) / 2.0f;

                float offAxisCoefficient = MAX_OFF_AXIS_ATTENUATION +
                    (OFF_AXIS_ATTENUATION_FORMULA_STEP * (angleOfDelivery / 90.0f));

                // multiply the current attenuation coefficient by the calculated off axis coefficient
                attenuationCoefficient *= offAxisCoefficient;
            }

            glm::vec3 rotatedSourcePosition = inverseOrientation * relativePosition;

            const float DISTANCE_SCALE = 2.5f;
            const float GEOMETRIC_AMPLITUDE_SCALAR = 0.3f;
            const float DISTANCE_LOG_BASE = 2.5f;
            const float DISTANCE_SCALE_LOG = logf(DISTANCE_SCALE) / logf(DISTANCE_LOG_BASE);

            // calculate the distance coefficient using the distance to this node
            float distanceCoefficient = powf(GEOMETRIC_AMPLITUDE_SCALAR,
                                             DISTANCE_SCALE_LOG +
                                             (0.5f * logf(distanceSquareToSource) / logf(DISTANCE_LOG_BASE)) - 1);
            distanceCoefficient = std::min(1.0f, distanceCoefficient);

            // multiply the current attenuation coefficient by the distance coefficient
            attenuationCoefficient *= distanceCoefficient;

            // project the rotated source position vector onto the XZ plane
            rotatedSourcePosition.y = 0.0f;

            // produce an oriented angle about the y-axis
            bearingRelativeAngleToSource = glm::orientedAngle(glm::vec3(0.0f, 0.0f, -1.0f),
                                                              glm::normalize(rotatedSourcePosition),
                                                              glm::vec3(0.0f, 1.0f, 0.0f));

            const float PHASE_AMPLITUDE_RATIO_AT_90 = 0.5;

            // figure out the number of samples of delay and the ratio of the amplitude
            // in the weak channel for audio spatialization
            float sinRatio = fabsf(sinf(glm::radians(bearingRelativeAngleToSource)));
            numSamplesDelay = PHASE_DELAY_AT_90 * sinRatio;
            weakChannelAmplitudeRatio = 1 - (PHASE_AMPLITUDE_RATIO_AT_90 * sinRatio);
        }
    }

    // if the bearing relative angle to source is > 0 then the delayed channel is the right one
    int delayedChannelOffset = (bearingRelativeAngleToSource > 0.0f) ? 1 : 0;
    int goodChannelOffset = delayedChannelOffset == 0 ? 1 : 0;

    for (int s = 0; s < NETWORK_BUFFER_LENGTH_SAMPLES_STEREO; s += 2) {
        if ((s / 2) < numSamplesDelay) {
            // pull the earlier sample for the delayed channel
            int earlierSample = (*bufferToAdd)[(s / 2) - numSamplesDelay] * attenuationCoefficient * weakChannelAmplitudeRatio;
            _clientSamples[s + delayedChannelOffset] = glm::clamp(_clientSamples[s + delayedChannelOffset] + earlierSample,
                                                                    MIN_SAMPLE_VALUE, MAX_SAMPLE_VALUE);
        }

        // pull the current sample for the good channel
        int16_t currentSample = (*bufferToAdd)[s / 2] * attenuationCoefficient;
        _clientSamples[s + goodChannelOffset] = glm::clamp(_clientSamples[s + goodChannelOffset] + currentSample,
                                                           MIN_SAMPLE_VALUE, MAX_SAMPLE_VALUE);

        if ((s / 2) + numSamplesDelay < NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL) {
            // place the current sample at the right spot in the delayed channel
            int16_t clampedSample = glm::clamp((int) (_clientSamples[s + (numSamplesDelay * 2) + delayedChannelOffset]
                                               + (currentSample * weakChannelAmplitudeRatio)),
                                               MIN_SAMPLE_VALUE, MAX_SAMPLE_VALUE);
            _clientSamples[s + (numSamplesDelay * 2) + delayedChannelOffset] = clampedSample;
        }
    }
}

void AudioMixer::prepareMixForListeningNode(Node* node) {
    AvatarAudioRingBuffer* nodeRingBuffer = ((AudioMixerClientData*) node->getLinkedData())->getAvatarAudioRingBuffer();

    // zero out the client mix for this node
    memset(_clientSamples, 0, sizeof(_clientSamples));

    // loop through all other nodes that have sufficient audio to mix
    foreach (const SharedNodePointer& otherNode, NodeList::getInstance()->getNodeHash()) {
        if (otherNode->getLinkedData()) {

            AudioMixerClientData* otherNodeClientData = (AudioMixerClientData*) otherNode->getLinkedData();

            // enumerate the ARBs attached to the otherNode and add all that should be added to mix
            for (unsigned int i = 0; i < otherNodeClientData->getRingBuffers().size(); i++) {
                PositionalAudioRingBuffer* otherNodeBuffer = otherNodeClientData->getRingBuffers()[i];

                if ((*otherNode != *node
                     || otherNodeBuffer->shouldLoopbackForNode())
                    && otherNodeBuffer->willBeAddedToMix()) {
                    addBufferToMixForListeningNodeWithBuffer(otherNodeBuffer, nodeRingBuffer);
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
                || mixerPacketType == PacketTypeInjectAudio) {
                
                nodeList->findNodeAndUpdateWithDataFromPacket(receivedPacket);
            } else {
                // let processNodeData handle it.
                nodeList->processNodeData(senderSockAddr, receivedPacket);
            }
        }
    }
}

void AudioMixer::receivedSessionUUID(const QUuid& sessionUUID) {
    populatePacketHeader(_clientMixBuffer, PacketTypeMixedAudio);
}

void AudioMixer::run() {

    commonInit(AUDIO_MIXER_LOGGING_TARGET_NAME, NodeType::AudioMixer);

    NodeList* nodeList = NodeList::getInstance();

    nodeList->addNodeTypeToInterestSet(NodeType::Agent);

    nodeList->linkedDataCreateCallback = attachNewBufferToNode;

    int nextFrame = 0;
    timeval startTime;

    gettimeofday(&startTime, NULL);

    int numBytesPacketHeader = numBytesForPacketHeaderGivenPacketType(PacketTypeMixedAudio);

    while (!_isFinished) {

        QCoreApplication::processEvents();

        if (_isFinished) {
            break;
        }

        foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
            if (node->getLinkedData()) {
                ((AudioMixerClientData*) node->getLinkedData())->checkBuffersBeforeFrameSend(JITTER_BUFFER_SAMPLES);
            }
        }

        foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
            if (node->getType() == NodeType::Agent && node->getActiveSocket() && node->getLinkedData()
                && ((AudioMixerClientData*) node->getLinkedData())->getAvatarAudioRingBuffer()) {
                prepareMixForListeningNode(node.data());

                memcpy(_clientMixBuffer.data() + numBytesPacketHeader, _clientSamples, sizeof(_clientSamples));
                nodeList->writeDatagram(_clientMixBuffer, node);
            }
        }

        // push forward the next output pointers for any audio buffers we used
        foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
            if (node->getLinkedData()) {
                ((AudioMixerClientData*) node->getLinkedData())->pushBuffersAfterFrameSend();
            }
        }

        int usecToSleep = usecTimestamp(&startTime) + (++nextFrame * BUFFER_SEND_INTERVAL_USECS) - usecTimestampNow();

        if (usecToSleep > 0) {
            usleep(usecToSleep);
        } else {
            qDebug() << "AudioMixer loop took" << -usecToSleep << "of extra time. Not sleeping.";
        }

    }
}
