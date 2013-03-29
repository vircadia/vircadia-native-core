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

void addSphere(VoxelTree * tree,bool random, bool wantColorRandomizer) {
	float r  = random ? randFloatInRange(0.05,0.1) : 0.25;
	float xc = random ? randFloatInRange(r,(1-r)) : 0.5;
	float yc = random ? randFloatInRange(r,(1-r)) : 0.5;
	float zc = random ? randFloatInRange(r,(1-r)) : 0.5;
	float s = (1.0/16); // size of voxels to make up surface of sphere
	bool solid = true;

	printf("adding sphere:");
	if (random)
		printf(" random");
	printf("\nradius=%f\n",r);
	printf("xc=%f\n",xc);
	printf("yc=%f\n",yc);
	printf("zc=%f\n",zc);

	tree->createSphere(r,xc,yc,zc,s,solid,wantColorRandomizer);
}

void addSphereScene(VoxelTree * tree, bool wantColorRandomizer) {
	printf("adding scene of spheres...\n");
	tree->createSphere(0.25,0.5,0.5,0.5,(1.0/256),true,wantColorRandomizer);
	tree->createSphere(0.030625,0.5,0.5,(0.25-0.06125),(1.0/2048),true,true);
}


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
    
    float treeRoot[3] = {0, 0, 0};
    
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

    // Handle Local Domain testing with the --local command line
    const char* local = "--local";
    bool wantLocalDomain = cmdOptionExists(argc, argv,local);
    if (wantLocalDomain) {
    	printf("Local Domain MODE!\n");
		int ip = getLocalAddress();
		sprintf(DOMAIN_IP,"%d.%d.%d.%d", (ip & 0xFF), ((ip >> 8) & 0xFF),((ip >> 16) & 0xFF), ((ip >> 24) & 0xFF));
    }

    agentList.linkedDataCreateCallback = &attachVoxelAgentDataToAgent;
    agentList.startSilentAgentRemovalThread();
    agentList.startDomainServerCheckInThread();
    
    srand((unsigned)time(0));
    
    // Check to see if the user passed in a command line option for loading a local
	// Voxel File. If so, load it now.
	const char* NO_COLOR_RANDOMIZER="--NoColorRandomizer";
	const char* INPUT_FILE="-i";
    bool wantColorRandomizer = !cmdOptionExists(argc, argv, NO_COLOR_RANDOMIZER);

	printf("wantColorRandomizer=%s\n",(wantColorRandomizer?"yes":"no"));
    const char* voxelsFilename = getCmdOption(argc, argv, INPUT_FILE);
    
    if (voxelsFilename) {
	    randomTree.loadVoxelsFile(voxelsFilename,wantColorRandomizer);
	}
    
	const char* NO_RANDOM_VOXELS="--NoRandomVoxels";
	if (!cmdOptionExists(argc, argv, NO_RANDOM_VOXELS)) {
		// create an octal code buffer and load it with 0 so that the recursive tree fill can give
		// octal codes to the tree nodes that it is creating
	    randomlyFillVoxelTree(MAX_VOXEL_TREE_DEPTH_LEVELS, randomTree.rootNode);
	}

	
	const char* ADD_SPHERE="--AddSphere";
	const char* ADD_RANDOM_SPHERE="--AddRandomSphere";
	if (cmdOptionExists(argc, argv, ADD_SPHERE)) {
		addSphere(&randomTree,false,wantColorRandomizer);
    } else if (cmdOptionExists(argc, argv, ADD_RANDOM_SPHERE)) {
		addSphere(&randomTree,true,wantColorRandomizer);
    }

	const char* ADD_SCENE="--AddScene";
	if (cmdOptionExists(argc, argv, ADD_SCENE)) {
		addSphereScene(&randomTree,wantColorRandomizer);
    }
    
    pthread_t sendVoxelThread;
    pthread_create(&sendVoxelThread, NULL, distributeVoxelsToListeners, NULL);
    
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
            }
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