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

#include <NodeList.h>
#include <NodeTypes.h>
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

void createAvatarDataForNode(Node* node) {
    if (!node->getLinkedData()) {
        node->setLinkedData(new AvatarData(node));
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
            
            // create an NodeList instance to handle communication with other nodes
            NodeList* nodeList = NodeList::createInstance(NODE_TYPE_AUDIO_INJECTOR, AUDIO_UDP_SEND_PORT);
            
            // start the node list thread that will kill off nodes when they stop talking
            nodeList->startSilentNodeRemovalThread();
            
            injector.setPosition(glm::vec3(::floatArguments[INJECTOR_POSITION_X],
                                           ::floatArguments[INJECTOR_POSITION_Y],
                                           ::floatArguments[INJECTOR_POSITION_Z]));
            injector.setOrientation(glm::quat(glm::vec3(0.0f, ::floatArguments[INJECTOR_YAW], 0.0f)));
            injector.setVolume(::volume);
            
            if (::radius > 0) {
                // if we were passed a cube side length, give that to the injector
                injector.setRadius(::radius);
            }

            // register the callback for node data creation
            nodeList->linkedDataCreateCallback = createAvatarDataForNode;
    
            timeval lastSend = {};
            int numBytesPacketHeader = numBytesForPacketHeader((unsigned char*) &PACKET_TYPE_INJECT_AUDIO);
            unsigned char* broadcastPacket = new unsigned char[numBytesPacketHeader];
            
            timeval lastDomainServerCheckIn = {};
            
            sockaddr senderAddress;
            ssize_t bytesReceived;
            unsigned char incomingPacket[MAX_PACKET_SIZE];
            
            // the audio injector needs to know about the avatar mixer and the audio mixer
            const char INJECTOR_NODES_OF_INTEREST[] = {NODE_TYPE_AUDIO_MIXER, NODE_TYPE_AVATAR_MIXER};
           
            int bytesNodesOfInterest = (::triggerDistance > 0)
                ? sizeof(INJECTOR_NODES_OF_INTEREST)
                : sizeof(INJECTOR_NODES_OF_INTEREST) - 1;
            
            NodeList::getInstance()->setNodeTypesOfInterest(INJECTOR_NODES_OF_INTEREST, bytesNodesOfInterest);
            
            while (true) {                
                // send a check in packet to the domain server if DOMAIN_SERVER_CHECK_IN_USECS has elapsed
                if (usecTimestampNow() - usecTimestamp(&lastDomainServerCheckIn) >= DOMAIN_SERVER_CHECK_IN_USECS) {
                    gettimeofday(&lastDomainServerCheckIn, NULL);
                    NodeList::getInstance()->sendDomainServerCheckIn();
                }
                
                while (nodeList->getNodeSocket()->receive(&senderAddress, incomingPacket, &bytesReceived) &&
                       packetVersionMatch(incomingPacket)) {
                    switch (incomingPacket[0]) {
                        case PACKET_TYPE_BULK_AVATAR_DATA:                  // this is the positional data for other nodes
                            // pass that off to the nodeList processBulkNodeData method
                            nodeList->processBulkNodeData(&senderAddress, incomingPacket, bytesReceived);
                            break;
                        default:
                            // have the nodeList handle list of nodes from DS, replies from other nodes, etc.
                            nodeList->processNodeData(&senderAddress, incomingPacket, bytesReceived);
                            break;
                    }
                }
                
                if (::triggerDistance) {
                    if (!injector.isInjectingAudio()) {
                        // enumerate the other nodes to decide if one is close enough that we should inject
                        for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
                            AvatarData* avatarData = (AvatarData*) node->getLinkedData();
                            
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
                    Node* avatarMixer = nodeList->soloNodeOfType(NODE_TYPE_AVATAR_MIXER);
                    
                    // make sure we actually have an avatar mixer with an active socket
                    if (avatarMixer && avatarMixer->getActiveSocket() != NULL
                        && (usecTimestampNow() - usecTimestamp(&lastSend) > AVATAR_MIXER_DATA_SEND_INTERVAL_MSECS)) {
                        
                        // update the lastSend timeval to the current time
                        gettimeofday(&lastSend, NULL);
                        
                        // use the UDPSocket instance attached to our node list to ask avatar mixer for a list of avatars
                        nodeList->getNodeSocket()->send(avatarMixer->getActiveSocket(),
                                                        broadcastPacket,
                                                        numBytesPacketHeader);
                    }
                } else {
                    if (!injector.isInjectingAudio() && (::shouldLoopAudio || !::hasInjectedAudioOnce)) {
                        // use the AudioInjectionManager to thread this injector
                        AudioInjectionManager::threadInjector(&injector);
                        ::hasInjectedAudioOnce = true;
                    }
                }
            }
            
            // stop the node list's threads
            nodeList->stopSilentNodeRemovalThread();
        }
    }
}

