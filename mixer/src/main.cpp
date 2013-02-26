#include <iostream>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>
#include <fstream>
#include <limits>
#include "AudioRingBuffer.h"
#include <AgentList.h>
#include <SharedUtil.h>

const int MAX_AGENTS = 1000;
const int LOGOFF_CHECK_INTERVAL = 1000;

const unsigned short MIXER_LISTEN_PORT = 55443;


const float SAMPLE_RATE = 22050.0;
const float BUFFER_SEND_INTERVAL_USECS = (BUFFER_LENGTH_SAMPLES/SAMPLE_RATE) * 1000000;

const short JITTER_BUFFER_MSECS = 20;
const short JITTER_BUFFER_SAMPLES = JITTER_BUFFER_MSECS * (SAMPLE_RATE / 1000.0);

const long MAX_SAMPLE_VALUE = std::numeric_limits<int16_t>::max();
const long MIN_SAMPLE_VALUE = std::numeric_limits<int16_t>::min();

char DOMAIN_HOSTNAME[] = "highfidelity.below92.com";
char DOMAIN_IP[100] = "";    //  IP Address will be re-set by lookup on startup
const int DOMAINSERVER_PORT = 40102;

const int MAX_SOURCE_BUFFERS = 20;

AgentList agentList(MIXER_LISTEN_PORT);

void *sendBuffer(void *args)
{
    int sentBytes;
    int nextFrame = 0;
    timeval startTime;

    int16_t *clientMix = new int16_t[BUFFER_LENGTH_SAMPLES];
    long *masterMix = new long[BUFFER_LENGTH_SAMPLES];
    
    gettimeofday(&startTime, NULL);

    while (true) {
        sentBytes = 0;
        
        for (int ms = 0; ms < BUFFER_LENGTH_SAMPLES; ms++) {
            masterMix[ms] = 0;
        }

        for (int ab = 0; ab < agentList.getAgents().size(); ab++) {
            AudioRingBuffer *agentBuffer = (AudioRingBuffer *)agentList.getAgents()[ab].getLinkedData();
            
            if (agentBuffer != NULL && agentBuffer->getEndOfLastWrite() != NULL) {
                if (!agentBuffer->isStarted() && agentBuffer->diffLastWriteNextOutput() <= BUFFER_LENGTH_SAMPLES + JITTER_BUFFER_SAMPLES) {
                    printf("Held back buffer %d.\n", ab);
                } else if (agentBuffer->diffLastWriteNextOutput() < BUFFER_LENGTH_SAMPLES) {
                    printf("Buffer %d starved.\n", ab);
                    agentBuffer->setStarted(false);
                } else {
                    // good buffer, add this to the mix
                    agentBuffer->setStarted(true);
                    agentBuffer->setAddedToMix(true);
                    
                    for (int s = 0; s < BUFFER_LENGTH_SAMPLES; s++) {
                        masterMix[s] += agentBuffer->getNextOutput()[s];
                    }
                    
                    agentBuffer->setNextOutput(agentBuffer->getNextOutput() + BUFFER_LENGTH_SAMPLES);
                    
                    if (agentBuffer->getNextOutput() >= agentBuffer->getBuffer() + RING_BUFFER_SAMPLES) {
                        agentBuffer->setNextOutput(agentBuffer->getBuffer());
                    }
                }
            }
        }
        
        for (int ab = 0; ab < agentList.getAgents().size(); ab++) {
            Agent *agent = &agentList.getAgents()[ab];
            AudioRingBuffer *agentBuffer = (AudioRingBuffer *)agent->getLinkedData();

            int16_t *previousOutput = NULL;

            if (agentBuffer != NULL && agentBuffer->wasAddedToMix()) {
                previousOutput = (agentBuffer->getNextOutput() == agentBuffer->getBuffer())
                    ? agentBuffer->getBuffer() + RING_BUFFER_SAMPLES - BUFFER_LENGTH_SAMPLES
                    : agentBuffer->getNextOutput() - BUFFER_LENGTH_SAMPLES;
                agentBuffer->setAddedToMix(false);
            }

            for (int s = 0; s < BUFFER_LENGTH_SAMPLES; s++) {
                long longSample = (previousOutput != NULL)
                    ? masterMix[s] - previousOutput[s]
                    : masterMix[s];
            
                int16_t shortSample;
                
                if (longSample < 0) {
                    shortSample = std::max(longSample, MIN_SAMPLE_VALUE);
                } else {
                    shortSample = std::min(longSample, MAX_SAMPLE_VALUE);
                }
                
                clientMix[s] = shortSample;
            }
            
            agentList.getAgentSocket().send(agent->getPublicSocket(), clientMix, BUFFER_LENGTH_BYTES);
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

void *reportAliveToDS(void *args) {
    
    timeval lastSend;
    unsigned char output[7];
   
    while (true) {
        gettimeofday(&lastSend, NULL);
        
        *output = 'M';
        packSocket(output + 1, 895283510, htons(MIXER_LISTEN_PORT));
        agentList.getAgentSocket().send(DOMAIN_IP, DOMAINSERVER_PORT, output, 7);
        
        double usecToSleep = 1000000 - (usecTimestampNow() - usecTimestamp(&lastSend));
        
        if (usecToSleep > 0) {
            usleep(usecToSleep);
        } else {
            std::cout << "No sleep required!";
        }
    }    
}

void attachNewBufferToAgent(Agent *newAgent) {
    if (newAgent->getLinkedData() == NULL) {
        newAgent->setLinkedData(new AudioRingBuffer());
    }
}

int main(int argc, const char * argv[])
{
    ssize_t receivedBytes = 0;
    
    agentList.linkedDataCreateCallback = attachNewBufferToAgent;
    
    // setup the agentSocket to report to domain server
    pthread_t reportAliveThread;
    pthread_create(&reportAliveThread, NULL, reportAliveToDS, NULL);
    
    //  Lookup the IP address of things we have hostnames
    if (atoi(DOMAIN_IP) == 0) {
        struct hostent* pHostInfo;
        if ((pHostInfo = gethostbyname(DOMAIN_HOSTNAME)) != NULL) {
            sockaddr_in tempAddress;
            memcpy(&tempAddress.sin_addr, pHostInfo->h_addr_list[0], pHostInfo->h_length);
            strcpy(DOMAIN_IP, inet_ntoa(tempAddress.sin_addr));
            printf("Domain server %s: %s\n", DOMAIN_HOSTNAME, DOMAIN_IP);
            
        } else {
            printf("Failed lookup domainserver\n");
        }
    } else {
        printf("Using static domainserver IP: %s\n", DOMAIN_IP);
    }

    int16_t *packetData = new int16_t[BUFFER_LENGTH_SAMPLES];

    pthread_t sendBufferThread;
    pthread_create(&sendBufferThread, NULL, sendBuffer, NULL);
    
    sockaddr *agentAddress = new sockaddr;

    while (true) {
        if(agentList.getAgentSocket().receive(agentAddress, packetData, &receivedBytes)) {
            if (receivedBytes == BUFFER_LENGTH_BYTES) {
                // add or update the existing interface agent
                agentList.addOrUpdateAgent(agentAddress, agentAddress, 'I');
                agentList.updateAgentWithData(agentAddress, (void *)packetData, receivedBytes);
            }
        }
    }
    
    pthread_join(reportAliveThread, NULL);
    pthread_join(sendBufferThread, NULL);
    
    return 0;
}

