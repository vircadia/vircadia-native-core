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
int PACKETS_PER_CLIENT_PER_INTERVAL = 20;

const int MAX_VOXEL_TREE_DEPTH_LEVELS = 4;

VoxelTree randomTree;
bool wantVoxelPersist = false;

bool wantColorRandomizer = false;
bool debugVoxelSending = false;

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
bool countVoxelsOperation(VoxelNode* node, void* extraData) {
    if (node->isColored()){
        _nodeCount++;
    }
    return true; // keep going
}

void addSphereScene(VoxelTree * tree, bool wantColorRandomizer) {
    printf("adding scene...\n");

    float voxelSize = 1.f/32;
    printf("creating corner points...\n");
    tree->createVoxel(0              , 0              , 0              , voxelSize, 255, 255 ,255);

    if (tree->getVoxelAt(0, 0, 0, voxelSize)) {
        printf("corner point 0,0,0 exists...\n");
    }

    tree->deleteVoxelAt(0, 0, 0, voxelSize);
    printf("attempting to delete corner point 0,0,0\n");

    if (tree->getVoxelAt(0, 0, 0, voxelSize)) {
        printf("corner point 0,0,0 exists...\n");
    } else {
        printf("corner point 0,0,0 does not exists...\n");
    }

    printf("creating corner points...\n");
    tree->createVoxel(0              , 0              , 0              , voxelSize, 255, 255 ,255);
    
    printf("saving file voxels.hio2...\n");
    tree->writeToFileV2("voxels.hio2");
    printf("erasing all voxels...\n");
    tree->eraseAllVoxels();

    if (tree->getVoxelAt(0, 0, 0, voxelSize)) {
        printf("corner point 0,0,0 exists...\n");
    } else {
        printf("corner point 0,0,0 does not exists...\n");
    }

    printf("loading file voxels.hio2...\n");
    tree->readFromFileV2("voxels.hio2",true);

    if (tree->getVoxelAt(0, 0, 0, voxelSize)) {
        printf("corner point 0,0,0 exists...\n");
    } else {
        printf("corner point 0,0,0 does not exists...\n");
    }


    tree->createVoxel(1.0 - voxelSize, 0              , 0              , voxelSize, 255, 0   ,0  );
    tree->createVoxel(0              , 1.0 - voxelSize, 0              , voxelSize, 0  , 255 ,0  );
    tree->createVoxel(0              , 0              , 1.0 - voxelSize, voxelSize, 0  , 0   ,255);


    tree->createVoxel(1.0 - voxelSize, 0              , 1.0 - voxelSize, voxelSize, 255, 0   ,255);
    tree->createVoxel(0              , 1.0 - voxelSize, 1.0 - voxelSize, voxelSize, 0  , 255 ,255);
    tree->createVoxel(1.0 - voxelSize, 1.0 - voxelSize, 0              , voxelSize, 255, 255 ,0  );
    tree->createVoxel(1.0 - voxelSize, 1.0 - voxelSize, 1.0 - voxelSize, voxelSize, 255, 255 ,255);
    printf("DONE creating corner points...\n");

    printf("creating voxel lines...\n");
    float lineVoxelSize = 0.99f/256;
    rgbColor red   = {255,0,0};
    rgbColor green = {0,255,0};
    rgbColor blue  = {0,0,255};

    tree->createLine(glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), lineVoxelSize, blue);
    tree->createLine(glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), lineVoxelSize, red);
    tree->createLine(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), lineVoxelSize, green);

    printf("DONE creating lines...\n");

    int sphereBaseSize = 512;
    printf("creating spheres...\n");
    tree->createSphere(0.25, 0.5, 0.5, 0.5, (1.0 / sphereBaseSize), true, wantColorRandomizer);
    printf("one sphere added...\n");
    tree->createSphere(0.030625, 0.5, 0.5, (0.25-0.06125), (1.0 / (sphereBaseSize * 2)), true, true);


    printf("two spheres added...\n");
    tree->createSphere(0.030625, (0.75 - 0.030625), (0.75 - 0.030625), (0.75 - 0.06125), (1.0 / (sphereBaseSize * 2)), true, true);
    printf("three spheres added...\n");
    tree->createSphere(0.030625, (0.75 - 0.030625), (0.75 - 0.030625), 0.06125, (1.0 / (sphereBaseSize * 2)), true, true);
    printf("four spheres added...\n");
    tree->createSphere(0.030625, (0.75 - 0.030625), 0.06125, (0.75 - 0.06125), (1.0 / (sphereBaseSize * 2)), true, true);
    printf("five spheres added...\n");
    tree->createSphere(0.06125, 0.125, 0.125, (0.75 - 0.125), (1.0 / (sphereBaseSize * 2)), true, true);

    float radius = 0.0125f;
    printf("6 spheres added...\n");
    tree->createSphere(radius, 0.25, radius * 5.0f, 0.25, (1.0 / 4096), true, true);
    printf("7 spheres added...\n");
    tree->createSphere(radius, 0.125, radius * 5.0f, 0.25, (1.0 / 4096), true, true);
    printf("8 spheres added...\n");
    tree->createSphere(radius, 0.075, radius * 5.0f, 0.25, (1.0 / 4096), true, true);
    printf("9 spheres added...\n");
    tree->createSphere(radius, 0.05, radius * 5.0f, 0.25, (1.0 / 4096), true, true);
    printf("10 spheres added...\n");
    tree->createSphere(radius, 0.025, radius * 5.0f, 0.25, (1.0 / 4096), true, true);
    printf("11 spheres added...\n");

    printf("DONE creating spheres...\n");

    _nodeCount=0;
    tree->recurseTreeWithOperation(countVoxelsOperation);
    printf("Nodes after adding scene %d nodes\n", _nodeCount);


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
        VoxelAgentData* agentData = (VoxelAgentData*) agent->getLinkedData();
        if (agentData) {
            // clean up the agent visit data
            agentData->nodeBag.deleteAll();
        }
    }
}


