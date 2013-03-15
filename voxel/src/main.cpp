//
//  main.cpp
//  Voxel Server
//
//  Created by Stephen Birara on 03/06/13.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include <sys/time.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <SharedUtil.h>
#include <OctalCode.h>
#include <AgentList.h>
#include <VoxelTree.h>

const int VOXEL_LISTEN_PORT = 40106;

const int NUMBER_OF_VOXELS = 250000;

const float MAX_UNIT_ANY_AXIS = 20.0f;

const int VERTICES_PER_VOXEL = 8;
const int VERTEX_POINTS_PER_VOXEL = 3 * VERTICES_PER_VOXEL;
const int COLOR_VALUES_PER_VOXEL = 3 * VERTICES_PER_VOXEL;

const int VOXEL_SIZE_BYTES = 3 + (3 * sizeof(float));
const int VOXELS_PER_PACKET = (MAX_PACKET_SIZE - 1) / VOXEL_SIZE_BYTES;

const int MIN_BRIGHTNESS = 64;
const float DEATH_STAR_RADIUS = 4.0;
const float MAX_CUBE = 0.05;

char DOMAIN_HOSTNAME[] = "highfidelity.below92.com";
char DOMAIN_IP[100] = "";    //  IP Address will be re-set by lookup on startup
const int DOMAINSERVER_PORT = 40102;

const int MAX_VOXEL_TREE_DEPTH_LEVELS = 1;
const int MAX_VOXEL_BITSTREAM_BYTES = 100000;

AgentList agentList(VOXEL_LISTEN_PORT);
in_addr_t localAddress;
char voxelBitStream[MAX_VOXEL_BITSTREAM_BYTES];

unsigned char randomColorValue() {
    return MIN_BRIGHTNESS + (rand() % (255 - MIN_BRIGHTNESS));
}

bool randomBoolean() {
    return rand() % 2;
}

void *reportAliveToDS(void *args) {
    
    timeval lastSend;
    unsigned char output[7];
    
    while (true) {
        gettimeofday(&lastSend, NULL);
        
        *output = 'V';
//        packSocket(output + 1, 895283510, htons(VOXEL_LISTEN_PORT));
        packSocket(output + 1, 788637888, htons(VOXEL_LISTEN_PORT));
        agentList.getAgentSocket().send(DOMAIN_IP, DOMAINSERVER_PORT, output, 7);
        
        double usecToSleep = 1000000 - (usecTimestampNow() - usecTimestamp(&lastSend));
        
        if (usecToSleep > 0) {
            usleep(usecToSleep);
        } else {
            std::cout << "No sleep required!";
        }
    }
}

void randomlyFillVoxelTree(int levelsToGo, VoxelNode *currentRootNode) {
    // randomly generate children for this node
    // the first level of the tree (where levelsToGo = MAX_VOXEL_TREE_DEPTH_LEVELS) has all 8
    if (levelsToGo > 0) {
        
        int coloredChildren = 0;
        bool createdChildren = false;
        unsigned char sumColor[3] = {};
        
        createdChildren = false;

        for (int i = 0; i < 8; i++) {
            if (randomBoolean() || levelsToGo == MAX_VOXEL_TREE_DEPTH_LEVELS) {
                // create a new VoxelNode to put here
                currentRootNode->children[i] = new VoxelNode();
                
                // give this child it's octal code
                currentRootNode->children[i]->octalCode = childOctalCode(currentRootNode->octalCode, i);
                
                printf("Child node in position %d at level %d\n", i, MAX_VOXEL_TREE_DEPTH_LEVELS - levelsToGo);
                
                // fill out the lower levels of the tree using that node as the root node
                randomlyFillVoxelTree(levelsToGo - 1, currentRootNode->children[i]);
                
                if (currentRootNode->children[i]) {
                    for (int c = 0; c < 3; c++) {
                        sumColor[c] += currentRootNode->children[i]->color[c];
                    }
                    
                    coloredChildren++;
                }
                
                currentRootNode->childMask += (1 << (7 - i));
                createdChildren = true;
            }
        }
        
        // figure out the color value for this node
        
        if (coloredChildren > 4 || !createdChildren) {
            // we need at least 4 colored children to have an average color value
            // or if we have none we generate random values
            
            for (int c = 0; c < 3; c++) {
                if (coloredChildren > 4) {
                    // set the average color value
                    currentRootNode->color[c] = sumColor[c] / coloredChildren;
                } else {
                    // we have no children, we're a leaf
                    // generate a random color value
                    currentRootNode->color[c] = randomColorValue();
                }
            }
            
            // set the alpha to 1 to indicate that this isn't transparent
            currentRootNode->color[3] = 1;
        } else {
            // some children, but not enough
            // set this node's alpha to 0
            currentRootNode->color[3] = 0;
        }
    } else {
        // this is a leaf node, just give it a color
        currentRootNode->color[0] = randomColorValue();
        currentRootNode->color[1] = randomColorValue();
        currentRootNode->color[2] = randomColorValue();
        currentRootNode->color[3] = 1;
    }
}

