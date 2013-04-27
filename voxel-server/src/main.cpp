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
#include <AgentTypes.h>
#include <VoxelTree.h>
#include "VoxelAgentData.h"
#include <SharedUtil.h>
#include <PacketHeaders.h>

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

const int VOXEL_SEND_INTERVAL_USECS = 100 * 1000;
const int PACKETS_PER_CLIENT_PER_INTERVAL = 5;

const int MAX_VOXEL_TREE_DEPTH_LEVELS = 4;

VoxelTree randomTree;

bool wantColorRandomizer = false;
bool debugViewFrustum = false;
bool viewFrustumCulling = true; // for now
bool newVoxelDistributor = false; // for now

void addSphere(VoxelTree * tree,bool random, bool wantColorRandomizer) {
	float r  = random ? randFloatInRange(0.05,0.1) : 0.25;
	float xc = random ? randFloatInRange(r,(1-r)) : 0.5;
	float yc = random ? randFloatInRange(r,(1-r)) : 0.5;
	float zc = random ? randFloatInRange(r,(1-r)) : 0.5;
	float s = (1.0/256); // size of voxels to make up surface of sphere
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
	
	// The old voxel distributor has a hard time with smaller voxels and more
	// complex scenes... so if we're using the old distributor make our scene
	// simple with larger sized voxels
	//int sphereBaseSize = ::newVoxelDistributor ? 512 : 256;
	int sphereBaseSize = 256;
	
	tree->createSphere(0.25,0.5,0.5,0.5,(1.0/sphereBaseSize),true,wantColorRandomizer);
	printf("one sphere added...\n");
	tree->createSphere(0.030625,0.5,0.5,(0.25-0.06125),(1.0/(sphereBaseSize*2)),true,true);
	printf("two spheres added...\n");
	tree->createSphere(0.030625,(1.0-0.030625),(1.0-0.030625),(1.0-0.06125),(1.0/(sphereBaseSize*2)),true,true);
	printf("three spheres added...\n");
	tree->createSphere(0.030625,(1.0-0.030625),(1.0-0.030625),0.06125,(1.0/(sphereBaseSize*2)),true,true);
	printf("four spheres added...\n");
	tree->createSphere(0.030625,(1.0-0.030625),0.06125,(1.0-0.06125),(1.0/(sphereBaseSize*2)),true,true);
	printf("five spheres added...\n");
	tree->createSphere(0.06125,0.125,0.125,(1.0-0.125),(1.0/(sphereBaseSize*2)),true,true);
	printf("DONE adding scene of spheres...\n");
}


void randomlyFillVoxelTree(int levelsToGo, VoxelNode *currentRootNode) {
    // randomly generate children for this node
    // the first level of the tree (where levelsToGo = MAX_VOXEL_TREE_DEPTH_LEVELS) has all 8
    if (levelsToGo > 0) {

        bool createdChildren = false;
        createdChildren = false;
        
        for (int i = 0; i < 8; i++) {
            if (true) {
                // create a new VoxelNode to put here
                currentRootNode->children[i] = new VoxelNode();
                
                // give this child it's octal code
                currentRootNode->children[i]->octalCode = childOctalCode(currentRootNode->octalCode, i);

                randomlyFillVoxelTree(levelsToGo - 1, currentRootNode->children[i]);
                createdChildren = true;
            }
        }
        
        if (!createdChildren) {
            // we didn't create any children for this node, making it a leaf
            // give it a random color
            currentRootNode->setRandomColor(MIN_BRIGHTNESS);
        } else {
            // set the color value for this node
            currentRootNode->setColorFromAverageOfChildren();
        }
    } else {
        // this is a leaf node, just give it a color
        currentRootNode->setRandomColor(MIN_BRIGHTNESS);
    }
}

