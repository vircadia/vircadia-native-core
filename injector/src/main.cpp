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

enum {
    INJECTOR_POSITION_X,
    INJECTOR_POSITION_Y,
    INJECTOR_POSITION_Z,
    INJECTOR_YAW
};

// Command line parameter defaults
bool shouldLoopAudio = true;
bool hasInjectedAudioOnce = false;
float sleepIntervalMin = 1.00;
float sleepIntervalMax = 2.00;
char *sourceAudioFile = NULL;
const char *allowedParameters = ":sc::a::f::t::r:";
float floatArguments[4] = {0.0f, 0.0f, 0.0f, 0.0f};
unsigned char volume = DEFAULT_INJECTOR_VOLUME;
float triggerDistance = 0.0f;
float radius = 0.0f;

void usage(void) {
    std::cout << "High Fidelity - Interface audio injector" << std::endl;
    std::cout << "   -s                             Single play mode. If not specified will default to constant loop." << std::endl;
    std::cout << "   -c FLOAT,FLOAT,FLOAT,FLOAT     X,Y,Z,YAW position in universe where audio will be originating from and direction. Defaults to 0,0,0,0" << std::endl;
    std::cout << "   -a 0-255                       Attenuation curve modifier, defaults to 255" << std::endl;
    std::cout << "   -f FILENAME                    Name of audio source file. Required - RAW format, 22050hz 16bit signed mono" << std::endl;
    std::cout << "   -t FLOAT                       Trigger distance for injection. If not specified will loop constantly" << std::endl;
    std::cout << "   -r FLOAT                       Radius for spherical source. If not specified injected audio is point source" << std::endl;
}

bool processParameters(int parameterCount, char* parameterData[]) {
    int p;
    while ((p = getopt(parameterCount, parameterData, allowedParameters)) != -1) {
        switch (p) {
            case 's':
                ::shouldLoopAudio = false;
                std::cout << "[DEBUG] Single play mode enabled" << std::endl;
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
            case 't':
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
            
            // start the agent list thread that will kill off agents when they stop talking
            agentList->startSilentAgentRemovalThread();
            
            injector.setPosition(glm::vec3(::floatArguments[INJECTOR_POSITION_X],
                                           ::floatArguments[INJECTOR_POSITION_Y],
                                           ::floatArguments[INJECTOR_POSITION_Z]));
            injector.setOrientation(glm::quat(glm::vec3(0.0f, ::floatArguments[INJECTOR_YAW], 0.0f)));
            injector.setVolume(::volume);
            
            if (::radius > 0) {
                // if we were passed a cube side length, give that to the injector
                injector.setRadius(::radius);
            }

            // register the callback for agent data creation
            agentList->linkedDataCreateCallback = createAvatarDataForAgent;
    
            timeval lastSend = {};
            unsigned char broadcastPacket = PACKET_HEADER_INJECT_AUDIO;            
            
            timeval lastDomainServerCheckIn = {};
            
            sockaddr senderAddress;
            ssize_t bytesReceived;
            unsigned char incomingPacket[MAX_PACKET_SIZE];
            
            // the audio injector needs to know about the avatar mixer and the audio mixer
            const char INJECTOR_AGENTS_OF_INTEREST[] = {AGENT_TYPE_AUDIO_MIXER, AGENT_TYPE_AVATAR_MIXER};
           
            int bytesAgentsOfInterest = (::triggerDistance > 0)
                ? sizeof(INJECTOR_AGENTS_OF_INTEREST)
                : sizeof(INJECTOR_AGENTS_OF_INTEREST) - 1;
            
            AgentList::getInstance()->setAgentTypesOfInterest(INJECTOR_AGENTS_OF_INTEREST, bytesAgentsOfInterest);
            
            while (true) {                
                // send a check in packet to the domain server if DOMAIN_SERVER_CHECK_IN_USECS has elapsed
                if (usecTimestampNow() - usecTimestamp(&lastDomainServerCheckIn) >= DOMAIN_SERVER_CHECK_IN_USECS) {
                    gettimeofday(&lastDomainServerCheckIn, NULL);
                    AgentList::getInstance()->sendDomainServerCheckIn();
                }
                
                while (agentList->getAgentSocket()->receive(&senderAddress, incomingPacket, &bytesReceived)) {
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
                
                if (::triggerDistance) {
                    if (!injector.isInjectingAudio()) {
                        // enumerate the other agents to decide if one is close enough that we should inject
                        for (AgentList::iterator agent = agentList->begin(); agent != agentList->end(); agent++) {
                            AvatarData* avatarData = (AvatarData*) agent->getLinkedData();
                            
                            if (avatarData) {
                                glm::vec3 tempVector = injector.getPosition() - avatarData->getPosition();
                                
                                if (glm::dot(tempVector, tempVector) <= ::triggerDistance) {
                                    // use the AudioInjectionManager to thread this injector
                                    AudioInjectionManager::threadInjector(&injector);
                                }
                            }
                        }
                    }
                    
                    // find the current avatar mixer
                    Agent* avatarMixer = agentList->soloAgentOfType(AGENT_TYPE_AVATAR_MIXER);
                    
                    // make sure we actually have an avatar mixer with an active socket
                    if (avatarMixer && avatarMixer->getActiveSocket() != NULL
                        && (usecTimestampNow() - usecTimestamp(&lastSend) > AVATAR_MIXER_DATA_SEND_INTERVAL_MSECS)) {
                        
                        // update the lastSend timeval to the current time
                        gettimeofday(&lastSend, NULL);
                        
                        // use the UDPSocket instance attached to our agent list to ask avatar mixer for a list of avatars
                        agentList->getAgentSocket()->send(avatarMixer->getActiveSocket(),
                                                          &broadcastPacket,
                                                          sizeof(broadcastPacket));
                    }
                } else {
                    if (!injector.isInjectingAudio() && (::shouldLoopAudio || !::hasInjectedAudioOnce)) {
                        // use the AudioInjectionManager to thread this injector
                        AudioInjectionManager::threadInjector(&injector);
                        ::hasInjectedAudioOnce = true;
                    }
                }
            }
            
            // stop the agent list's threads
            agentList->stopSilentAgentRemovalThread();
        }
    }
}

