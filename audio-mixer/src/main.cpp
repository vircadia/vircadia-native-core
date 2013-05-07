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
#include <pthread.h>
#include <errno.h>
#include <fstream>
#include <limits>
#include <signal.h>

#include <AgentList.h>
#include <AgentTypes.h>
#include <SharedUtil.h>
#include <StdDev.h>
#include <Stacktrace.h>

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

const float DISTANCE_RATIO = 3.0f / 0.3f;
const float PHASE_AMPLITUDE_RATIO_AT_90 = 0.5;
const int PHASE_DELAY_AT_90 = 20;

const float MAX_OFF_AXIS_ATTENUATION = 0.5f;
const float OFF_AXIS_ATTENUATION_FORMULA_STEP = (1 - MAX_OFF_AXIS_ATTENUATION) / 2.0f;

void plateauAdditionOfSamples(int16_t &mixSample, int16_t sampleToAdd) {
    long sumSample = sampleToAdd + mixSample;
    
    long normalizedSample = std::min(MAX_SAMPLE_VALUE, sumSample);
    normalizedSample = std::max(MIN_SAMPLE_VALUE, sumSample);
    
    mixSample = normalizedSample;    
}

void *sendBuffer(void *args) {
    int sentBytes;
    int nextFrame = 0;
    timeval startTime;
    
    AgentList* agentList = AgentList::getInstance();
    
    gettimeofday(&startTime, NULL);

    while (true) {
        sentBytes = 0;
        
        for (AgentList::iterator agent = agentList->begin(); agent != agentList->end(); agent++) {
            AudioRingBuffer* agentBuffer = (AudioRingBuffer*) agent->getLinkedData();
            
            if (agentBuffer != NULL && agentBuffer->getEndOfLastWrite() != NULL) {
                
                if (!agentBuffer->isStarted()
                    && agentBuffer->diffLastWriteNextOutput() <= BUFFER_LENGTH_SAMPLES_PER_CHANNEL + JITTER_BUFFER_SAMPLES) {
                    printf("Held back buffer for agent with ID %d.\n", agent->getAgentId());
                    agentBuffer->setShouldBeAddedToMix(false);
                } else if (agentBuffer->diffLastWriteNextOutput() < BUFFER_LENGTH_SAMPLES_PER_CHANNEL) {
                    printf("Buffer from agent with ID %d starved.\n", agent->getAgentId());
                    agentBuffer->setStarted(false);
                    agentBuffer->setShouldBeAddedToMix(false);
                } else {
                    // good buffer, add this to the mix
                    agentBuffer->setStarted(true);
                    agentBuffer->setShouldBeAddedToMix(true);
                }
            }
        }
        
        int numAgents = agentList->size();
        float distanceCoefficients[numAgents][numAgents];
        memset(distanceCoefficients, 0, sizeof(distanceCoefficients));

        for (AgentList::iterator agent = agentList->begin(); agent != agentList->end(); agent++) {
            AudioRingBuffer* agentRingBuffer = (AudioRingBuffer*) agent->getLinkedData();
            float agentBearing = agentRingBuffer->getBearing();
            
            int16_t clientMix[BUFFER_LENGTH_SAMPLES_PER_CHANNEL * 2] = {};
            
            for (AgentList::iterator otherAgent = agentList->begin(); otherAgent != agentList->end(); otherAgent++) {
                if (otherAgent != agent || (otherAgent == agent && agentRingBuffer->shouldLoopbackForAgent())) {
                    AudioRingBuffer* otherAgentBuffer = (AudioRingBuffer*) otherAgent->getLinkedData();
                    
                    if (otherAgentBuffer->shouldBeAddedToMix()) {
                        float *agentPosition = agentRingBuffer->getPosition();
                        float *otherAgentPosition = otherAgentBuffer->getPosition();
                        
                        // calculate the distance to the other agent
                        
                        // use the distance to the other agent to calculate the change in volume for this frame
                        int lowAgentIndex = std::min(agent.getAgentIndex(), otherAgent.getAgentIndex());
                        int highAgentIndex = std::max(agent.getAgentIndex(), otherAgent.getAgentIndex());
                        
                        if (distanceCoefficients[lowAgentIndex][highAgentIndex] == 0) {
                            float distanceToAgent = sqrtf(powf(agentPosition[0] - otherAgentPosition[0], 2) +
                                                          powf(agentPosition[1] - otherAgentPosition[1], 2) +
                                                          powf(agentPosition[2] - otherAgentPosition[2], 2));
                            
                            float minCoefficient = std::min(1.0f,
                                                            powf(0.5, (logf(DISTANCE_RATIO * distanceToAgent) / logf(3)) - 1));
                            distanceCoefficients[lowAgentIndex][highAgentIndex] = minCoefficient;
                        }
                        
                        
                        // get the angle from the right-angle triangle
                        float triangleAngle = atan2f(fabsf(agentPosition[2] - otherAgentPosition[2]),
                                                     fabsf(agentPosition[0] - otherAgentPosition[0])) * (180 / M_PI);
                        float absoluteAngleToSource = 0;
                        float bearingRelativeAngleToSource = 0;
                        
                        
                        // find the angle we need for calculation based on the orientation of the triangle
                        if (otherAgentPosition[0] > agentPosition[0]) {
                            if (otherAgentPosition[2] > agentPosition[2]) {
                                absoluteAngleToSource = -90 + triangleAngle;
                            } else {
                                absoluteAngleToSource = -90 - triangleAngle;
                            }
                        } else {
                            if (otherAgentPosition[2] > agentPosition[2]) {
                                absoluteAngleToSource = 90 - triangleAngle;
                            } else {
                                absoluteAngleToSource = 90 + triangleAngle;
                            }
                        }
                        
                        if (absoluteAngleToSource > 180) {
                            absoluteAngleToSource -= 360;
                        } else if (absoluteAngleToSource < -180) {
                            absoluteAngleToSource += 360;
                        }
                        
                        bearingRelativeAngleToSource = absoluteAngleToSource - agentBearing;
                        bearingRelativeAngleToSource *= (M_PI / 180);
                        
                        float angleOfDelivery = absoluteAngleToSource - otherAgentBuffer->getBearing();
                        
                        if (angleOfDelivery < -180) {
                            angleOfDelivery += 360;
                        }
                        
                        float offAxisCoefficient = MAX_OFF_AXIS_ATTENUATION +
                            (OFF_AXIS_ATTENUATION_FORMULA_STEP * (fabsf(angleOfDelivery) / 90.0f));
                        
                        float attenuationCoefficient = distanceCoefficients[lowAgentIndex][highAgentIndex]
                            * otherAgentBuffer->getAttenuationRatio()
                            * offAxisCoefficient;
                        
                        float sinRatio = fabsf(sinf(bearingRelativeAngleToSource));
                        int numSamplesDelay = PHASE_DELAY_AT_90 * sinRatio;
                        float weakChannelAmplitudeRatio = 1 - (PHASE_AMPLITUDE_RATIO_AT_90 * sinRatio);
                        
                        int16_t* goodChannel = bearingRelativeAngleToSource > 0  ? clientMix + BUFFER_LENGTH_SAMPLES_PER_CHANNEL : clientMix;
                        int16_t* delayedChannel = bearingRelativeAngleToSource > 0 ? clientMix : clientMix + BUFFER_LENGTH_SAMPLES_PER_CHANNEL;
                        
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
                                                         currentSample *
                                                         weakChannelAmplitudeRatio);
                            }
                        }
                    }
                }
            }
            
            agentList->getAgentSocket().send(agent->getPublicSocket(), clientMix, BUFFER_LENGTH_BYTES);
        }
        
        for (AgentList::iterator agent = agentList->begin(); agent != agentList->end(); agent++) {
            AudioRingBuffer* agentBuffer = (AudioRingBuffer*) agent->getLinkedData();
            if (agentBuffer->shouldBeAddedToMix()) {
                agentBuffer->setNextOutput(agentBuffer->getNextOutput() + BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
                
                if (agentBuffer->getNextOutput() >= agentBuffer->getBuffer() + RING_BUFFER_SAMPLES) {
                    agentBuffer->setNextOutput(agentBuffer->getBuffer());
                }
                
                agentBuffer->setShouldBeAddedToMix(false);
            }
        }
        
        double usecToSleep = usecTimestamp(&startTime) + (++nextFrame * BUFFER_SEND_INTERVAL_USECS) - usecTimestampNow();
        
        if (usecToSleep > 0) {
            usleep(usecToSleep);
        } else {
            std::cout << "Took too much time, not sleeping!\n";
        }
    }  

    pthread_exit(0);  
}