void eraseVoxelTreeAndCleanupAgentVisitData() {

	// As our tree to erase all it's voxels
	::randomTree.eraseAllVoxels();

	// enumerate the agents clean up their marker nodes
    
	for (AgentList::iterator agent = AgentList::getInstance()->begin(); agent != AgentList::getInstance()->end(); agent++) {

		//printf("eraseVoxelTreeAndCleanupAgentVisitData() agent[%d]\n",i);
        
		VoxelAgentData* agentData = (VoxelAgentData *)agent->getLinkedData();
		
		if (agentData) {
            // clean up the agent visit data
            agentData->nodeBag.deleteAll();
            // old way
            delete agentData->rootMarkerNode;
            agentData->rootMarkerNode = new MarkerNode();
        }
	}
}


void newDistributeHelper(AgentList* agentList, AgentList::iterator& agent, VoxelAgentData* agentData, ViewFrustum& viewFrustum) {
    // If we don't have nodes already in our agent's node bag, then fill the node bag
    if (agentData->nodeBag.isEmpty()) {
        randomTree.searchForColoredNodes(randomTree.rootNode, viewFrustum, agentData->nodeBag);
    }

    // If we have something in our nodeBag, then turn them into packets and send them out...
    if (!agentData->nodeBag.isEmpty()) {
        unsigned char* tempOutputBuffer = new unsigned char[MAX_VOXEL_PACKET_SIZE-1]; // save 1 for "V" in final
        int bytesWritten = 0;

        // NOTE: we can assume the voxelPacket has already been set up with a "V"
        int packetsSentThisInterval = 0;

        while (packetsSentThisInterval < PACKETS_PER_CLIENT_PER_INTERVAL) {
            if (!agentData->nodeBag.isEmpty()) {
                VoxelNode* subTree = agentData->nodeBag.extract();

                // Only let this guy create at largest packets equal to the amount of space we have left in our final???
                // Or let it create the largest possible size (minus 1 for the "V")
                bytesWritten = randomTree.encodeTreeBitstream(subTree, viewFrustum, 
                        tempOutputBuffer, MAX_VOXEL_PACKET_SIZE-1, agentData->nodeBag);

                // if we have room in our final packet, add this buffer to the final packet
                if (agentData->getAvailable() >= bytesWritten) {
                    agentData->writeToPacket(tempOutputBuffer, bytesWritten);
                } else {
                    // otherwise "send" the packet because it's as full as we can make it for now
                    agentList->getAgentSocket().send(agent->getActiveSocket(), 
                                   agentData->getPacket(), agentData->getPacketLength());

                    // keep track that we sent it
                    packetsSentThisInterval++;

                    // reset our finalOutputBuffer (keep the 'V')
                    agentData->resetVoxelPacket();

                    // we also need to stick the last created partial packet in here!!
                    agentData->writeToPacket(tempOutputBuffer, bytesWritten);
                }
            } else {
                // we're here, if there are no more nodes in our bag waiting to be sent.
                // If we have a partial packet ready, then send it...
                if (agentData->isPacketWaiting()) {
                    agentList->getAgentSocket().send(agent->getActiveSocket(), 
                                    agentData->getPacket(), agentData->getPacketLength());

                    // reset our finalOutputBuffer (keep the 'V')
                    agentData->resetVoxelPacket();
                }

                // and we're done now for this interval, because we know we have not nodes in our
                // bag, and we just sent the last waiting packet if we had one, so tell ourselves
                // we done for the interval
                packetsSentThisInterval = PACKETS_PER_CLIENT_PER_INTERVAL;
            }
        }
    
        // end
        delete[] tempOutputBuffer;
    }
}