void voxelDistributor(AgentList* agentList, AgentList::iterator& agent, VoxelAgentData* agentData, ViewFrustum& viewFrustum) {
    bool searchReset = false;
    int  searchLoops = 0;
    int  searchLevelWas = agentData->getMaxSearchLevel();
    double start = usecTimestampNow();
    while (!searchReset && agentData->nodeBag.isEmpty()) {
        searchLoops++;

        searchLevelWas = agentData->getMaxSearchLevel();
        int maxLevelReached = randomTree.searchForColoredNodes(agentData->getMaxSearchLevel(), randomTree.rootNode, 
                                                               viewFrustum, agentData->nodeBag);
        agentData->setMaxLevelReached(maxLevelReached);
        
        // If nothing got added, then we bump our levels.
        if (agentData->nodeBag.isEmpty()) {
            if (agentData->getMaxLevelReached() < agentData->getMaxSearchLevel()) {
                agentData->resetMaxSearchLevel();
                searchReset = true;
            } else {
                agentData->incrementMaxSearchLevel();
            }
        }
    }
    double end = usecTimestampNow();
    double elapsedmsec = (end - start)/1000.0;
    if (elapsedmsec > 100) {
        if (elapsedmsec > 1000) {
            double elapsedsec = (end - start)/1000000.0;
            printf("WARNING! searchForColoredNodes() took %lf seconds to identify %d nodes at level %d in %d loops\n",
                elapsedsec, agentData->nodeBag.count(), searchLevelWas, searchLoops);
        } else {
            printf("WARNING! searchForColoredNodes() took %lf milliseconds to identify %d nodes at level %d in %d loops\n",
                elapsedmsec, agentData->nodeBag.count(), searchLevelWas, searchLoops);
        }
    } else if (::debugVoxelSending) {
        printf("searchForColoredNodes() took %lf milliseconds to identify %d nodes at level %d in %d loops\n",
                elapsedmsec, agentData->nodeBag.count(), searchLevelWas, searchLoops);
    }


    // If we have something in our nodeBag, then turn them into packets and send them out...
    if (!agentData->nodeBag.isEmpty()) {
        static unsigned char tempOutputBuffer[MAX_VOXEL_PACKET_SIZE - 1]; // save on allocs by making this static
        int bytesWritten = 0;
        int packetsSentThisInterval = 0;
        int truePacketsSent = 0;
        int trueBytesSent = 0;
        double start = usecTimestampNow();

        while (packetsSentThisInterval < PACKETS_PER_CLIENT_PER_INTERVAL) {
            if (!agentData->nodeBag.isEmpty()) {
                VoxelNode* subTree = agentData->nodeBag.extract();
                bytesWritten = randomTree.encodeTreeBitstream(agentData->getMaxSearchLevel(), subTree,
                                                              &tempOutputBuffer[0], MAX_VOXEL_PACKET_SIZE - 1, 
                                                              agentData->nodeBag, &viewFrustum);

                if (agentData->getAvailable() >= bytesWritten) {
                    agentData->writeToPacket(&tempOutputBuffer[0], bytesWritten);
                } else {
                    agentList->getAgentSocket().send(agent->getActiveSocket(), 
                                                     agentData->getPacket(), agentData->getPacketLength());
                    trueBytesSent += agentData->getPacketLength();
                    truePacketsSent++;
                    packetsSentThisInterval++;
                    agentData->resetVoxelPacket();
                    agentData->writeToPacket(&tempOutputBuffer[0], bytesWritten);
                }
            } else {
                if (agentData->isPacketWaiting()) {
                    agentList->getAgentSocket().send(agent->getActiveSocket(), 
                                                     agentData->getPacket(), agentData->getPacketLength());
                    trueBytesSent += agentData->getPacketLength();
                    truePacketsSent++;
                    agentData->resetVoxelPacket();
                    
                }
                packetsSentThisInterval = PACKETS_PER_CLIENT_PER_INTERVAL; // done for now, no nodes left
            }
        }
        double end = usecTimestampNow();
        double elapsedmsec = (end - start)/1000.0;
        if (elapsedmsec > 100) {
            if (elapsedmsec > 1000) {
                double elapsedsec = (end - start)/1000000.0;
                printf("WARNING! packetLoop() took %lf seconds to generate %d bytes in %d packets at level %d, %d nodes still to send\n",
                        elapsedsec, trueBytesSent, truePacketsSent, searchLevelWas, agentData->nodeBag.count());
            } else {
                printf("WARNING! packetLoop() took %lf milliseconds to generate %d bytes in %d packets at level %d, %d nodes still to send\n",
                        elapsedmsec, trueBytesSent, truePacketsSent, searchLevelWas, agentData->nodeBag.count());
            }
        } else if (::debugVoxelSending) {
            printf("packetLoop() took %lf milliseconds to generate %d bytes in %d packets at level %d, %d nodes still to send\n",
                    elapsedmsec, trueBytesSent, truePacketsSent, searchLevelWas, agentData->nodeBag.count());
        }

        // if during this last pass, we emptied our bag, then we want to move to the next level.
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
    
    while (true) {
        gettimeofday(&lastSendTime, NULL);
        
        // enumerate the agents to send 3 packets to each
        for (AgentList::iterator agent = agentList->begin(); agent != agentList->end(); agent++) {
            VoxelAgentData* agentData = (VoxelAgentData*) agent->getLinkedData();

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

                voxelDistributor(agentList, agent, agentData, viewFrustum);
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

void terminate (int sig) {
    printf("terminating now...\n");
    
    // This apparently does not work, because the randomTree has already been freed before this
    // sig term handler runs... So, we need to find a better solution than this.
    if (false) {
        _nodeCount=0;
        ::randomTree.recurseTreeWithOperation(countVoxelsOperation);
        printf("Nodes in scene before saving: %d nodes\n", _nodeCount);

        printf("saving voxels to file...\n");
        randomTree.writeToFileV2("voxels.hio2");
        printf("DONE saving voxels to file...\n");
    }
    exit(EXIT_SUCCESS);
}


int main(int argc, const char * argv[])
{
    signal(SIGABRT,&terminate);
    signal(SIGTERM,&terminate);
    signal(SIGINT,&terminate);
    
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

	const char* DEBUG_VOXEL_SENDING = "--debugVoxelSending";
    ::debugVoxelSending = cmdOptionExists(argc, argv, DEBUG_VOXEL_SENDING);
	printf("debugVoxelSending=%s\n", (::debugVoxelSending ? "yes" : "no"));
    
	const char* WANT_COLOR_RANDOMIZER = "--wantColorRandomizer";
    ::wantColorRandomizer = cmdOptionExists(argc, argv, WANT_COLOR_RANDOMIZER);
	printf("wantColorRandomizer=%s\n", (::wantColorRandomizer ? "yes" : "no"));

	const char* WANT_VOXEL_PERSIST = "--wantVoxelPersist";
    ::wantVoxelPersist = cmdOptionExists(argc, argv, WANT_VOXEL_PERSIST);
	printf("wantVoxelPersist=%s\n", (::wantVoxelPersist ? "yes" : "no"));

    // if we want Voxel Persistance, load the local file now...
    if (::wantVoxelPersist) {
        printf("loading voxels from file...\n");
        ::randomTree.readFromFileV2("voxels.hio2",true);
        printf("DONE loading voxels from file...\n");
        _nodeCount=0;
        ::randomTree.recurseTreeWithOperation(countVoxelsOperation);
        printf("Nodes after loading scene %d nodes\n", _nodeCount);
    }

    // Check to see if the user passed in a command line option for loading an old style local
	// Voxel File. If so, load it now. This is not the same as a voxel persist file
	const char* INPUT_FILE = "-i";
    const char* voxelsFilename = getCmdOption(argc, argv, INPUT_FILE);
    if (voxelsFilename) {
	    randomTree.loadVoxelsFile(voxelsFilename,wantColorRandomizer);
	}

    // Check to see if the user passed in a command line option for setting packet send rate
	const char* PACKETS_PER_SECOND = "--packetsPerSecond";
    const char* packetsPerSecond = getCmdOption(argc, argv, PACKETS_PER_SECOND);
    if (packetsPerSecond) {
	    PACKETS_PER_CLIENT_PER_INTERVAL = atoi(packetsPerSecond)/10;
	    if (PACKETS_PER_CLIENT_PER_INTERVAL < 1) {
	        PACKETS_PER_CLIENT_PER_INTERVAL = 1;
	    }
    	printf("packetsPerSecond=%s PACKETS_PER_CLIENT_PER_INTERVAL=%d\n", packetsPerSecond, PACKETS_PER_CLIENT_PER_INTERVAL);
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