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

const int MAX_VOXEL_TREE_DEPTH_LEVELS = 10;

AgentList agentList('V', VOXEL_LISTEN_PORT);

void addRandomSphere(VoxelTree * tree) {
	float r = randFloatInRange(0.05,0.1);
	float xc = randFloatInRange(r,(1-r));
	float yc = randFloatInRange(r,(1-r));
	float zc = randFloatInRange(r,(1-r));
	float s = 0.001; // size of voxels to make up surface of sphere
	bool solid = true;

	printf("random sphere\n");
	printf("radius=%f\n",r);
	printf("xc=%f\n",xc);
	printf("yc=%f\n",yc);
	printf("zc=%f\n",zc);

	tree->createSphere(r,xc,yc,zc,s,solid);
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

    // Handle Local Domain testing with the --local command line
    bool wantLocalDomain = cmdOptionExists(argc, argv, "--local");
    if (wantLocalDomain) {
    	printf("Local Domain MODE!\n");
		int ip = getLocalAddress();
		sprintf(DOMAIN_IP,"%d.%d.%d.%d", (ip & 0xFF), ((ip >> 8) & 0xFF),((ip >> 16) & 0xFF), ((ip >> 24) & 0xFF));
    }

    agentList.linkedDataCreateCallback = &attachVoxelAgentDataToAgent;
    agentList.startSilentAgentRemovalThread();
    agentList.startDomainServerCheckInThread();
    
    srand((unsigned)time(0));

    // use our method to create a random voxel tree
    VoxelTree randomTree;

	// Check to see if the user passed in a command line option for loading a local
	// Voxel File. If so, load it now.
    bool wantColorRandomizer = !cmdOptionExists(argc, argv, "--NoColorRandomizer");
    const char* voxelsFilename = getCmdOption(argc, argv, "-i");
    if (voxelsFilename) {
	    randomTree.loadVoxelsFile(voxelsFilename,wantColorRandomizer);
	}
	if (!cmdOptionExists(argc, argv, "--NoRandomVoxelSheet")) {
		// create an octal code buffer and load it with 0 so that the recursive tree fill can give
		// octal codes to the tree nodes that it is creating
	    randomlyFillVoxelTree(MAX_VOXEL_TREE_DEPTH_LEVELS, randomTree.rootNode);
	}
	
	if (cmdOptionExists(argc, argv, "--AddRandomSpheres")) {
		addRandomSphere(&randomTree);
	}

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
        	// XXXBHG: Hacked in support for 'I' insert command
            if (packetData[0] == 'I') {
            	unsigned short int itemNumber = (*((unsigned short int*)&packetData[1]));
            	printf("got I command from client receivedBytes=%ld itemNumber=%d\n",receivedBytes,itemNumber);
            	int atByte = 3;
            	unsigned char* pVoxelData = (unsigned char*)&packetData[3];
            	while (atByte < receivedBytes) {
            		unsigned char octets = (unsigned char)*pVoxelData;
            		int voxelDataSize = bytesRequiredForCodeLength(octets)+3; // 3 for color!
		            randomTree.readCodeColorBufferToTree(pVoxelData);
	            	//printf("readCodeColorBufferToTree() of size=%d  atByte=%d receivedBytes=%ld\n",voxelDataSize,atByte,receivedBytes);
            		// skip to next
            		pVoxelData+=voxelDataSize;
            		atByte+=voxelDataSize;
            	}
            	//printf("about to call pruneTree()\n");
				//randomTree.pruneTree(randomTree.rootNode); // hack
            }
            if (packetData[0] == 'H') {
                if (agentList.addOrUpdateAgent(&agentPublicAddress, &agentPublicAddress, packetData[0], agentList.getLastAgentId())) {
                    agentList.increaseAgentId();
                }
                
                agentList.updateAgentWithData(&agentPublicAddress, (void *)packetData, receivedBytes);
                
                VoxelAgentData *agentData = (VoxelAgentData *) agentList.getAgents()[agentList.indexOfMatchingAgent(&agentPublicAddress)].getLinkedData();
                int newLevel = 10;
                if (newLevel > agentData->lastSentLevel) {
                    // the agent has already received a deeper level than this from us
                    // do nothing
                    
                    stopOctal = randomTree.rootNode->octalCode;
                    packetCount = 0;
                    totalBytesSent = 0;
                    randomTree.leavesWrittenToBitstream = 0;
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
                    
                    printf("%d packets sent to client totalling %d bytes\n", packetCount, totalBytesSent);
                    printf("%d leaves were sent - %f bpv\n",
                           randomTree.leavesWrittenToBitstream,
                           (float)totalBytesSent / randomTree.leavesWrittenToBitstream);
                    
                    agentData->lastSentLevel = newLevel;
                }
            }
        }
    }

    return 0;
}