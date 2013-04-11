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
#include <AgentList.h>
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

const float DISTANCE_RATIO = 3.0/4.2;
const float PHASE_AMPLITUDE_RATIO_AT_90 = 0.5;
const int PHASE_DELAY_AT_90 = 20;


const int AGENT_LOOPBACK_MODIFIER = 307;

const int LOOPBACK_SANITY_CHECK = 0;

StDev stdev;

void plateauAdditionOfSamples(int16_t &mixSample, int16_t sampleToAdd) {
    long sumSample = sampleToAdd + mixSample;
    
    long normalizedSample = std::min(MAX_SAMPLE_VALUE, sumSample);
    normalizedSample = std::max(MIN_SAMPLE_VALUE, sumSample);
    
    mixSample = normalizedSample;    
}

void *sendBuffer(void *args)
{
    int sentBytes;
    int nextFrame = 0;
    timeval startTime;
    
    AgentList *agentList = AgentList::getInstance();
    
    gettimeofday(&startTime, NULL);

    while (true) {
        sentBytes = 0;
        
        for (int i = 0; i < agentList->getAgents().size(); i++) {
            AudioRingBuffer *agentBuffer = (AudioRingBuffer *) agentList->getAgents()[i].getLinkedData();
            
            if (agentBuffer != NULL && agentBuffer->getEndOfLastWrite() != NULL) {
                
                if (!agentBuffer->isStarted()
                    && agentBuffer->diffLastWriteNextOutput() <= BUFFER_LENGTH_SAMPLES_PER_CHANNEL + JITTER_BUFFER_SAMPLES) {
                    printf("Held back buffer %d.\n", i);
                } else if (agentBuffer->diffLastWriteNextOutput() < BUFFER_LENGTH_SAMPLES_PER_CHANNEL) {
                    printf("Buffer %d starved.\n", i);
                    agentBuffer->setStarted(false);
                } else {
                    // good buffer, add this to the mix
                    agentBuffer->setStarted(true);
                    agentBuffer->setAddedToMix(true);
                }
            }
        }
        
        int numAgents = agentList->getAgents().size();
        float distanceCoeffs[numAgents][numAgents];
        memset(distanceCoeffs, 0, sizeof(distanceCoeffs));

        for (int i = 0; i < agentList->getAgents().size(); i++) {
            Agent *agent = &agentList->getAgents()[i];
            
            AudioRingBuffer *agentRingBuffer = (AudioRingBuffer *) agent->getLinkedData();
            float agentBearing = agentRingBuffer->getBearing();
            bool agentWantsLoopback = false;
            
            if (agentBearing > 180 || agentBearing < -180) {
                // we were passed an invalid bearing because this agent wants loopback (pressed the H key)
                agentWantsLoopback = true;
                
                // correct the bearing
                agentBearing = agentBearing > 0
                    ? agentBearing - AGENT_LOOPBACK_MODIFIER
                    : agentBearing + AGENT_LOOPBACK_MODIFIER;
            }
            
            int16_t clientMix[BUFFER_LENGTH_SAMPLES_PER_CHANNEL * 2] = {};
            
            
            for (int j = 0; j < agentList->getAgents().size(); j++) {
                if (i != j || ( i == j && agentWantsLoopback)) {
                    AudioRingBuffer *otherAgentBuffer = (AudioRingBuffer *)agentList->getAgents()[j].getLinkedData();
                    
                    float *agentPosition = agentRingBuffer->getPosition();
                    float *otherAgentPosition = otherAgentBuffer->getPosition();
                   
                    // calculate the distance to the other agent
                    
                    // use the distance to the other agent to calculate the change in volume for this frame
                    int lowAgentIndex = std::min(i, j);
                    int highAgentIndex = std::max(i, j);
                    
                    if (distanceCoeffs[lowAgentIndex][highAgentIndex] == 0) {
                        float distanceToAgent = sqrtf(powf(agentPosition[0] - otherAgentPosition[0], 2) +
                                                      powf(agentPosition[1] - otherAgentPosition[1], 2) +
                                                      powf(agentPosition[2] - otherAgentPosition[2], 2));
                        
                        float minCoefficient = std::min(1.0f,
                                                        powf(0.5, (logf(DISTANCE_RATIO * distanceToAgent) / logf(3)) - 1));
                        distanceCoeffs[lowAgentIndex][highAgentIndex] = minCoefficient;
                    }
                    
                    
                    // get the angle from the right-angle triangle
                    float triangleAngle = atan2f(fabsf(agentPosition[2] - otherAgentPosition[2]),
                                                 fabsf(agentPosition[0] - otherAgentPosition[0])) * (180 / M_PI);
                    float angleToSource;
                    
                    
                    // find the angle we need for calculation based on the orientation of the triangle
                    if (otherAgentPosition[0] > agentPosition[0]) {
                        if (otherAgentPosition[2] > agentPosition[2]) {
                            angleToSource = -90 + triangleAngle - agentBearing;
                        } else {
                            angleToSource = -90 - triangleAngle - agentBearing;
                        }
                    } else {
                        if (otherAgentPosition[2] > agentPosition[2]) {
                            angleToSource = 90 - triangleAngle - agentBearing;
                        } else {
                            angleToSource = 90 + triangleAngle - agentBearing;
                        }
                    }
                    
                    if (angleToSource > 180) {
                        angleToSource -= 360;
                    } else if (angleToSource < -180) {
                        angleToSource += 360;
                    }
                    
                    angleToSource *= (M_PI / 180);
                    
                    float sinRatio = fabsf(sinf(angleToSource));
                    int numSamplesDelay = PHASE_DELAY_AT_90 * sinRatio;
                    float weakChannelAmplitudeRatio = 1 - (PHASE_AMPLITUDE_RATIO_AT_90 * sinRatio);
                    
                    int16_t *goodChannel = angleToSource > 0  ? clientMix + BUFFER_LENGTH_SAMPLES_PER_CHANNEL : clientMix;
                    int16_t *delayedChannel = angleToSource > 0 ? clientMix : clientMix + BUFFER_LENGTH_SAMPLES_PER_CHANNEL;
                    
                    int16_t *delaySamplePointer = otherAgentBuffer->getNextOutput() == otherAgentBuffer->getBuffer()
                        ? otherAgentBuffer->getBuffer() + RING_BUFFER_SAMPLES - numSamplesDelay
                        : otherAgentBuffer->getNextOutput() - numSamplesDelay;
                    
                    
                    for (int s = 0; s < BUFFER_LENGTH_SAMPLES_PER_CHANNEL; s++) {
                        
                        if (s < numSamplesDelay) {
                            // pull the earlier sample for the delayed channel
                            
                            int earlierSample = delaySamplePointer[s] *
                                                distanceCoeffs[lowAgentIndex][highAgentIndex] *
                                                otherAgentBuffer->getAttenuationRatio();
                            
                            plateauAdditionOfSamples(delayedChannel[s], earlierSample * weakChannelAmplitudeRatio);
                        }
                        
                        int16_t currentSample = (otherAgentBuffer->getNextOutput()[s] *
                                                 distanceCoeffs[lowAgentIndex][highAgentIndex] *
                                                 otherAgentBuffer->getAttenuationRatio());
                        plateauAdditionOfSamples(goodChannel[s], currentSample);
                        
                        if (s + numSamplesDelay < BUFFER_LENGTH_SAMPLES_PER_CHANNEL) {
                            plateauAdditionOfSamples(delayedChannel[s + numSamplesDelay],
                                                     currentSample *
                                                     weakChannelAmplitudeRatio *
                                                     otherAgentBuffer->getAttenuationRatio());
                        }
                    }
                }
            }
            
            agentList->getAgentSocket().send(agent->getPublicSocket(), clientMix, BUFFER_LENGTH_BYTES);
        }
        
        for (int i = 0; i < agentList->getAgents().size(); i++) {
            AudioRingBuffer *agentBuffer = (AudioRingBuffer *)agentList->getAgents()[i].getLinkedData();
            if (agentBuffer->wasAddedToMix()) {
                agentBuffer->setNextOutput(agentBuffer->getNextOutput() + BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
                
                if (agentBuffer->getNextOutput() >= agentBuffer->getBuffer() + RING_BUFFER_SAMPLES) {
                    agentBuffer->setNextOutput(agentBuffer->getBuffer());
                }
                
                agentBuffer->setAddedToMix(false);
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

int main(int argc, const char * argv[])
{
    AgentList *agentList = AgentList::createInstance(AGENT_TYPE_AUDIO_MIXER, MIXER_LISTEN_PORT);
    setvbuf(stdout, NULL, _IOLBF, 0);
    
    ssize_t receivedBytes = 0;
    
    agentList->linkedDataCreateCallback = attachNewBufferToAgent;
    
    agentList->startSilentAgentRemovalThread();
    agentList->startDomainServerCheckInThread();

    unsigned char *packetData = new unsigned char[MAX_PACKET_SIZE];

    pthread_t sendBufferThread;
    pthread_create(&sendBufferThread, NULL, sendBuffer, NULL);
    
    int16_t *loopbackAudioPacket;
    if (LOOPBACK_SANITY_CHECK) {
        loopbackAudioPacket = new int16_t[1024];
    }
    
    sockaddr *agentAddress = new sockaddr;
    timeval lastReceive;
    gettimeofday(&lastReceive, NULL);
    
    bool firstSample = true;

    while (true) {
        if(agentList->getAgentSocket().receive(agentAddress, packetData, &receivedBytes)) {
            if (packetData[0] == PACKET_HEADER_INJECT_AUDIO) {
                                
                //  Compute and report standard deviation for jitter calculation
                if (firstSample) {
                    stdev.reset();
                    firstSample = false;
                } else {
                    double tDiff = (usecTimestampNow() - usecTimestamp(&lastReceive)) / 1000;
                    stdev.addValue(tDiff);
                    
                    if (stdev.getSamples() > 500) {
                        //printf("Avg: %4.2f, Stdev: %4.2f\n", stdev.getAverage(), stdev.getStDev());
                        stdev.reset();
                    }
                }
                
                gettimeofday(&lastReceive, NULL);
                
                // add or update the existing interface agent
                if (!LOOPBACK_SANITY_CHECK) {
                    
                    if (agentList->addOrUpdateAgent(agentAddress, agentAddress, packetData[0], agentList->getLastAgentId())) {
                        agentList->increaseAgentId();
                    }
                    
                    agentList->updateAgentWithData(agentAddress, (void *)packetData, receivedBytes);
                } else {
                    memcpy(loopbackAudioPacket, packetData + 1 + (sizeof(float) * 4), 1024);
                    agentList->getAgentSocket().send(agentAddress, loopbackAudioPacket, 1024);
                }
            }
        }
    }
    
    pthread_join(sendBufferThread, NULL);
    
    return 0;
}
