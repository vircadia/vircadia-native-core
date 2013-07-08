//
//  main.cpp
//  eve
//
//  Created by Stephen Birarda on 4/22/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <cstring>
#include <sys/time.h>
#include <cstring>

#include <SharedUtil.h>
#include <NodeTypes.h>
#include <PacketHeaders.h>
#include <NodeList.h>
#include <AvatarData.h>
#include <AudioInjectionManager.h>
#include <AudioInjector.h>

const int EVE_NODE_LISTEN_PORT = 55441;

const float RANDOM_POSITION_MAX_DIMENSION = 10.0f;

const float DATA_SEND_INTERVAL_MSECS = 15;
const float MIN_AUDIO_SEND_INTERVAL_SECS = 10;
const int MIN_ITERATIONS_BETWEEN_AUDIO_SENDS = (MIN_AUDIO_SEND_INTERVAL_SECS * 1000) / DATA_SEND_INTERVAL_MSECS;
const int MAX_AUDIO_SEND_INTERVAL_SECS = 15;
const float MAX_ITERATIONS_BETWEEN_AUDIO_SENDS = (MAX_AUDIO_SEND_INTERVAL_SECS * 1000) / DATA_SEND_INTERVAL_MSECS;

const int ITERATIONS_BEFORE_HAND_GRAB = 100;
const int HAND_GRAB_DURATION_ITERATIONS = 50;
const int HAND_TIMER_SLEEP_ITERATIONS = 50;

const float EVE_PELVIS_HEIGHT = 0.565925f;

const float AUDIO_INJECT_PROXIMITY = 0.4f;
const int EVE_VOLUME_BYTE = 190;

const char EVE_AUDIO_FILENAME[] = "/etc/highfidelity/eve/resources/eve.raw";

bool stopReceiveNodeDataThread;

void *receiveNodeData(void *args) {
    sockaddr senderAddress;
    ssize_t bytesReceived;
    unsigned char incomingPacket[MAX_PACKET_SIZE];
    
    NodeList* nodeList = NodeList::getInstance();
    
    while (!::stopReceiveNodeDataThread) {
        if (nodeList->getNodeSocket()->receive(&senderAddress, incomingPacket, &bytesReceived)) { 
            switch (incomingPacket[0]) {
                case PACKET_TYPE_BULK_AVATAR_DATA:
                    // this is the positional data for other nodes
                    // pass that off to the nodeList processBulkNodeData method
                    nodeList->processBulkNodeData(&senderAddress, incomingPacket, bytesReceived);
                    
                    break;
                default:
                    // have the nodeList handle list of nodes from DS, replies from other nodes, etc.
                    nodeList->processNodeData(&senderAddress, incomingPacket, bytesReceived);
                    break;
            }
        }
    }
    
    pthread_exit(0);
    return NULL;
}

void createAvatarDataForNode(Node* node) {
    if (!node->getLinkedData()) {
        node->setLinkedData(new AvatarData(node));
    }
}

