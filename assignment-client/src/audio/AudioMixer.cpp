//
//  AudioMixer.cpp
//  hifi
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
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

const float LOUDNESS_TO_DISTANCE_RATIO = 0.00305f;

const QString AUDIO_MIXER_LOGGING_TARGET_NAME = "audio-mixer";

void attachNewBufferToNode(Node *newNode) {
    if (!newNode->getLinkedData()) {
        newNode->setLinkedData(new AudioMixerClientData());
    }
}

AudioMixer::AudioMixer(const QByteArray& packet) :
    ThreadedAssignment(packet),
    _trailingSleepRatio(1.0f),
    _minAudibilityThreshold(LOUDNESS_TO_DISTANCE_RATIO / 2.0f),
    _numClientsMixedInFrame(0)
{
    
}

void AudioMixer::addBufferToMixForListeningNodeWithBuffer(PositionalAudioRingBuffer* bufferToAdd,
                                                          AvatarAudioRingBuffer* listeningNodeBuffer) {
    float bearingRelativeAngleToSource = 0.0f;
    float attenuationCoefficient = 1.0f;
    int numSamplesDelay = 0;
    float weakChannelAmplitudeRatio = 1.0f;
    
    if (bufferToAdd != listeningNodeBuffer) {
        // if the two buffer pointers do not match then these are different buffers
        glm::vec3 relativePosition = bufferToAdd->getPosition() - listeningNodeBuffer->getPosition();
        
        float distanceBetween = glm::length(relativePosition);
       
        if (distanceBetween < EPSILON) {
            distanceBetween = EPSILON;
        }
        
        if (bufferToAdd->getAverageLoudness() / distanceBetween <= _minAudibilityThreshold) {
            // according to mixer performance we have decided this does not get to be mixed in
            // bail out
            return;
        }
        
        ++_numClientsMixedInFrame;
        
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
                    (OFF_AXIS_ATTENUATION_FORMULA_STEP * (angleOfDelivery / PI_OVER_TWO));

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
            float sinRatio = fabsf(sinf(bearingRelativeAngleToSource));
            numSamplesDelay = SAMPLE_PHASE_DELAY_AT_90 * sinRatio;
            weakChannelAmplitudeRatio = 1 - (PHASE_AMPLITUDE_RATIO_AT_90 * sinRatio);
        }
    }

    // if the bearing relative angle to source is > 0 then the delayed channel is the right one
    int delayedChannelOffset = (bearingRelativeAngleToSource > 0.0f) ? 1 : 0;
    int goodChannelOffset = delayedChannelOffset == 0 ? 1 : 0;
    
    const int16_t* nextOutputStart = bufferToAdd->getNextOutput();
   
    const int16_t* bufferStart = bufferToAdd->getBuffer();
    int ringBufferSampleCapacity = bufferToAdd->getSampleCapacity();

    int16_t correctBufferSample[2], delayBufferSample[2];
    int delayedChannelIndex = 0;
    
    const int SINGLE_STEREO_OFFSET = 2;
    
    for (int s = 0; s < NETWORK_BUFFER_LENGTH_SAMPLES_STEREO; s += 4) {
        
        // setup the int16_t variables for the two sample sets
        correctBufferSample[0] = nextOutputStart[s / 2] * attenuationCoefficient;
        correctBufferSample[1] = nextOutputStart[(s / 2) + 1] * attenuationCoefficient;
        
        delayedChannelIndex = s + (numSamplesDelay * 2) + delayedChannelOffset;
        
        delayBufferSample[0] = correctBufferSample[0] * weakChannelAmplitudeRatio;
        delayBufferSample[1] = correctBufferSample[1] * weakChannelAmplitudeRatio;
        
        __m64 bufferSamples = _mm_set_pi16(_clientSamples[s + goodChannelOffset],
                                           _clientSamples[s + goodChannelOffset + SINGLE_STEREO_OFFSET],
                                           _clientSamples[delayedChannelIndex],
                                           _clientSamples[delayedChannelIndex + SINGLE_STEREO_OFFSET]);
        __m64 addedSamples = _mm_set_pi16(correctBufferSample[0], correctBufferSample[1],
                                         delayBufferSample[0], delayBufferSample[1]);
        
        // perform the MMX add (with saturation) of two correct and delayed samples
        __m64 mmxResult = _mm_adds_pi16(bufferSamples, addedSamples);
        int16_t* shortResults = reinterpret_cast<int16_t*>(&mmxResult);
        
        // assign the results from the result of the mmx arithmetic
        _clientSamples[s + goodChannelOffset] = shortResults[3];
        _clientSamples[s + goodChannelOffset + SINGLE_STEREO_OFFSET] = shortResults[2];
        _clientSamples[delayedChannelIndex] = shortResults[1];
        _clientSamples[delayedChannelIndex + SINGLE_STEREO_OFFSET] = shortResults[0];
    }
    
    // The following code is pretty gross and redundant, but AFAIK it's the best way to avoid
    // too many conditionals in handling the delay samples at the beginning of _clientSamples.
    // Basically we try to take the samples in batches of four, and then handle the remainder
    // conditionally to get rid of the rest.
    
    const int DOUBLE_STEREO_OFFSET = 4;
    const int TRIPLE_STEREO_OFFSET = 6;
    
    if (numSamplesDelay > 0) {
        // if there was a sample delay for this buffer, we need to pull samples prior to the nextOutput
        // to stick at the beginning
        float attenuationAndWeakChannelRatio = attenuationCoefficient * weakChannelAmplitudeRatio;
        const int16_t* delayNextOutputStart = nextOutputStart - numSamplesDelay;
        if (delayNextOutputStart < bufferStart) {
            delayNextOutputStart = bufferStart + ringBufferSampleCapacity - numSamplesDelay;
        }
        
        int i = 0;
        
        while (i + 3 < numSamplesDelay) {
            // handle the first cases where we can MMX add four samples at once
            int parentIndex = i * 2;
            __m64 bufferSamples = _mm_set_pi16(_clientSamples[parentIndex + delayedChannelOffset],
                                               _clientSamples[parentIndex + SINGLE_STEREO_OFFSET + delayedChannelOffset],
                                               _clientSamples[parentIndex + DOUBLE_STEREO_OFFSET + delayedChannelOffset],
                                               _clientSamples[parentIndex + TRIPLE_STEREO_OFFSET + delayedChannelOffset]);
            __m64 addSamples = _mm_set_pi16(delayNextOutputStart[i] * attenuationAndWeakChannelRatio,
                                            delayNextOutputStart[i + 1] * attenuationAndWeakChannelRatio,
                                            delayNextOutputStart[i + 2] * attenuationAndWeakChannelRatio,
                                            delayNextOutputStart[i + 3] * attenuationAndWeakChannelRatio);
            __m64 mmxResult = _mm_adds_pi16(bufferSamples, addSamples);
            int16_t* shortResults = reinterpret_cast<int16_t*>(&mmxResult);
            
            _clientSamples[parentIndex + delayedChannelOffset] = shortResults[3];
            _clientSamples[parentIndex + SINGLE_STEREO_OFFSET + delayedChannelOffset] = shortResults[2];
            _clientSamples[parentIndex + DOUBLE_STEREO_OFFSET + delayedChannelOffset] = shortResults[1];
            _clientSamples[parentIndex + TRIPLE_STEREO_OFFSET + delayedChannelOffset] = shortResults[0];
            
            // push the index
            i += 4;
        }
        
        int parentIndex = i * 2;
        
        if (i + 2 < numSamplesDelay) {
            // MMX add only three delayed samples
            
            __m64 bufferSamples = _mm_set_pi16(_clientSamples[parentIndex + delayedChannelOffset],
                                               _clientSamples[parentIndex + SINGLE_STEREO_OFFSET + delayedChannelOffset],
                                               _clientSamples[parentIndex + DOUBLE_STEREO_OFFSET + delayedChannelOffset],
                                               0);
            __m64 addSamples = _mm_set_pi16(delayNextOutputStart[i] * attenuationAndWeakChannelRatio,
                                            delayNextOutputStart[i + 1] * attenuationAndWeakChannelRatio,
                                            delayNextOutputStart[i + 2] * attenuationAndWeakChannelRatio,
                                            0);
            __m64 mmxResult = _mm_adds_pi16(bufferSamples, addSamples);
            int16_t* shortResults = reinterpret_cast<int16_t*>(&mmxResult);
            
            _clientSamples[parentIndex + delayedChannelOffset] = shortResults[3];
            _clientSamples[parentIndex + SINGLE_STEREO_OFFSET + delayedChannelOffset] = shortResults[2];
            _clientSamples[parentIndex + DOUBLE_STEREO_OFFSET + delayedChannelOffset] = shortResults[1];
            
        } else if (i + 1 < numSamplesDelay) {
            // MMX add two delayed samples
            __m64 bufferSamples = _mm_set_pi16(_clientSamples[parentIndex + delayedChannelOffset],
                                               _clientSamples[parentIndex + SINGLE_STEREO_OFFSET + delayedChannelOffset], 0, 0);
            __m64 addSamples = _mm_set_pi16(delayNextOutputStart[i] * attenuationAndWeakChannelRatio,
                                            delayNextOutputStart[i + 1] * attenuationAndWeakChannelRatio, 0, 0);
            
            __m64 mmxResult = _mm_adds_pi16(bufferSamples, addSamples);
            int16_t* shortResults = reinterpret_cast<int16_t*>(&mmxResult);
            
            _clientSamples[parentIndex + delayedChannelOffset] = shortResults[3];
            _clientSamples[parentIndex + SINGLE_STEREO_OFFSET + delayedChannelOffset] = shortResults[2];
            
        } else if (i < numSamplesDelay) {
            // MMX add a single delayed sample
            __m64 bufferSamples = _mm_set_pi16(_clientSamples[parentIndex + delayedChannelOffset], 0, 0, 0);
            __m64 addSamples = _mm_set_pi16(delayNextOutputStart[i] * attenuationAndWeakChannelRatio, 0, 0, 0);
            
            __m64 mmxResult = _mm_adds_pi16(bufferSamples, addSamples);
            int16_t* shortResults = reinterpret_cast<int16_t*>(&mmxResult);
            
            _clientSamples[parentIndex + delayedChannelOffset] = shortResults[3];
        }
    }
}