void attachNewBufferToAgent(Agent *newAgent) {
    if (newAgent->getLinkedData() == NULL) {
        newAgent->setLinkedData(new AudioRingBuffer(RING_BUFFER_SAMPLES, BUFFER_LENGTH_SAMPLES_PER_CHANNEL));
    }
}

int main(int argc, const char* argv[]) {
    signal(SIGSEGV, printStacktrace);
    setvbuf(stdout, NULL, _IOLBF, 0);
    
    AgentList* agentList = AgentList::createInstance(AGENT_TYPE_AUDIO_MIXER, MIXER_LISTEN_PORT);
    
    ssize_t receivedBytes = 0;
    
    agentList->linkedDataCreateCallback = attachNewBufferToAgent;
    
    agentList->startSilentAgentRemovalThread();
    agentList->startDomainServerCheckInThread();

    unsigned char *packetData = new unsigned char[MAX_PACKET_SIZE];

    pthread_t sendBufferThread;
    pthread_create(&sendBufferThread, NULL, sendBuffer, NULL);
    
    sockaddr *agentAddress = new sockaddr;
    
    while (true) {
        if(agentList->getAgentSocket().receive(agentAddress, packetData, &receivedBytes)) {
            if (packetData[0] == PACKET_HEADER_INJECT_AUDIO) {
                
                if (agentList->addOrUpdateAgent(agentAddress, agentAddress, packetData[0], agentList->getLastAgentId())) {
                    agentList->increaseAgentId();
                }
                
                agentList->updateAgentWithData(agentAddress, packetData, receivedBytes);
            }
        }
    }
    
    pthread_join(sendBufferThread, NULL);
    
    return 0;
}
