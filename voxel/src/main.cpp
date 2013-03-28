//
//  main.cpp
//  Voxel Server
//
//  Created by Stephen Birara on 03/06/13.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//


#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <OctalCode.h>
#include <AgentList.h>
#include <VoxelTree.h>
#include "VoxelAgentData.h"
#include <SharedUtil.h>

#ifdef _WIN32
#include "Syssocket.h"
#include "Systime.h"
#else
#include <sys/time.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#endif

const int VOXEL_LISTEN_PORT = 40106;

const int VERTICES_PER_VOXEL = 8;
const int VERTEX_POINTS_PER_VOXEL = 3 * VERTICES_PER_VOXEL;
const int COLOR_VALUES_PER_VOXEL = 3 * VERTICES_PER_VOXEL;

const int VOXEL_SIZE_BYTES = 3 + (3 * sizeof(float));
const int VOXELS_PER_PACKET = (MAX_PACKET_SIZE - 1) / VOXEL_SIZE_BYTES;

const int MIN_BRIGHTNESS = 64;
const float DEATH_STAR_RADIUS = 4.0;
const float MAX_CUBE = 0.05f;

const int VOXEL_SEND_INTERVAL_USECS = 30 * 1000;
const int PACKETS_PER_CLIENT_PER_INTERVAL = 3;

const int MAX_VOXEL_TREE_DEPTH_LEVELS = 4;

AgentList agentList('V', VOXEL_LISTEN_PORT);
VoxelTree randomTree;

void randomlyFillVoxelTree(int levelsToGo, VoxelNode *currentRootNode) {
    // randomly generate children for this node
    // the first level of the tree (where levelsToGo = MAX_VOXEL_TREE_DEPTH_LEVELS) has all 8
    if (levelsToGo > 0) {

        bool createdChildren = false;
        int colorArray[4] = {};
        
        createdChildren = false;
        
        for (int i = 0; i < 8; i++) {
            if (true) {
                // create a new VoxelNode to put here
                currentRootNode->children[i] = new VoxelNode();
                
                // give this child it's octal code
                currentRootNode->children[i]->octalCode = childOctalCode(currentRootNode->octalCode, i);
                
                randomlyFillVoxelTree(levelsToGo - 1, currentRootNode->children[i]);

                if (currentRootNode->children[i]->color[3] == 1) {
                    for (int c = 0; c < 3; c++) {
                        colorArray[c] += currentRootNode->children[i]->color[c];
                    }
                    
                    colorArray[3]++;
                }
                
                createdChildren = true;
            }
        }
        
        if (!createdChildren) {
            // we didn't create any children for this node, making it a leaf
            // give it a random color
            currentRootNode->setRandomColor(MIN_BRIGHTNESS);
        } else {
            // set the color value for this node
            currentRootNode->setColorFromAverageOfChildren(colorArray);
        }
    } else {
        // this is a leaf node, just give it a color
        currentRootNode->setRandomColor(MIN_BRIGHTNESS);
    }
}

void *distributeVoxelsToListeners(void *args) {
    
    timeval lastSendTime;
    
    unsigned char *stopOctal;
    int packetCount;
    
    int totalBytesSent;
    
    unsigned char *voxelPacket = new unsigned char[MAX_VOXEL_PACKET_SIZE];
    unsigned char *voxelPacketEnd;
    
    float treeRoot[3] = {-40, 0, -40};
    
    while (true) {
        gettimeofday(&lastSendTime, NULL);
        
        // enumerate the agents to send 3 packets to each
        for (int i = 0; i < agentList.getAgents().size(); i++) {
            
            Agent *thisAgent = (Agent *)&agentList.getAgents()[i];
            VoxelAgentData *agentData = (VoxelAgentData *)(thisAgent->getLinkedData());
            
            // lock this agent's delete mutex so that the delete thread doesn't
            // kill the agent while we are working with it
            pthread_mutex_lock(&thisAgent->deleteMutex);
            
            stopOctal = NULL;
            packetCount = 0;
            totalBytesSent = 0;
            randomTree.leavesWrittenToBitstream = 0;
            
            for (int j = 0; j < PACKETS_PER_CLIENT_PER_INTERVAL; j++) {
                voxelPacketEnd = voxelPacket;
                stopOctal = randomTree.loadBitstreamBuffer(voxelPacketEnd,
                                                           randomTree.rootNode,
                                                           agentData->rootMarkerNode,
                                                           agentData->position,
                                                           treeRoot,
                                                           stopOctal);
                
                agentList.getAgentSocket().send(thisAgent->getActiveSocket(), voxelPacket, voxelPacketEnd - voxelPacket);
                
                packetCount++;
                totalBytesSent += voxelPacketEnd - voxelPacket;
                
                if (agentData->rootMarkerNode->childrenVisitedMask == 255) {
                    break;
                }
            }
            
            // for any agent that has a root marker node with 8 visited children
            // recursively delete its marker nodes so we can revisit
            if (agentData->rootMarkerNode->childrenVisitedMask == 255) {
                delete agentData->rootMarkerNode;
                agentData->rootMarkerNode = new MarkerNode();
            }
            
            // unlock the delete mutex so the other thread can
            // kill the agent if it has dissapeared
            pthread_mutex_unlock(&thisAgent->deleteMutex);
        }
        
        // dynamically sleep until we need to fire off the next set of voxels
        double usecToSleep =  VOXEL_SEND_INTERVAL_USECS - (usecTimestampNow() - usecTimestamp(&lastSendTime));
        
        if (usecToSleep > 0) {
            usleep(usecToSleep);
        } else {
            std::cout << "Last send took too much time, not sleeping!\n";
        }
    }
    
    pthread_exit(0);
}

void attachVoxelAgentDataToAgent(Agent *newAgent) {
    if (newAgent->getLinkedData() == NULL) {
        newAgent->setLinkedData(new VoxelAgentData());
    }
}


int main(int argc, const char * argv[])
{
    setvbuf(stdout, NULL, _IOLBF, 0);

    agentList.linkedDataCreateCallback = &attachVoxelAgentDataToAgent;
    agentList.startSilentAgentRemovalThread();
    agentList.startDomainServerCheckInThread();
    
    srand((unsigned)time(0));

    // create an octal code buffer and load it with 0 so that the recursive tree fill can give
    // octal codes to the tree nodes that it is creating
    randomlyFillVoxelTree(MAX_VOXEL_TREE_DEPTH_LEVELS, randomTree.rootNode);
    
    pthread_t sendVoxelThread;
    pthread_create(&sendVoxelThread, NULL, distributeVoxelsToListeners, NULL);

    sockaddr agentPublicAddress;
    
    char *packetData = new char[MAX_PACKET_SIZE];
    ssize_t receivedBytes;

    // loop to send to agents requesting data
    while (true) {
        if (agentList.getAgentSocket().receive(&agentPublicAddress, packetData, &receivedBytes)) {
            if (packetData[0] == 'H') {
                if (agentList.addOrUpdateAgent(&agentPublicAddress,
                                               &agentPublicAddress,
                                               packetData[0],
                                               agentList.getLastAgentId())) {
                    agentList.increaseAgentId();
                }
                
                agentList.updateAgentWithData(&agentPublicAddress, (void *)packetData, receivedBytes);
            }
        }
    }
    
    pthread_join(sendVoxelThread, NULL);

    return 0;
}