int main(int argc, const char * argv[])
{
    setvbuf(stdout, NULL, _IOLBF, 0);
    
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
    
    srand((unsigned)time(0));
    
    // use our method to create a random voxel tree
    VoxelTree randomTree;
    
    // create an octal code buffer and load it with 0 so that the recursive tree fill can give
    // octal codes to the tree nodes that it is creating
    randomlyFillVoxelTree(MAX_VOXEL_TREE_DEPTH_LEVELS, randomTree.rootNode);
    
    char *packetSplitsPtrs[MAX_VOXEL_BITSTREAM_BYTES / MAX_PACKET_SIZE] = {};
    
    // returned unsigned char * is end of last write to bitstream
    char *lastPacketSplit = randomTree.loadBitstream(voxelBitStream, randomTree.rootNode, packetSplitsPtrs);
    
    // set the last packet split pointer to lastPacketSplit
    for (int i = 1; i < MAX_VOXEL_BITSTREAM_BYTES / MAX_PACKET_SIZE; i++) {
        if (packetSplitsPtrs[i] == NULL) {
            if (lastPacketSplit != NULL) {
                // lastPacketSplit has not been read yet
                // set this packetSplitPtr to that and set it NULL so we know it's been read
                packetSplitsPtrs[i] = lastPacketSplit;
                lastPacketSplit = NULL;
            } else {
                // no more split pointers to read, break out
                break;
            }
        }
    }
    
    sockaddr_in agentPublicAddress;
    
    char *packetData = new char[MAX_PACKET_SIZE];
    ssize_t receivedBytes;
    
    int sentVoxels = 0;
    
    // loop to send to agents requesting data
    while (true) {
        if (agentList.getAgentSocket().receive((sockaddr *)&agentPublicAddress, packetData, &receivedBytes)) {
            if (packetData[0] == 'I') {
                printf("Sending voxels to agent at address %s\n", inet_ntoa(agentPublicAddress.sin_addr));
                
                // send the bitstream to the client
                for (int i = 1; i < MAX_VOXEL_BITSTREAM_BYTES / MAX_PACKET_SIZE; i++) {
                    if (packetSplitsPtrs[i] == NULL) {
                        // no more split pointers to read, break out
                        break;
                    }
                    
                    // figure out the number of bytes in this packet
                    int thisPacketBytes = packetSplitsPtrs[i] - packetSplitsPtrs[i - 1];
                    
                    agentList.getAgentSocket().send((sockaddr *)&agentPublicAddress,
                                                    packetSplitsPtrs[i - 1],
                                                    thisPacketBytes);

                    
                    // output the bits in each byte of this packet
                    for (int j = 0; j < thisPacketBytes; j++) {
                        outputBits(*(packetSplitsPtrs[i - 1] + j));
                    }
                }
            }
        }
    }
    
    pthread_join(reportAliveThread, NULL);
    
    return 0;
}