int main(int argc, const char* argv[]) {
    // new seed for random audio sleep times
    srand(time(0));
    
    // create an NodeList instance to handle communication with other nodes
    NodeList* nodeList = NodeList::createInstance(NODE_TYPE_AGENT, EVE_NODE_LISTEN_PORT);
    
    // start the node list thread that will kill off nodes when they stop talking
    nodeList->startSilentNodeRemovalThread();
    
    pthread_t receiveNodeDataThread;
    pthread_create(&receiveNodeDataThread, NULL, receiveNodeData, NULL);
    
    // create an AvatarData object, "eve"
    AvatarData eve;
    
    // move eve away from the origin
    // pick a random point inside a 10x10 grid
    
    eve.setPosition(glm::vec3(randFloatInRange(0, RANDOM_POSITION_MAX_DIMENSION),
                              EVE_PELVIS_HEIGHT, // this should be the same as the avatar's pelvis standing height
                              randFloatInRange(0, RANDOM_POSITION_MAX_DIMENSION)));
    
    // face any instance of eve down the z-axis
    eve.setBodyYaw(0);
    
    // put her hand out so somebody can shake it
    eve.setHandPosition(glm::vec3(eve.getPosition()[0] - 0.2,
                                  0.5,
                                  eve.getPosition()[2] + 0.1));
    
    // prepare the audio injection manager by giving it a handle to our node socket
    AudioInjectionManager::setInjectorSocket(nodeList->getNodeSocket());
    
    // read eve's audio data
    AudioInjector eveAudioInjector(EVE_AUDIO_FILENAME);
    
    // lower Eve's volume by setting the attentuation modifier (this is a value out of 255)
    eveAudioInjector.setVolume(EVE_VOLUME_BYTE);
    
    // set the position of the audio injector
    eveAudioInjector.setPosition(eve.getPosition());
    
    // register the callback for node data creation
    nodeList->linkedDataCreateCallback = createAvatarDataForNode;
    
    unsigned char broadcastPacket[MAX_PACKET_SIZE];
    int numHeaderBytes = populateTypeAndVersion(broadcastPacket, PACKET_TYPE_HEAD_DATA);
    
    timeval thisSend;
    long long numMicrosecondsSleep = 0;
    
    int handStateTimer = 0;
    
    timeval lastDomainServerCheckIn = {};
    
    // eve wants to hear about an avatar mixer and an audio mixer from the domain server
    const char EVE_NODE_TYPES_OF_INTEREST[] = {NODE_TYPE_AVATAR_MIXER, NODE_TYPE_AUDIO_MIXER};
    NodeList::getInstance()->setNodeTypesOfInterest(EVE_NODE_TYPES_OF_INTEREST, sizeof(EVE_NODE_TYPES_OF_INTEREST));

    while (true) {
        // send a check in packet to the domain server if DOMAIN_SERVER_CHECK_IN_USECS has elapsed
        if (usecTimestampNow() - usecTimestamp(&lastDomainServerCheckIn) >= DOMAIN_SERVER_CHECK_IN_USECS) {
            gettimeofday(&lastDomainServerCheckIn, NULL);
            NodeList::getInstance()->sendDomainServerCheckIn();
        }
        
        // update the thisSend timeval to the current time
        gettimeofday(&thisSend, NULL);
        
        // find the current avatar mixer
        Node* avatarMixer = nodeList->soloNodeOfType(NODE_TYPE_AVATAR_MIXER);
        
        // make sure we actually have an avatar mixer with an active socket
        if (nodeList->getOwnerID() != UNKNOWN_NODE_ID && avatarMixer && avatarMixer->getActiveSocket() != NULL) {
            unsigned char* packetPosition = broadcastPacket + numHeaderBytes;
            packetPosition += packNodeId(packetPosition, nodeList->getOwnerID());
            
            // use the getBroadcastData method in the AvatarData class to populate the broadcastPacket buffer
            packetPosition += eve.getBroadcastData(packetPosition);
            
            // use the UDPSocket instance attached to our node list to send avatar data to mixer
            nodeList->getNodeSocket()->send(avatarMixer->getActiveSocket(), broadcastPacket, packetPosition - broadcastPacket);
        }
        
        if (!eveAudioInjector.isInjectingAudio()) {
            // enumerate the other nodes to decide if one is close enough that eve should talk
            for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
                AvatarData* avatarData = (AvatarData*) node->getLinkedData();
                
                if (avatarData) {
                    glm::vec3 tempVector = eve.getPosition() - avatarData->getPosition();
                    float squareDistance = glm::dot(tempVector, tempVector);
                    
                    if (squareDistance <= AUDIO_INJECT_PROXIMITY) {
                        // look for an audio mixer in our node list
                        Node* audioMixer = NodeList::getInstance()->soloNodeOfType(NODE_TYPE_AUDIO_MIXER);
                        
                        if (audioMixer) {
                            // update the destination socket for the AIM, in case the mixer has changed
                            AudioInjectionManager::setDestinationSocket(*audioMixer->getPublicSocket());
                            
                            // we have an active audio mixer we can send data to
                            AudioInjectionManager::threadInjector(&eveAudioInjector);
                        }
                    }
                }            
            }
        }
        
        // simulate the effect of pressing and un-pressing the mouse button/pad
        handStateTimer++;
        
        if (handStateTimer == ITERATIONS_BEFORE_HAND_GRAB) {
            eve.setHandState(1);
        } else if (handStateTimer == ITERATIONS_BEFORE_HAND_GRAB + HAND_GRAB_DURATION_ITERATIONS) {
            eve.setHandState(0);
        } else if (handStateTimer >= ITERATIONS_BEFORE_HAND_GRAB + HAND_GRAB_DURATION_ITERATIONS + HAND_TIMER_SLEEP_ITERATIONS) {
            handStateTimer = 0;
        }
        
        // sleep for the correct amount of time to have data send be consistently timed
        if ((numMicrosecondsSleep = (DATA_SEND_INTERVAL_MSECS * 1000) - (usecTimestampNow() - usecTimestamp(&thisSend))) > 0) {
            usleep(numMicrosecondsSleep);
        }   
    }
    
    // stop the receive node data thread
    stopReceiveNodeDataThread = true;
    pthread_join(receiveNodeDataThread, NULL);
    
    // stop the node list's threads
    nodeList->stopSilentNodeRemovalThread();
}
