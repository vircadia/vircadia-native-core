//
//  main.cpp
//  mixer
//
//  Created by Stephen Birarda on 2/1/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <iostream>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <fstream>
#include <limits>
#include <signal.h>

#include <glm/gtx/norm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <AgentList.h>
#include <Agent.h>
#include <AgentTypes.h>
#include <SharedUtil.h>
#include <StdDev.h>

#include "AudioRingBuffer.h"
#include "PacketHeaders.h"

#ifdef _WIN32
#include "Syssocket.h"
#include "Systime.h"
#include <math.h>
#else
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif //_WIN32

const unsigned short MIXER_LISTEN_PORT = 55443;

const float SAMPLE_RATE = 22050.0;

const short JITTER_BUFFER_MSECS = 12;
const short JITTER_BUFFER_SAMPLES = JITTER_BUFFER_MSECS * (SAMPLE_RATE / 1000.0);

const int BUFFER_LENGTH_BYTES = 1024;
const int BUFFER_LENGTH_SAMPLES_PER_CHANNEL = (BUFFER_LENGTH_BYTES / 2) / sizeof(int16_t);

const short RING_BUFFER_FRAMES = 10;
const short RING_BUFFER_SAMPLES = RING_BUFFER_FRAMES * BUFFER_LENGTH_SAMPLES_PER_CHANNEL;

const float BUFFER_SEND_INTERVAL_USECS = (BUFFER_LENGTH_SAMPLES_PER_CHANNEL / SAMPLE_RATE) * 1000000;

const long MAX_SAMPLE_VALUE = std::numeric_limits<int16_t>::max();
const long MIN_SAMPLE_VALUE = std::numeric_limits<int16_t>::min();

const float PHASE_AMPLITUDE_RATIO_AT_90 = 0.5;
const int PHASE_DELAY_AT_90 = 20;

const float MAX_OFF_AXIS_ATTENUATION = 0.2f;
const float OFF_AXIS_ATTENUATION_FORMULA_STEP = (1 - MAX_OFF_AXIS_ATTENUATION) / 2.0f;

void plateauAdditionOfSamples(int16_t &mixSample, int16_t sampleToAdd) {
    long sumSample = sampleToAdd + mixSample;
    
    long normalizedSample = std::min(MAX_SAMPLE_VALUE, sumSample);
    normalizedSample = std::max(MIN_SAMPLE_VALUE, sumSample);
    
    mixSample = normalizedSample;    
}

void attachNewBufferToAgent(Agent *newAgent) {
    if (!newAgent->getLinkedData()) {
        newAgent->setLinkedData(new AudioRingBuffer(RING_BUFFER_SAMPLES, BUFFER_LENGTH_SAMPLES_PER_CHANNEL));
    }
}

