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

char DOMAIN_HOSTNAME[] = "highfidelity.below92.com";
char DOMAIN_IP[100] = "";    //  IP Address will be re-set by lookup on startup
const int DOMAINSERVER_PORT = 40102;

const int MAX_VOXEL_TREE_DEPTH_LEVELS = 6;

AgentList agentList(VOXEL_LISTEN_PORT);
in_addr_t localAddress;

void *reportAliveToDS(void *args) {

    timeval lastSend;
    unsigned char output[7];

    while (true) {
        gettimeofday(&lastSend, NULL);

        *output = 'V';
        packSocket(output + 1, 895283510, htons(VOXEL_LISTEN_PORT));
//        packSocket(output + 1, 788637888, htons(VOXEL_LISTEN_PORT));
        agentList.getAgentSocket().send(DOMAIN_IP, DOMAINSERVER_PORT, output, 7);

        double usecToSleep = 1000000 - (usecTimestampNow() - usecTimestamp(&lastSend));

        if (usecToSleep > 0) {
        #ifdef _WIN32
            Sleep( static_cast<int>(1000.0f*usecToSleep) );
        #else
            usleep(usecToSleep);
        #endif
        } else {
            std::cout << "No sleep required!";
        }
    }
}

int randomlyFillVoxelTree(int levelsToGo, VoxelNode *currentRootNode) {
    // randomly generate children for this node
    // the first level of the tree (where levelsToGo = MAX_VOXEL_TREE_DEPTH_LEVELS) has all 8
    if (levelsToGo > 0) {

        int grandChildrenFromNode = 0;
        bool createdChildren = false;
        int colorArray[4] = {};

        createdChildren = false;

        for (int i = 0; i < 8; i++) {
            if (((i == 0 || i == 1 | i == 4 | i == 5) && (randomBoolean() || levelsToGo != 1)) ) {
                // create a new VoxelNode to put here
                currentRootNode->children[i] = new VoxelNode();

                // give this child it's octal code
                currentRootNode->children[i]->octalCode = childOctalCode(currentRootNode->octalCode, i);

                // fill out the lower levels of the tree using that node as the root node
                grandChildrenFromNode = randomlyFillVoxelTree(levelsToGo - 1, currentRootNode->children[i]);

                if (currentRootNode->children[i]->color[3] == 1) {
                    for (int c = 0; c < 3; c++) {
                        colorArray[c] += currentRootNode->children[i]->color[c];
                    }

                    colorArray[3]++;
                }

                if (grandChildrenFromNode > 0) {
                    currentRootNode->childMask += (1 << (7 - i));
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

        return createdChildren;
    } else {
        // this is a leaf node, just give it a color
        currentRootNode->setRandomColor(MIN_BRIGHTNESS);

        return 0;
    }
}

void attachVoxelAgentDataToAgent(Agent *newAgent) {
    if (newAgent->getLinkedData() == NULL) {
        newAgent->setLinkedData(new VoxelAgentData());
    }
}


int main(int argc, const char * argv[])
{
    setvbuf(stdout, NULL, _IOLBF, 0);

#ifndef _WIN32
    // get the local address of the voxel server
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa ->ifa_addr->sa_family==AF_INET) { // check it is IP4
            // is a valid IP4 Address
            localAddress = ((struct sockaddr_in *)ifa->ifa_addr)->sin_addr.s_addr;
        }
    }
#endif

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

    // setup the agentSocket to report to domain server
    pthread_t reportAliveThread;
    pthread_create(&reportAliveThread, NULL, reportAliveToDS, NULL);

    agentList.linkedDataCreateCallback = &attachVoxelAgentDataToAgent;
    agentList.startSilentAgentRemovalThread();
    
    srand((unsigned)time(0));

    // use our method to create a random voxel tree
    VoxelTree randomTree;

    // create an octal code buffer and load it with 0 so that the recursive tree fill can give
    // octal codes to the tree nodes that it is creating
    randomlyFillVoxelTree(MAX_VOXEL_TREE_DEPTH_LEVELS, randomTree.rootNode);

    unsigned char *voxelPacket = new unsigned char[MAX_VOXEL_PACKET_SIZE];
    unsigned char *voxelPacketEnd;

    unsigned char *stopOctal;
    int packetCount;
    int totalBytesSent;

    sockaddr agentPublicAddress;
    
    char *packetData = new char[MAX_PACKET_SIZE];
    ssize_t receivedBytes;

    // loop to send to agents requesting data
    while (true) {
        if (agentList.getAgentSocket().receive(&agentPublicAddress, packetData, &receivedBytes)) {
            if (packetData[0] == 'H') {
                agentList.addOrUpdateAgent(&agentPublicAddress, &agentPublicAddress, packetData[0]);
                agentList.updateAgentWithData(&agentPublicAddress, (void *)packetData, receivedBytes);
                
                VoxelAgentData *agentData = (VoxelAgentData *) agentList.getAgents()[agentList.indexOfMatchingAgent(&agentPublicAddress)].getLinkedData();
                int newLevel = 6;
                if (newLevel > agentData->lastSentLevel) {
                    // the agent has already received a deeper level than this from us
                    // do nothing
                    
                    stopOctal = randomTree.rootNode->octalCode;
                    packetCount = 0;
                    totalBytesSent = 0;
                    
                    while (stopOctal != NULL) {
                        voxelPacketEnd = voxelPacket;
                        stopOctal = randomTree.loadBitstreamBuffer(voxelPacketEnd,
                                                                   stopOctal,
                                                                   randomTree.rootNode,
                                                                   newLevel);
                        
                        agentList.getAgentSocket().send((sockaddr *)&agentPublicAddress,
                                                        voxelPacket,
                                                        voxelPacketEnd - voxelPacket);
                        
                        packetCount++;
                        totalBytesSent += voxelPacketEnd - voxelPacket;
                    }
                    
                    agentData->lastSentLevel = newLevel;
                }
            }
        }
    }

    pthread_join(reportAliveThread, NULL);
    agentList.stopSilentAgentRemovalThread();

    return 0;
}