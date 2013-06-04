//
//  main.cpp
//  Audio Injector
//
//  Created by Leonardo Murillo on 3/5/13.
//  Copyright (c) 2013 Leonardo Murillo. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sstream>

#include <AgentList.h>
#include <AgentTypes.h>
#include <AvatarData.h>
#include <SharedUtil.h>
#include <PacketHeaders.h>
#include <UDPSocket.h>
#include <AudioInjector.h>
#include <AudioInjectionManager.h>

const int AVATAR_MIXER_DATA_SEND_INTERVAL_MSECS = 15;

const int DEFAULT_INJECTOR_VOLUME = 0xFF;

// Command line parameter defaults
bool loopAudio = true;
float sleepIntervalMin = 1.00;
float sleepIntervalMax = 2.00;
char *sourceAudioFile = NULL;
const char *allowedParameters = ":sb::t::c::a::f::d::r:";
float floatArguments[4] = {0.0f, 0.0f, 0.0f, 0.0f};
unsigned char volume = DEFAULT_INJECTOR_VOLUME;
float triggerDistance = 0.0f;
float radius = 0.0f;

void usage(void) {
    std::cout << "High Fidelity - Interface audio injector" << std::endl;
    std::cout << "   -s                             Random sleep mode. If not specified will default to constant loop." << std::endl;
    std::cout << "   -b FLOAT                       Min. number of seconds to sleep. Only valid in random sleep mode. Default 1.0" << std::endl;
    std::cout << "   -t FLOAT                       Max. number of seconds to sleep. Only valid in random sleep mode. Default 2.0" << std::endl;
    std::cout << "   -c FLOAT,FLOAT,FLOAT,FLOAT     X,Y,Z,YAW position in universe where audio will be originating from and direction. Defaults to 0,0,0,0" << std::endl;
    std::cout << "   -a 0-255                       Attenuation curve modifier, defaults to 255" << std::endl;
    std::cout << "   -f FILENAME                    Name of audio source file. Required - RAW format, 22050hz 16bit signed mono" << std::endl;
    std::cout << "   -d FLOAT                       Trigger distance for injection. If not specified will loop constantly" << std::endl;
    std::cout << "   -r FLOAT                       Radius for spherical source. If not specified injected audio is point source" << std::endl;
}

bool processParameters(int parameterCount, char* parameterData[]) {
    int p;
    while ((p = getopt(parameterCount, parameterData, allowedParameters)) != -1) {
        switch (p) {
            case 's':
                ::loopAudio = false;
                std::cout << "[DEBUG] Random sleep mode enabled" << std::endl;
                break;
            case 'b':
                ::sleepIntervalMin = atof(optarg);
                std::cout << "[DEBUG] Min delay between plays " << sleepIntervalMin << "sec" << std::endl;
                break;
            case 't':
                ::sleepIntervalMax = atof(optarg);
                std::cout << "[DEBUG] Max delay between plays " << sleepIntervalMax << "sec" << std::endl;
                break;
            case 'f':
                ::sourceAudioFile = optarg;
                std::cout << "[DEBUG] Opening file: " << sourceAudioFile << std::endl;
                break;
            case 'c':
            {
                std::istringstream ss(optarg);
                std::string token;
                
                int i = 0;
                while (std::getline(ss, token, ',')) {
                    ::floatArguments[i] = atof(token.c_str());
                    ++i;
                    if (i == 4) {
                        break;
                    }
                }
                 
                break;
            }
            case 'a':
                ::volume = atoi(optarg);
                std::cout << "[DEBUG] Attenuation modifier: " << optarg << std::endl;
                break;
            case 'd':
                ::triggerDistance = atof(optarg);
                std::cout << "[DEBUG] Trigger distance: " << optarg << std::endl;
                break;
            case 'r':
                ::radius = atof(optarg);
                std::cout << "[DEBUG] Injector radius: " << optarg << std::endl;
                break;
            default:
                usage();
                return false;
        }
    }
    return true;
};

bool stopReceiveAgentDataThread;

void *receiveAgentData(void *args) {
    sockaddr senderAddress;
    ssize_t bytesReceived;
    unsigned char incomingPacket[MAX_PACKET_SIZE];
    
    AgentList* agentList = AgentList::getInstance();
    
    while (!::stopReceiveAgentDataThread) {
        if (agentList->getAgentSocket()->receive(&senderAddress, incomingPacket, &bytesReceived)) {
            switch (incomingPacket[0]) {
                case PACKET_HEADER_BULK_AVATAR_DATA:
                    // this is the positional data for other agents
                    // pass that off to the agentList processBulkAgentData method
                    agentList->processBulkAgentData(&senderAddress, incomingPacket, bytesReceived);
                    break;
                default:
                    // have the agentList handle list of agents from DS, replies from other agents, etc.
                    agentList->processAgentData(&senderAddress, incomingPacket, bytesReceived);
                    break;
            }
        }
    }
    
    pthread_exit(0);
    return NULL;
}