void *distributeVoxelsToListeners(void *args) {
    
    AgentList* agentList = AgentList::getInstance();
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
        for (AgentList::iterator agent = agentList->begin(); agent != agentList->end(); agent++) {
            VoxelAgentData* agentData = (VoxelAgentData *)agent->getLinkedData();

            // Sometimes the agent data has not yet been linked, in which case we can't really do anything
    		if (agentData) {
                ViewFrustum viewFrustum;
                // get position and orientation details from the camera
                viewFrustum.setPosition(agentData->getCameraPosition());
                viewFrustum.setOrientation(agentData->getCameraDirection(), agentData->getCameraUp(), agentData->getCameraRight());
    
                // Also make sure it's got the correct lens details from the camera
                viewFrustum.setFieldOfView(agentData->getCameraFov());
                viewFrustum.setAspectRatio(agentData->getCameraAspectRatio());
                viewFrustum.setNearClip(agentData->getCameraNearClip());
                viewFrustum.setFarClip(agentData->getCameraFarClip());
            
                viewFrustum.calculate();

                if (::newVoxelDistributor) {            
                    newDistributeHelper(agentList, agent, agentData, viewFrustum);
                } else {
                    stopOctal = NULL;
                    packetCount = 0;
                    totalBytesSent = 0;
                    randomTree.leavesWrittenToBitstream = 0;
            
                    for (int j = 0; j < PACKETS_PER_CLIENT_PER_INTERVAL; j++) {
                        voxelPacketEnd = voxelPacket;
                        stopOctal = randomTree.loadBitstreamBuffer(voxelPacketEnd,
                                                                   randomTree.rootNode,
                                                                   agentData->rootMarkerNode,
                                                                   agentData->getPosition(),
                                                                   treeRoot,
                                                                   viewFrustum,
                                                                   ::viewFrustumCulling,
                                                                   stopOctal);
                
                        agentList->getAgentSocket().send(agent->getActiveSocket(), voxelPacket, voxelPacketEnd - voxelPacket);
                
                        packetCount++;
                        totalBytesSent += voxelPacketEnd - voxelPacket;
                
                        // XXXBHG Hack Attack: This is temporary code to help debug an issue.
                        // Normally we use this break to prevent resending voxels that an agent has
                        // already visited. But since we might be modifying the voxel tree we might
                        // want to always send. This is a hack to test the behavior
                        bool alwaysSend = true;
                        if (!alwaysSend && agentData->rootMarkerNode->childrenVisitedMask == 255) {
                            break;
                        }
                    }
            
                    // for any agent that has a root marker node with 8 visited children
                    // recursively delete its marker nodes so we can revisit
                    if (agentData->rootMarkerNode->childrenVisitedMask == 255) {
                        delete agentData->rootMarkerNode;
                        agentData->rootMarkerNode = new MarkerNode();
                    }
                }
            }
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
    AgentList* agentList = AgentList::createInstance(AGENT_TYPE_VOXEL, VOXEL_LISTEN_PORT);
    setvbuf(stdout, NULL, _IOLBF, 0);

    // Handle Local Domain testing with the --local command line
    const char* local = "--local";
    bool wantLocalDomain = cmdOptionExists(argc, argv,local);
    if (wantLocalDomain) {
    	printf("Local Domain MODE!\n");
		int ip = getLocalAddress();
		sprintf(DOMAIN_IP,"%d.%d.%d.%d", (ip & 0xFF), ((ip >> 8) & 0xFF),((ip >> 16) & 0xFF), ((ip >> 24) & 0xFF));
    }

    agentList->linkedDataCreateCallback = &attachVoxelAgentDataToAgent;
    agentList->startSilentAgentRemovalThread();
    agentList->startDomainServerCheckInThread();
    
    srand((unsigned)time(0));

    const char* DEBUG_VIEW_FRUSTUM = "--DebugViewFrustum";
    ::debugViewFrustum = cmdOptionExists(argc, argv, DEBUG_VIEW_FRUSTUM);
	printf("debugViewFrustum=%s\n", (::debugViewFrustum ? "yes" : "no"));

    const char* NO_VIEW_FRUSTUM_CULLING = "--NoViewFrustumCulling";
    ::viewFrustumCulling = !cmdOptionExists(argc, argv, NO_VIEW_FRUSTUM_CULLING);
	printf("viewFrustumCulling=%s\n", (::viewFrustumCulling ? "yes" : "no"));
    
	const char* WANT_COLOR_RANDOMIZER = "--wantColorRandomizer";
    ::wantColorRandomizer = cmdOptionExists(argc, argv, WANT_COLOR_RANDOMIZER);
	printf("wantColorRandomizer=%s\n", (::wantColorRandomizer ? "yes" : "no"));

	const char* NEW_VOXEL_DISTRIBUTOR = "--newVoxelDistributor";
    ::newVoxelDistributor = cmdOptionExists(argc, argv, NEW_VOXEL_DISTRIBUTOR);
	printf("newVoxelDistributor=%s\n", (::newVoxelDistributor ? "yes" : "no"));
	
    // Check to see if the user passed in a command line option for loading a local
	// Voxel File. If so, load it now.
	const char* INPUT_FILE = "-i";
    const char* voxelsFilename = getCmdOption(argc, argv, INPUT_FILE);
    if (voxelsFilename) {
	    randomTree.loadVoxelsFile(voxelsFilename,wantColorRandomizer);
	}
    
	const char* ADD_RANDOM_VOXELS = "--AddRandomVoxels";
	if (cmdOptionExists(argc, argv, ADD_RANDOM_VOXELS)) {
		// create an octal code buffer and load it with 0 so that the recursive tree fill can give
		// octal codes to the tree nodes that it is creating
	    randomlyFillVoxelTree(MAX_VOXEL_TREE_DEPTH_LEVELS, randomTree.rootNode);
	}
	
	const char* ADD_SPHERE = "--AddSphere";
	const char* ADD_RANDOM_SPHERE = "--AddRandomSphere";
	if (cmdOptionExists(argc, argv, ADD_SPHERE)) {
		addSphere(&randomTree,false,wantColorRandomizer);
    } else if (cmdOptionExists(argc, argv, ADD_RANDOM_SPHERE)) {
		addSphere(&randomTree,true,wantColorRandomizer);
    }

	const char* NO_ADD_SCENE = "--NoAddScene";
	if (!cmdOptionExists(argc, argv, NO_ADD_SCENE)) {
		addSphereScene(&randomTree,wantColorRandomizer);
    }
    
    pthread_t sendVoxelThread;
    pthread_create(&sendVoxelThread, NULL, distributeVoxelsToListeners, NULL);
    
    sockaddr agentPublicAddress;
    
    unsigned char *packetData = new unsigned char[MAX_PACKET_SIZE];
    ssize_t receivedBytes;

    // loop to send to agents requesting data
    while (true) {
        if (agentList->getAgentSocket().receive(&agentPublicAddress, packetData, &receivedBytes)) {
        	// XXXBHG: Hacked in support for 'S' SET command
            if (packetData[0] == PACKET_HEADER_SET_VOXEL) {
            	unsigned short int itemNumber = (*((unsigned short int*)&packetData[1]));
            	printf("got I - insert voxels - command from client receivedBytes=%ld itemNumber=%d\n",
            		receivedBytes,itemNumber);
            	int atByte = 3;
            	unsigned char* pVoxelData = (unsigned char*)&packetData[3];
            	while (atByte < receivedBytes) {
            		unsigned char octets = (unsigned char)*pVoxelData;
            		int voxelDataSize = bytesRequiredForCodeLength(octets)+3; // 3 for color!
            		int voxelCodeSize = bytesRequiredForCodeLength(octets);

					// color randomization on insert
					int colorRandomizer = ::wantColorRandomizer ? randIntInRange (-50, 50) : 0;
					int red   = pVoxelData[voxelCodeSize+0];
					int green = pVoxelData[voxelCodeSize+1];
					int blue  = pVoxelData[voxelCodeSize+2];
            		printf("insert voxels - wantColorRandomizer=%s old r=%d,g=%d,b=%d \n",
            			(::wantColorRandomizer?"yes":"no"),red,green,blue);
					red   = std::max(0,std::min(255,red   + colorRandomizer));
					green = std::max(0,std::min(255,green + colorRandomizer));
					blue  = std::max(0,std::min(255,blue  + colorRandomizer));
            		printf("insert voxels - wantColorRandomizer=%s NEW r=%d,g=%d,b=%d \n",
            			(::wantColorRandomizer?"yes":"no"),red,green,blue);
					pVoxelData[voxelCodeSize+0]=red;
					pVoxelData[voxelCodeSize+1]=green;
					pVoxelData[voxelCodeSize+2]=blue;

            		float* vertices = firstVertexForCode(pVoxelData);
            		printf("inserting voxel at: %f,%f,%f\n",vertices[0],vertices[1],vertices[2]);
            		delete []vertices;
            		
		            randomTree.readCodeColorBufferToTree(pVoxelData);
	            	//printf("readCodeColorBufferToTree() of size=%d  atByte=%d receivedBytes=%ld\n",
	            	//		voxelDataSize,atByte,receivedBytes);
            		// skip to next
            		pVoxelData+=voxelDataSize;
            		atByte+=voxelDataSize;
            	}
            	// after done inserting all these voxels, then reaverage colors
				randomTree.reaverageVoxelColors(randomTree.rootNode);
            }
            if (packetData[0] == PACKET_HEADER_ERASE_VOXEL) {

            	// Send these bits off to the VoxelTree class to process them
				printf("got Erase Voxels message, have voxel tree do the work... randomTree.processRemoveVoxelBitstream()\n");
            	randomTree.processRemoveVoxelBitstream((unsigned char*)packetData,receivedBytes);

            	// Now send this to the connected agents so they know to delete
				printf("rebroadcasting delete voxel message to connected agents... agentList.broadcastToAgents()\n");
				agentList->broadcastToAgents(packetData,receivedBytes, &AGENT_TYPE_AVATAR, 1);
            	
            }
            if (packetData[0] == PACKET_HEADER_Z_COMMAND) {

            	// the Z command is a special command that allows the sender to send the voxel server high level semantic
            	// requests, like erase all, or add sphere scene
				char* command = (char*) &packetData[1]; // start of the command
				int commandLength = strlen(command); // commands are null terminated strings
                int totalLength = 1+commandLength+1;

				printf("got Z message len(%ld)= %s\n",receivedBytes,command);

				while (totalLength <= receivedBytes) {
					if (0==strcmp(command,(char*)"erase all")) {
						printf("got Z message == erase all\n");
						
						eraseVoxelTreeAndCleanupAgentVisitData();
					}
					if (0==strcmp(command,(char*)"add scene")) {
						printf("got Z message == add scene\n");
						addSphereScene(&randomTree,false);
					}
                    totalLength += commandLength+1;
				}

				// Now send this to the connected agents so they can also process these messages
				printf("rebroadcasting Z message to connected agents... agentList.broadcastToAgents()\n");
				agentList->broadcastToAgents(packetData,receivedBytes, &AGENT_TYPE_AVATAR, 1);
            }
            // If we got a PACKET_HEADER_HEAD_DATA, then we're talking to an AGENT_TYPE_AVATAR, and we
            // need to make sure we have it in our agentList.
            if (packetData[0] == PACKET_HEADER_HEAD_DATA) {
                if (agentList->addOrUpdateAgent(&agentPublicAddress,
                                               &agentPublicAddress,
                                               AGENT_TYPE_AVATAR,
                                               agentList->getLastAgentId())) {
                    agentList->increaseAgentId();
                }
                
                agentList->updateAgentWithData(&agentPublicAddress, packetData, receivedBytes);
            }
        }
    }
    
    pthread_join(sendVoxelThread, NULL);

    return 0;
}