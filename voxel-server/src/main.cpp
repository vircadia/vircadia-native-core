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


const int VOXEL_SIZE_BYTES = 3 + (3 * sizeof(float));
const int VOXELS_PER_PACKET = (MAX_PACKET_SIZE - 1) / VOXEL_SIZE_BYTES;

const int MIN_BRIGHTNESS = 64;
const float DEATH_STAR_RADIUS = 4.0;
const float MAX_CUBE = 0.05f;

const int VOXEL_SEND_INTERVAL_USECS = 100 * 1000;
const int PACKETS_PER_CLIENT_PER_INTERVAL = 2;

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

int _nodeCount=0;
bool countVoxelsOperation(VoxelNode* node, bool down, void* extraData) {
    if (down) {
        if (node->isColored()){
            _nodeCount++;
        }
    }
    return true; // keep going
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

    float radius = 0.0125f;
	printf("6 spheres added...\n");
	tree->createSphere(radius,0.25,radius*5.0f,0.25,(1.0/(4096)),true,true);
	printf("7 spheres added...\n");
	tree->createSphere(radius,0.125,radius*5.0f,0.25,(1.0/(4096)),true,true);
	printf("8 spheres added...\n");
	tree->createSphere(radius,0.075,radius*5.0f,0.25,(1.0/(4096)),true,true);
	printf("9 spheres added...\n");
	tree->createSphere(radius,0.05,radius*5.0f,0.25,(1.0/(4096)),true,true);
	printf("10 spheres added...\n");
	tree->createSphere(radius,0.025,radius*5.0f,0.25,(1.0/(4096)),true,true);
	printf("11 spheres added...\n");

    _nodeCount=0;
    tree->recurseTreeWithOperation(countVoxelsOperation);
    printf("Nodes after adding scene %d nodes\n",_nodeCount);


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
    // A quick explanation of the strategy here. First, each time through, we ask ourselves, do we have voxels
    // that need to be sent? If not, we search for them, if we do, then we send them. We store our to be sent voxel sub trees
    // in a VoxelNodeBag on a per agent basis. The bag stores just pointers to the root node of the sub tree to be sent, so
    // it doesn't store much on a per agent basis.
    //
    // There could be multiple strategies at play for how we determine which voxels need to be sent. For example, at the
    // simplest level, we can just add the root node to this bag, and let the system send it. The other thing that we use
    // this bag for is, keeping track of voxels sub trees we wanted to send in the packet, but wouldn't fit into the current
    // packet because they were too big once encoded. So, as we run though this function multiple times, we start out with
    // voxel sub trees that we determined needed to be sent because they were in view, new, correct distance, etc. But as
    // we send those sub trees, if their child trees don't fit in a packet, we'll add those sub trees to this bag as well, and
    // next chance we get, we'll also send those needed sub trees.
    // If we don't have nodes already in our agent's node bag, then fill the node bag
    if (agentData->nodeBag.isEmpty()) {
    
        // To get things started, we look for colored nodes. We could have also just started with the root node. In fact, if
        // you substitute this call with something as simple as agentData->nodeBag.insert(rootNod), you'll see almost the same
        // behavior on the client. The scene will appear. 
        //
        // So why do we do this extra effort to look for colored nodes? It turns out that there are subtle differences between
        // how many bytes it takes to encode a tree based on how deep it is relative to the root node (which effects the octal
        // code size) vs how dense the tree is (which effects how many bits in the bitMask are being wasted, and maybe more
        // importantly, how many subtrees are also included). There is a break point where the more dense a tree is, it's more
        // efficient to encode the peers together with their empty parents. This would argue that we shouldn't search for these
        // sub trees, and we should instead encode the parent for dense scenes.
        //
        // But, there's another important side effect of dense trees related to out maximum packet size. Namely... if a tree
        // is very dense, then you can't fit as many branches in a single network packet. Because when we encode the parent and
        // children in a single packet, we must include the entire child branch (all the way down to our target LOD) before we
        // can include the siblings. Since dense trees take more space per ranch, we often end up only being able to encode a
        // single branch. This means on a per packet basis, the trees actually _are not_ dense. And sparse trees are shorter to
        // encode when we only include the child tree.
        //
        // Now, a quick explanation of maxSearchLevel: We will actually send the entire scene, multiple times for each search
        // level. We start at level 1, and we scan the scene for this level, then we increment to the next level until we've
        // sent the entire scene at it's deepest possible level. This means that clients will get an initial view of the scene
        // with chunky granularity and then finer and finer granularity until they've gotten the whole scene. Then we start
        // over to handle packet loss and changes in the scene. 
        int maxLevelReached = randomTree.searchForColoredNodes(agentData->getMaxSearchLevel(), randomTree.rootNode, 
                                                                        viewFrustum, agentData->nodeBag);
        agentData->setMaxLevelReached(maxLevelReached);
        
        // If nothing got added, then we bump our levels.
        if (agentData->nodeBag.isEmpty()) {
            if (agentData->getMaxLevelReached() < agentData->getMaxSearchLevel()) {
                agentData->resetMaxSearchLevel();
            } else {
                agentData->incrementMaxSearchLevel();
            }
        }
    }

    // If we have something in our nodeBag, then turn them into packets and send them out...
    if (!agentData->nodeBag.isEmpty()) {
        static unsigned char tempOutputBuffer[MAX_VOXEL_PACKET_SIZE-1]; // save on allocs by making this static
        int bytesWritten = 0;

        // NOTE: we can assume the voxelPacket has already been set up with a "V"
        int packetsSentThisInterval = 0;

        while (packetsSentThisInterval < PACKETS_PER_CLIENT_PER_INTERVAL) {
            if (!agentData->nodeBag.isEmpty()) {
                VoxelNode* subTree = agentData->nodeBag.extract();

                // Only let this guy create at largest packets equal to the amount of space we have left in our final???
                // Or let it create the largest possible size (minus 1 for the "V")
                bytesWritten = randomTree.encodeTreeBitstream(agentData->getMaxSearchLevel(), subTree, viewFrustum, 
                        &tempOutputBuffer[0], MAX_VOXEL_PACKET_SIZE-1, agentData->nodeBag);

                // if we have room in our final packet, add this buffer to the final packet
                if (agentData->getAvailable() >= bytesWritten) {
                    agentData->writeToPacket(&tempOutputBuffer[0], bytesWritten);
                } else {
                    // otherwise "send" the packet because it's as full as we can make it for now
                    agentList->getAgentSocket().send(agent->getActiveSocket(), 
                                   agentData->getPacket(), agentData->getPacketLength());

                    // keep track that we sent it
                    packetsSentThisInterval++;

                    // reset our finalOutputBuffer (keep the 'V')
                    agentData->resetVoxelPacket();

                    // we also need to stick the last created partial packet in here!!
                    agentData->writeToPacket(&tempOutputBuffer[0], bytesWritten);
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

        // Ok, so we're in the "send from our bag mode"... if during this last pass, we emptied our bag, then
        // we want to move to the next level.
        if (agentData->nodeBag.isEmpty()) {
            if (agentData->getMaxLevelReached() < agentData->getMaxSearchLevel()) {
                agentData->resetMaxSearchLevel();
            } else {
                agentData->incrementMaxSearchLevel();
            }
        }        
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