void createAvatarDataForAgent(Agent* agent) {
    if (!agent->getLinkedData()) {
        agent->setLinkedData(new AvatarData(agent));
    }
}

int main(int argc, char* argv[]) {
    // new seed for random audio sleep times
    srand(time(0));
    
    int AUDIO_UDP_SEND_PORT = 1500 + (rand() % (int)(1500 - 2000 + 1));
    
    if (processParameters(argc, argv)) {
        if (::sourceAudioFile == NULL) {
            std::cout << "[FATAL] Source audio file not specified" << std::endl;
            exit(-1);
        } else {
            AudioInjector injector(sourceAudioFile);
            
            // create an AgentList instance to handle communication with other agents
            AgentList* agentList = AgentList::createInstance(AGENT_TYPE_AUDIO_INJECTOR, AUDIO_UDP_SEND_PORT);
            
            pthread_t receiveAgentDataThread;
            pthread_create(&receiveAgentDataThread, NULL, receiveAgentData, NULL);
            
            // start telling the domain server that we are alive
            agentList->startDomainServerCheckInThread();
            
            // start the agent list thread that will kill off agents when they stop talking
            agentList->startSilentAgentRemovalThread();
            
            injector.setPosition(glm::vec3(::floatArguments[0], ::floatArguments[1], ::floatArguments[2]));
            injector.setBearing(*(::floatArguments + 3));
            injector.setVolume(::volume);
            
            if (::radius > 0) {
                // if we were passed a cube side length, give that to the injector
                injector.setRadius(::radius);
            }

            // register the callback for agent data creation
            agentList->linkedDataCreateCallback = createAvatarDataForAgent;
            
            unsigned char broadcastPacket = PACKET_HEADER_INJECT_AUDIO;
            
            timeval thisSend;
            double numMicrosecondsSleep = 0;
            
            while (true) {
                if (::triggerDistance) {
                    
                    // update the thisSend timeval to the current time
                    gettimeofday(&thisSend, NULL);
                    
                    // find the current avatar mixer
                    Agent* avatarMixer = agentList->soloAgentOfType(AGENT_TYPE_AVATAR_MIXER);
                    
                    // make sure we actually have an avatar mixer with an active socket
                    if (avatarMixer && avatarMixer->getActiveSocket() != NULL) {
                        // use the UDPSocket instance attached to our agent list to ask avatar mixer for a list of avatars
                        agentList->getAgentSocket()->send(avatarMixer->getActiveSocket(),
                                                          &broadcastPacket,
                                                          sizeof(broadcastPacket));
                    }
                    
                    if (!injector.isInjectingAudio()) {
                        // enumerate the other agents to decide if one is close enough that we should inject
                        for (AgentList::iterator agent = agentList->begin(); agent != agentList->end(); agent++) {
                            AvatarData* avatarData = (AvatarData*) agent->getLinkedData();
                            
                            if (avatarData) {
                                glm::vec3 tempVector = injector.getPosition() - avatarData->getPosition();
                                float squareDistance = glm::dot(tempVector, tempVector);
                                
                                if (squareDistance <= ::triggerDistance) {
                                    // look for an audio mixer in our agent list
                                    Agent* audioMixer = AgentList::getInstance()->soloAgentOfType(AGENT_TYPE_AUDIO_MIXER);
                                    
                                    if (audioMixer) {                                        
                                        // we have an active audio mixer we can send data to
                                        AudioInjectionManager::threadInjector(&injector);
                                    }
                                }
                            }
                        }
                    }
                    
                    // sleep for the correct amount of time to have data send be consistently timed
                    if ((numMicrosecondsSleep = (AVATAR_MIXER_DATA_SEND_INTERVAL_MSECS * 1000) -
                         (usecTimestampNow() - usecTimestamp(&thisSend))) > 0) {
                        usleep(numMicrosecondsSleep);
                    }
                } else {
                    // look for an audio mixer in our agent list
                    Agent* audioMixer = AgentList::getInstance()->soloAgentOfType(AGENT_TYPE_AUDIO_MIXER);
                    
                    if (audioMixer) {
                        injector.injectAudio(agentList->getAgentSocket(), audioMixer->getActiveSocket());
                    }
                    
                    float delay = 0;
                    int usecDelay = 0;
                    
                    if (!::loopAudio) {
                        delay = randFloatInRange(::sleepIntervalMin, ::sleepIntervalMax);
                        usecDelay = delay * 1000 * 1000;
                        usleep(usecDelay);
                    }                    
                }
            }
            
            // stop the receive agent data thread
            stopReceiveAgentDataThread = true;
            pthread_join(receiveAgentDataThread, NULL);
            
            // stop the agent list's threads
            agentList->stopDomainServerCheckInThread();
            agentList->stopSilentAgentRemovalThread();
        }
    }
}