void AudioMixer::prepareMixForListeningNode(Node* node) {
    AvatarAudioRingBuffer* nodeRingBuffer = ((AudioMixerClientData*) node->getLinkedData())->getAvatarAudioRingBuffer();

    // zero out the client mix for this node
    memset(_clientSamples, 0, NETWORK_BUFFER_LENGTH_BYTES_STEREO);

    // loop through all other nodes that have sufficient audio to mix
    foreach (const SharedNodePointer& otherNode, NodeList::getInstance()->getNodeHash()) {
        if (otherNode->getLinkedData()) {

            AudioMixerClientData* otherNodeClientData = (AudioMixerClientData*) otherNode->getLinkedData();

            // enumerate the ARBs attached to the otherNode and add all that should be added to mix
            for (unsigned int i = 0; i < otherNodeClientData->getRingBuffers().size(); i++) {
                PositionalAudioRingBuffer* otherNodeBuffer = otherNodeClientData->getRingBuffers()[i];

                if ((*otherNode != *node
                     || otherNodeBuffer->shouldLoopbackForNode())
                    && otherNodeBuffer->willBeAddedToMix()
                    && otherNodeBuffer->getAverageLoudness() > 0) {
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
                || mixerPacketType == PacketTypeInjectAudio
                || mixerPacketType == PacketTypeSilentAudioFrame) {
                
                nodeList->findNodeAndUpdateWithDataFromPacket(receivedPacket);
            } else {
                // let processNodeData handle it.
                nodeList->processNodeData(senderSockAddr, receivedPacket);
            }
        }
    }
}

void AudioMixer::run() {

    commonInit(AUDIO_MIXER_LOGGING_TARGET_NAME, NodeType::AudioMixer);

    NodeList* nodeList = NodeList::getInstance();

    nodeList->addNodeTypeToInterestSet(NodeType::Agent);

    nodeList->linkedDataCreateCallback = attachNewBufferToNode;

    int nextFrame = 0;
    timeval startTime;

    gettimeofday(&startTime, NULL);
    
    char* clientMixBuffer = new char[NETWORK_BUFFER_LENGTH_BYTES_STEREO
                                     + numBytesForPacketHeaderGivenPacketType(PacketTypeMixedAudio)];
    
    int usecToSleep = BUFFER_SEND_INTERVAL_USECS;
    float audabilityCutoffRatio = 0;
    
    const int TRAILING_AVERAGE_FRAMES = 100;
    int framesSinceCutoffEvent = TRAILING_AVERAGE_FRAMES;

    while (!_isFinished) {
        
        foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
            if (node->getLinkedData()) {
                ((AudioMixerClientData*) node->getLinkedData())->checkBuffersBeforeFrameSend(JITTER_BUFFER_SAMPLES);
            }
        }
        
        const float STRUGGLE_TRIGGER_SLEEP_PERCENTAGE_THRESHOLD = 0.10f;
        const float BACK_OFF_TRIGGER_SLEEP_PERCENTAGE_THRESHOLD = 0.20f;
        const float CUTOFF_EPSILON = 0.0001f;
        const float CUTOFF_DELTA = 0.02f;
        
        const float CURRENT_FRAME_RATIO = 1.0f / TRAILING_AVERAGE_FRAMES;
        const float PREVIOUS_FRAMES_RATIO = 1.0f - CURRENT_FRAME_RATIO;
        
        if (usecToSleep < 0) {
            usecToSleep = 0;
        }
        
        _trailingSleepRatio = (PREVIOUS_FRAMES_RATIO * _trailingSleepRatio)
            + (usecToSleep * CURRENT_FRAME_RATIO / (float) BUFFER_SEND_INTERVAL_USECS);
        
        float lastCutoffRatio = audabilityCutoffRatio;
        bool hasRatioChanged = false;
        
        if (framesSinceCutoffEvent >= TRAILING_AVERAGE_FRAMES) {
            if (_trailingSleepRatio <= STRUGGLE_TRIGGER_SLEEP_PERCENTAGE_THRESHOLD) {
                // we're struggling - change our min required loudness to reduce some load
                audabilityCutoffRatio += CUTOFF_DELTA;
                
                if (audabilityCutoffRatio >= 1) {
                    audabilityCutoffRatio = 1 - CUTOFF_DELTA;
                }
                
                qDebug() << "Mixer is struggling, sleeping" << _trailingSleepRatio * 100 << "% of frame time. Old cutoff was"
                    << lastCutoffRatio << "and is now" << audabilityCutoffRatio;
                hasRatioChanged = true;
            } else if (_trailingSleepRatio >= BACK_OFF_TRIGGER_SLEEP_PERCENTAGE_THRESHOLD && audabilityCutoffRatio != 0) {
                // we've recovered and can back off the required loudness
                audabilityCutoffRatio -= CUTOFF_DELTA;
                
                if (audabilityCutoffRatio < 0) {
                    audabilityCutoffRatio = 0;
                }
                
                qDebug() << "Mixer is recovering, sleeping" << _trailingSleepRatio * 100 << "% of frame time. Old cutoff was"
                    << lastCutoffRatio << "and is now" << audabilityCutoffRatio;
                hasRatioChanged = true;
            }
            
            if (hasRatioChanged) {
                // set out min audability threshold from the new ratio
                _minAudibilityThreshold = LOUDNESS_TO_DISTANCE_RATIO / (2.0f * (1.0f - audabilityCutoffRatio));
                qDebug() << "Minimum audability required to be mixed is now" << _minAudibilityThreshold;
                
                framesSinceCutoffEvent = 0;
            }
        } else {
            framesSinceCutoffEvent++;
        }

        foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
            if (node->getType() == NodeType::Agent && node->getActiveSocket() && node->getLinkedData()
                && ((AudioMixerClientData*) node->getLinkedData())->getAvatarAudioRingBuffer()) {
                prepareMixForListeningNode(node.data());
                
                int numBytesPacketHeader = populatePacketHeader(clientMixBuffer, PacketTypeMixedAudio);

                memcpy(clientMixBuffer + numBytesPacketHeader, _clientSamples, NETWORK_BUFFER_LENGTH_BYTES_STEREO);
                nodeList->writeDatagram(clientMixBuffer, NETWORK_BUFFER_LENGTH_BYTES_STEREO + numBytesPacketHeader, node);
            }
        }
        
        qDebug() << "There were" << _numClientsMixedInFrame << "clients mixed in the last frame";
        _numClientsMixedInFrame = 0;

        // push forward the next output pointers for any audio buffers we used
        foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
            if (node->getLinkedData()) {
                ((AudioMixerClientData*) node->getLinkedData())->pushBuffersAfterFrameSend();
            }
        }
        
        QCoreApplication::processEvents();
        
        if (_isFinished) {
            break;
        }

        usecToSleep = usecTimestamp(&startTime) + (++nextFrame * BUFFER_SEND_INTERVAL_USECS) - usecTimestampNow();

        if (usecToSleep > 0) {
            usleep(usecToSleep);
        }
    }
    
    delete[] clientMixBuffer;
}