int main(int argc, const char* argv[]) {
    setvbuf(stdout, NULL, _IOLBF, 0);
    
    AgentList* agentList = AgentList::createInstance(AGENT_TYPE_AUDIO_MIXER, MIXER_LISTEN_PORT);
    
    ssize_t receivedBytes = 0;
    
    agentList->linkedDataCreateCallback = attachNewBufferToAgent;
    
    agentList->startSilentAgentRemovalThread();
    agentList->startDomainServerCheckInThread();

    unsigned char* packetData = new unsigned char[MAX_PACKET_SIZE];

    sockaddr* agentAddress = new sockaddr;

    // make sure our agent socket is non-blocking
    agentList->getAgentSocket()->setBlocking(false);
    
    int nextFrame = 0;
    timeval startTime;
    
    unsigned char clientPacket[BUFFER_LENGTH_BYTES + 1];
    clientPacket[0] = PACKET_HEADER_MIXED_AUDIO;
    
    int16_t clientSamples[BUFFER_LENGTH_SAMPLES_PER_CHANNEL * 2] = {};
    
    gettimeofday(&startTime, NULL);
    
    while (true) {
        // enumerate the agents, check if we can add audio from the agent to current mix
        for (AgentList::iterator agent = agentList->begin(); agent != agentList->end(); agent++) {
            AudioRingBuffer* agentBuffer = (AudioRingBuffer*) agent->getLinkedData();
            
            if (agentBuffer->getEndOfLastWrite()) {
                if (!agentBuffer->isStarted()
                    && agentBuffer->diffLastWriteNextOutput() <= BUFFER_LENGTH_SAMPLES_PER_CHANNEL + JITTER_BUFFER_SAMPLES) {
                    printf("Held back buffer for agent with ID %d.\n", agent->getAgentID());
                    agentBuffer->setShouldBeAddedToMix(false);
                } else if (agentBuffer->diffLastWriteNextOutput() < BUFFER_LENGTH_SAMPLES_PER_CHANNEL) {
                    printf("Buffer from agent with ID %d starved.\n", agent->getAgentID());
                    agentBuffer->setStarted(false);
                    agentBuffer->setShouldBeAddedToMix(false);
                } else {
                    // good buffer, add this to the mix
                    agentBuffer->setStarted(true);
                    agentBuffer->setShouldBeAddedToMix(true);
                }
            }
        }
        
        for (AgentList::iterator agent = agentList->begin(); agent != agentList->end(); agent++) {
            if (agent->getType() == AGENT_TYPE_AVATAR) {
                AudioRingBuffer* agentRingBuffer = (AudioRingBuffer*) agent->getLinkedData();
                
                // zero out the client mix for this agent
                memset(clientSamples, 0, sizeof(clientSamples));
                
                for (AgentList::iterator otherAgent = agentList->begin(); otherAgent != agentList->end(); otherAgent++) {
                    if (otherAgent != agent || (otherAgent == agent && agentRingBuffer->shouldLoopbackForAgent())) {
                        AudioRingBuffer* otherAgentBuffer = (AudioRingBuffer*) otherAgent->getLinkedData();
                        
                        if (otherAgentBuffer->shouldBeAddedToMix()) {
                            
                            float bearingRelativeAngleToSource = 0.f;
                            float attenuationCoefficient = 1.0f;
                            int numSamplesDelay = 0;
                            float weakChannelAmplitudeRatio = 1.0f;
                            
                            if (otherAgent != agent) {
                                glm::vec3 listenerPosition = agentRingBuffer->getPosition();
                                glm::vec3 relativePosition = otherAgentBuffer->getPosition() - agentRingBuffer->getPosition();
                                glm::quat inverseOrientation = glm::inverse(agentRingBuffer->getOrientation());
                                glm::vec3 rotatedSourcePosition = inverseOrientation * relativePosition;
                            
                                float distanceSquareToSource = glm::dot(relativePosition, relativePosition);
                            
                                float distanceCoefficient = 1.0f;
                                float offAxisCoefficient = 1.0f;

                                if (otherAgentBuffer->getRadius() == 0
                                    || (distanceSquareToSource > (otherAgentBuffer->getRadius()
                                                                 * otherAgentBuffer->getRadius()))) {
                                    // this is either not a spherical source, or the listener is outside the sphere
                                    
                                    if (otherAgentBuffer->getRadius() > 0) {
                                        // this is a spherical source - the distance used for the coefficient
                                        // needs to be the closest point on the boundary to the source
                                        
                                        // multiply the normalized vector between the center of the sphere
                                        // and the position of the source by the radius to get the
                                        // closest point on the boundary of the sphere to the source
                                        
                                        glm::vec3 closestPoint = glm::normalize(-relativePosition) * otherAgentBuffer->getRadius();

                                        // for the other calculations the agent position is the closest point on the sphere
                                        rotatedSourcePosition = inverseOrientation * -closestPoint;

                                        // ovveride the distance to the agent with the distance to the point on the
                                        // boundary of the sphere
                                        distanceSquareToSource = glm::distance2(listenerPosition, closestPoint);
                                        
                                    } else {
                                        // calculate the angle delivery
                                        glm::vec3 rotatedListenerPosition = glm::inverse(otherAgentBuffer->getOrientation())
                                        * relativePosition;
                                        
                                        float angleOfDelivery = glm::angle(glm::vec3(0.0f, 0.0f, 1.0f),
                                                                           glm::normalize(rotatedListenerPosition));
                                        
                                        offAxisCoefficient = MAX_OFF_AXIS_ATTENUATION +
                                            (OFF_AXIS_ATTENUATION_FORMULA_STEP * (angleOfDelivery / 90.0f));
                                    }
                                    
                                    const float DISTANCE_SCALE = 2.5f;
                                    const float GEOMETRIC_AMPLITUDE_SCALAR = 0.3f;
                                    const float DISTANCE_LOG_BASE = 2.5f;
                                    const float DISTANCE_SCALE_LOG = logf(DISTANCE_SCALE) / logf(DISTANCE_LOG_BASE);
                                    
                                    // calculate the distance coefficient using the distance to this agent
                                    distanceCoefficient = powf(GEOMETRIC_AMPLITUDE_SCALAR,
                                                               DISTANCE_SCALE_LOG +
                                                               (logf(distanceSquareToSource) / logf(DISTANCE_LOG_BASE)) - 1);
                                    distanceCoefficient = std::min(1.0f, distanceCoefficient);
                                    
                                    // off-axis attenuation and spatialization of audio
                                    // not performed if listener is inside spherical injector
                                    
                                    // calculate the angle from the source to the listener
                                    
                                    // project the rotated source position vector onto the XZ plane
                                    rotatedSourcePosition.y = 0.0f;
                                    
                                    // produce an oriented angle about the y-axis
                                    bearingRelativeAngleToSource = glm::orientedAngle(glm::vec3(0.0f, 0.0f, -1.0f),
                                                                                      glm::normalize(rotatedSourcePosition),
                                                                                      glm::vec3(0.0f, 1.0f, 0.0f));
                                    
                                    
                                    float sinRatio = fabsf(sinf(glm::radians(bearingRelativeAngleToSource)));
                                    numSamplesDelay = PHASE_DELAY_AT_90 * sinRatio;
                                    weakChannelAmplitudeRatio = 1 - (PHASE_AMPLITUDE_RATIO_AT_90 * sinRatio);
                                }
                                
                                attenuationCoefficient = distanceCoefficient
                                    * otherAgentBuffer->getAttenuationRatio()
                                    * offAxisCoefficient;
                            }
                            
                            int16_t* goodChannel = bearingRelativeAngleToSource > 0.0f
                                ? clientSamples
                                : clientSamples + BUFFER_LENGTH_SAMPLES_PER_CHANNEL;
                            int16_t* delayedChannel = bearingRelativeAngleToSource > 0.0f
                                ? clientSamples + BUFFER_LENGTH_SAMPLES_PER_CHANNEL
                                : clientSamples;
                            
                            int16_t* delaySamplePointer = otherAgentBuffer->getNextOutput() == otherAgentBuffer->getBuffer()
                                ? otherAgentBuffer->getBuffer() + RING_BUFFER_SAMPLES - numSamplesDelay
                                : otherAgentBuffer->getNextOutput() - numSamplesDelay;
                            
                            for (int s = 0; s < BUFFER_LENGTH_SAMPLES_PER_CHANNEL; s++) {
                                
                                if (s < numSamplesDelay) {
                                    // pull the earlier sample for the delayed channel
                                    int earlierSample = delaySamplePointer[s] * attenuationCoefficient;
                                    plateauAdditionOfSamples(delayedChannel[s], earlierSample * weakChannelAmplitudeRatio);
                                }
                                
                                int16_t currentSample = (otherAgentBuffer->getNextOutput()[s] * attenuationCoefficient);
                                plateauAdditionOfSamples(goodChannel[s], currentSample);
                                
                                if (s + numSamplesDelay < BUFFER_LENGTH_SAMPLES_PER_CHANNEL) {                                    
                                    plateauAdditionOfSamples(delayedChannel[s + numSamplesDelay],
                                                             currentSample * weakChannelAmplitudeRatio);
                                }
                            }
                        }
                    }
                }
                
                memcpy(clientPacket + 1, clientSamples, sizeof(clientSamples));
                agentList->getAgentSocket()->send(agent->getPublicSocket(), clientPacket, BUFFER_LENGTH_BYTES + 1);
            }
        }
        
        // push forward the next output pointers for any audio buffers we used
        for (AgentList::iterator agent = agentList->begin(); agent != agentList->end(); agent++) {
            AudioRingBuffer* agentBuffer = (AudioRingBuffer*) agent->getLinkedData();
            if (agentBuffer && agentBuffer->shouldBeAddedToMix()) {
                agentBuffer->setNextOutput(agentBuffer->getNextOutput() + BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
                
                if (agentBuffer->getNextOutput() >= agentBuffer->getBuffer() + RING_BUFFER_SAMPLES) {
                    agentBuffer->setNextOutput(agentBuffer->getBuffer());
                }
                
                agentBuffer->setShouldBeAddedToMix(false);
            }
        }
        
        // pull any new audio data from agents off of the network stack
        while (agentList->getAgentSocket()->receive(agentAddress, packetData, &receivedBytes)) {
            if (packetData[0] == PACKET_HEADER_MICROPHONE_AUDIO) {
                Agent* avatarAgent = agentList->addOrUpdateAgent(agentAddress,
                                                                 agentAddress,
                                                                 AGENT_TYPE_AVATAR,
                                                                 agentList->getLastAgentID());
                
                if (avatarAgent->getAgentID() == agentList->getLastAgentID()) {
                    agentList->increaseAgentID();
                }
                
                agentList->updateAgentWithData(agentAddress, packetData, receivedBytes);
                
                if (std::isnan(((AudioRingBuffer *)avatarAgent->getLinkedData())->getOrientation().x)) {
                    // kill off this agent - temporary solution to mixer crash on mac sleep
                    avatarAgent->setAlive(false);
                }
            } else if (packetData[0] == PACKET_HEADER_INJECT_AUDIO) {
                Agent* matchingInjector = NULL;
                
                for (AgentList::iterator agent = agentList->begin(); agent != agentList->end(); agent++) {
                    if (agent->getLinkedData()) {
                       
                        AudioRingBuffer* ringBuffer = (AudioRingBuffer*) agent->getLinkedData();
                        if (memcmp(ringBuffer->getStreamIdentifier(),
                                   packetData + 1,
                                   STREAM_IDENTIFIER_NUM_BYTES) == 0) {
                            // this is the matching stream, assign to matchingInjector and stop looking
                            matchingInjector = &*agent;
                            break;
                        }
                    }
                }
                
                if (!matchingInjector) {
                    matchingInjector = agentList->addOrUpdateAgent(NULL,
                                                                   NULL,
                                                                   AGENT_TYPE_AUDIO_INJECTOR,
                                                                   agentList->getLastAgentID());
                    agentList->increaseAgentID();
                    
                }
                
                // give the new audio data to the matching injector agent
                agentList->updateAgentWithData(matchingInjector, packetData, receivedBytes);
            }
        }
        
        double usecToSleep = usecTimestamp(&startTime) + (++nextFrame * BUFFER_SEND_INTERVAL_USECS) - usecTimestampNow();
        
        if (usecToSleep > 0) {
            usleep(usecToSleep);
        } else {
            std::cout << "Took too much time, not sleeping!\n";
        }
    }
    
    return 0;
}
