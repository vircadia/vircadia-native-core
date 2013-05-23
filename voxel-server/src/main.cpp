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
#include <EnvironmentData.h>
#include <VoxelTree.h>
#include "VoxelAgentData.h"
#include <SharedUtil.h>
#include <PacketHeaders.h>
#include <SceneUtils.h>
#include <PerfStat.h>

#ifdef _WIN32
#include "Syssocket.h"
#include "Systime.h"
#else
#include <sys/time.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#endif

const char* LOCAL_VOXELS_PERSIST_FILE = "resources/voxels.hio2";
const char* VOXELS_PERSIST_FILE = "/etc/highfidelity/voxel-server/resources/voxels.hio2";
const double VOXEL_PERSIST_INTERVAL = 1000.0 * 30; // every 30 seconds

const int VOXEL_LISTEN_PORT = 40106;


const int VOXEL_SIZE_BYTES = 3 + (3 * sizeof(float));
const int VOXELS_PER_PACKET = (MAX_PACKET_SIZE - 1) / VOXEL_SIZE_BYTES;

const int MIN_BRIGHTNESS = 64;
const float DEATH_STAR_RADIUS = 4.0;
const float MAX_CUBE = 0.05f;

const int VOXEL_SEND_INTERVAL_USECS = 100 * 1000;
int PACKETS_PER_CLIENT_PER_INTERVAL = 50;

const int MAX_VOXEL_TREE_DEPTH_LEVELS = 4;

const int ENVIRONMENT_SEND_INTERVAL_USECS = 1000000;

VoxelTree randomTree(false); // this is NOT a reaveraging tree 
bool wantVoxelPersist = true;
bool wantLocalDomain = false;


bool wantColorRandomizer = false;
bool debugVoxelSending = false;
bool shouldShowAnimationDebug = false;
bool wantSearchForColoredNodes = false;

EnvironmentData environmentData[3];


void randomlyFillVoxelTree(int levelsToGo, VoxelNode *currentRootNode) {
    // randomly generate children for this node
    // the first level of the tree (where levelsToGo = MAX_VOXEL_TREE_DEPTH_LEVELS) has all 8
    if (levelsToGo > 0) {

        bool createdChildren = false;
        createdChildren = false;
        
        for (int i = 0; i < 8; i++) {
            if (true) {
                // create a new VoxelNode to put here
                currentRootNode->addChildAtIndex(i);
                randomlyFillVoxelTree(levelsToGo - 1, currentRootNode->getChildAtIndex(i));
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


// Version of voxel distributor that sends each LOD level at a time
void resInVoxelDistributor(AgentList* agentList, 
                           AgentList::iterator& agent, 
                           VoxelAgentData* agentData) {
    ViewFrustum viewFrustum = agentData->getCurrentViewFrustum();
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

        bool shouldSendEnvironments = shouldDo(ENVIRONMENT_SEND_INTERVAL_USECS, VOXEL_SEND_INTERVAL_USECS);
        while (packetsSentThisInterval < PACKETS_PER_CLIENT_PER_INTERVAL - (shouldSendEnvironments ? 1 : 0)) {
            if (!agentData->nodeBag.isEmpty()) {
                VoxelNode* subTree = agentData->nodeBag.extract();
                bytesWritten = randomTree.encodeTreeBitstream(agentData->getMaxSearchLevel(), subTree,
                                                              &tempOutputBuffer[0], MAX_VOXEL_PACKET_SIZE - 1, 
                                                              agentData->nodeBag, &viewFrustum,
                                                              agentData->getWantColor(), WANT_EXISTS_BITS);

                if (agentData->getAvailable() >= bytesWritten) {
                    agentData->writeToPacket(&tempOutputBuffer[0], bytesWritten);
                } else {
                    agentList->getAgentSocket()->send(agent->getActiveSocket(),
                                                     agentData->getPacket(), agentData->getPacketLength());
                    trueBytesSent += agentData->getPacketLength();
                    truePacketsSent++;
                    packetsSentThisInterval++;
                    agentData->resetVoxelPacket();
                    agentData->writeToPacket(&tempOutputBuffer[0], bytesWritten);
                }
            } else {
                if (agentData->isPacketWaiting()) {
                    agentList->getAgentSocket()->send(agent->getActiveSocket(),
                                                      agentData->getPacket(), agentData->getPacketLength());
                    trueBytesSent += agentData->getPacketLength();
                    truePacketsSent++;
                    agentData->resetVoxelPacket();
                    
                }
                packetsSentThisInterval = PACKETS_PER_CLIENT_PER_INTERVAL; // done for now, no nodes left
            }
        }
        // send the environment packets
        if (shouldSendEnvironments) {
            int envPacketLength = 1;
            *tempOutputBuffer = PACKET_HEADER_ENVIRONMENT_DATA;
            for (int i = 0; i < sizeof(environmentData) / sizeof(environmentData[0]); i++) {
                envPacketLength += environmentData[i].getBroadcastData(tempOutputBuffer + envPacketLength);
            }
            agentList->getAgentSocket()->send(agent->getActiveSocket(), tempOutputBuffer, envPacketLength);
            trueBytesSent += envPacketLength;
            truePacketsSent++;
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

pthread_mutex_t treeLock;

// Version of voxel distributor that sends the deepest LOD level at once
void deepestLevelVoxelDistributor(AgentList* agentList, 
                                  AgentList::iterator& agent,
                                  VoxelAgentData* agentData,
                                  bool viewFrustumChanged) {


    pthread_mutex_lock(&::treeLock);

    int maxLevelReached = 0;
    double start = usecTimestampNow();

    // FOR NOW... agent tells us if it wants to receive only view frustum deltas
    bool wantDelta = agentData->getWantDelta();
    const ViewFrustum* lastViewFrustum =  wantDelta ? &agentData->getLastKnownViewFrustum() : NULL;

    if (::debugVoxelSending) {
        printf("deepestLevelVoxelDistributor() viewFrustumChanged=%s, nodeBag.isEmpty=%s, viewSent=%s\n",
                debug::valueOf(viewFrustumChanged), debug::valueOf(agentData->nodeBag.isEmpty()), 
                debug::valueOf(agentData->getViewSent())
            );
    }
    
    // If the current view frustum has changed OR we have nothing to send, then search against 
    // the current view frustum for things to send.
    if (viewFrustumChanged || agentData->nodeBag.isEmpty()) {

        // For now, we're going to disable the "search for colored nodes" because that strategy doesn't work when we support
        // deletion of nodes. Instead if we just start at the root we get the correct behavior we want. We are keeping this
        // code for now because we want to be able to go back to it and find a solution to support both. The search method
        // helps improve overall bitrate performance.
        if (::wantSearchForColoredNodes) {
            // If the bag was empty, then send everything in view, not just the delta
            maxLevelReached = randomTree.searchForColoredNodes(INT_MAX, randomTree.rootNode, agentData->getCurrentViewFrustum(), 
                                                               agentData->nodeBag, wantDelta, lastViewFrustum);

            // if nothing was found in view, send the root node.
            if (agentData->nodeBag.isEmpty()){
                agentData->nodeBag.insert(randomTree.rootNode);
            }
            agentData->setViewSent(false);
        } else {
            agentData->nodeBag.insert(randomTree.rootNode);
        }

    }
    double end = usecTimestampNow();
    double elapsedmsec = (end - start)/1000.0;
    if (elapsedmsec > 100) {
        if (elapsedmsec > 1000) {
            double elapsedsec = (end - start)/1000000.0;
            printf("WARNING! searchForColoredNodes() took %lf seconds to identify %d nodes at level %d\n",
                elapsedsec, agentData->nodeBag.count(), maxLevelReached);
        } else {
            printf("WARNING! searchForColoredNodes() took %lf milliseconds to identify %d nodes at level %d\n",
                elapsedmsec, agentData->nodeBag.count(), maxLevelReached);
        }
    } else if (::debugVoxelSending) {
        printf("searchForColoredNodes() took %lf milliseconds to identify %d nodes at level %d\n",
                elapsedmsec, agentData->nodeBag.count(), maxLevelReached);
    }

    // If we have something in our nodeBag, then turn them into packets and send them out...
    if (!agentData->nodeBag.isEmpty()) {
        static unsigned char tempOutputBuffer[MAX_VOXEL_PACKET_SIZE - 1]; // save on allocs by making this static
        int bytesWritten = 0;
        int packetsSentThisInterval = 0;
        int truePacketsSent = 0;
        int trueBytesSent = 0;
        double start = usecTimestampNow();

        bool shouldSendEnvironments = shouldDo(ENVIRONMENT_SEND_INTERVAL_USECS, VOXEL_SEND_INTERVAL_USECS);
        while (packetsSentThisInterval < PACKETS_PER_CLIENT_PER_INTERVAL - (shouldSendEnvironments ? 1 : 0)) {
            if (!agentData->nodeBag.isEmpty()) {
                VoxelNode* subTree = agentData->nodeBag.extract();
                bytesWritten = randomTree.encodeTreeBitstream(INT_MAX, subTree,
                                                              &tempOutputBuffer[0], MAX_VOXEL_PACKET_SIZE - 1, 
                                                              agentData->nodeBag, &agentData->getCurrentViewFrustum(),
                                                              agentData->getWantColor(), WANT_EXISTS_BITS,
                                                              wantDelta, lastViewFrustum);

                if (agentData->getAvailable() >= bytesWritten) {
                    agentData->writeToPacket(&tempOutputBuffer[0], bytesWritten);
                } else {
                    agentList->getAgentSocket()->send(agent->getActiveSocket(),
                                                     agentData->getPacket(), agentData->getPacketLength());
                    trueBytesSent += agentData->getPacketLength();
                    truePacketsSent++;
                    packetsSentThisInterval++;
                    agentData->resetVoxelPacket();
                    agentData->writeToPacket(&tempOutputBuffer[0], bytesWritten);
                }
            } else {
                if (agentData->isPacketWaiting()) {
                    agentList->getAgentSocket()->send(agent->getActiveSocket(),
                                                     agentData->getPacket(), agentData->getPacketLength());
                    trueBytesSent += agentData->getPacketLength();
                    truePacketsSent++;
                    agentData->resetVoxelPacket();
                    
                }
                packetsSentThisInterval = PACKETS_PER_CLIENT_PER_INTERVAL; // done for now, no nodes left
            }
        }
        // send the environment packet
        if (shouldSendEnvironments) {
            int envPacketLength = 1;
            *tempOutputBuffer = PACKET_HEADER_ENVIRONMENT_DATA;
            for (int i = 0; i < sizeof(environmentData) / sizeof(environmentData[0]); i++) {
                envPacketLength += environmentData[i].getBroadcastData(tempOutputBuffer + envPacketLength);
            }
            agentList->getAgentSocket()->send(agent->getActiveSocket(), tempOutputBuffer, envPacketLength);
            trueBytesSent += envPacketLength;
            truePacketsSent++;
        }
        
        double end = usecTimestampNow();
        double elapsedmsec = (end - start)/1000.0;
        if (elapsedmsec > 100) {
            if (elapsedmsec > 1000) {
                double elapsedsec = (end - start)/1000000.0;
                printf("WARNING! packetLoop() took %lf seconds to generate %d bytes in %d packets %d nodes still to send\n",
                        elapsedsec, trueBytesSent, truePacketsSent, agentData->nodeBag.count());
            } else {
                printf("WARNING! packetLoop() took %lf milliseconds to generate %d bytes in %d packets, %d nodes still to send\n",
                        elapsedmsec, trueBytesSent, truePacketsSent, agentData->nodeBag.count());
            }
        } else if (::debugVoxelSending) {
            printf("packetLoop() took %lf milliseconds to generate %d bytes in %d packets, %d nodes still to send\n",
                    elapsedmsec, trueBytesSent, truePacketsSent, agentData->nodeBag.count());
        }
        
        // if after sending packets we've emptied our bag, then we want to remember that we've sent all 
        // the voxels from the current view frustum
        if (agentData->nodeBag.isEmpty()) {
            agentData->updateLastKnownViewFrustum();
            agentData->setViewSent(true);
        }
        
        
    } // end if bag wasn't empty, and so we sent stuff...

    pthread_mutex_unlock(&::treeLock);
}

double lastPersistVoxels = 0;
void persistVoxelsWhenDirty() {
    double now = usecTimestampNow();
    double sinceLastTime = (now - ::lastPersistVoxels) / 1000.0;

    // check the dirty bit and persist here...
    if (::wantVoxelPersist && ::randomTree.isDirty() && sinceLastTime > VOXEL_PERSIST_INTERVAL) {

        {
            PerformanceWarning warn(::shouldShowAnimationDebug, 
                                    "persistVoxelsWhenDirty() - reaverageVoxelColors()", ::shouldShowAnimationDebug);

            // after done inserting all these voxels, then reaverage colors
            randomTree.reaverageVoxelColors(randomTree.rootNode);
        }


        {
            PerformanceWarning warn(::shouldShowAnimationDebug, 
                                    "persistVoxelsWhenDirty() - writeToFileV2()", ::shouldShowAnimationDebug);

            printf("saving voxels to file...\n");
            randomTree.writeToFileV2(::wantLocalDomain ? LOCAL_VOXELS_PERSIST_FILE : VOXELS_PERSIST_FILE);
            randomTree.clearDirtyBit(); // tree is clean after saving
            printf("DONE saving voxels to file...\n");
        }
        ::lastPersistVoxels = usecTimestampNow();
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
                bool viewFrustumChanged = agentData->updateCurrentViewFrustum();
                if (::debugVoxelSending) {
                    printf("agentData->updateCurrentViewFrustum() changed=%s\n", debug::valueOf(viewFrustumChanged));
                }

                if (agentData->getWantResIn()) { 
                    resInVoxelDistributor(agentList, agent, agentData);
                } else {
                    deepestLevelVoxelDistributor(agentList, agent, agentData, viewFrustumChanged);
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

int main(int argc, const char * argv[]) {

    pthread_mutex_init(&::treeLock, NULL);

    AgentList* agentList = AgentList::createInstance(AGENT_TYPE_VOXEL, VOXEL_LISTEN_PORT);
    setvbuf(stdout, NULL, _IOLBF, 0);

    // Handle Local Domain testing with the --local command line
    const char* local = "--local";
    ::wantLocalDomain = cmdOptionExists(argc, argv,local);
    if (::wantLocalDomain) {
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
    printf("debugVoxelSending=%s\n", debug::valueOf(::debugVoxelSending));

    const char* WANT_ANIMATION_DEBUG = "--shouldShowAnimationDebug";
    ::shouldShowAnimationDebug = cmdOptionExists(argc, argv, WANT_ANIMATION_DEBUG);
    printf("shouldShowAnimationDebug=%s\n", debug::valueOf(::shouldShowAnimationDebug));

    const char* WANT_COLOR_RANDOMIZER = "--wantColorRandomizer";
    ::wantColorRandomizer = cmdOptionExists(argc, argv, WANT_COLOR_RANDOMIZER);
    printf("wantColorRandomizer=%s\n", debug::valueOf(::wantColorRandomizer));

    const char* WANT_SEARCH_FOR_NODES = "--wantSearchForColoredNodes";
    ::wantSearchForColoredNodes = cmdOptionExists(argc, argv, WANT_SEARCH_FOR_NODES);
    printf("wantSearchForColoredNodes=%s\n", debug::valueOf(::wantSearchForColoredNodes));

    // By default we will voxel persist, if you want to disable this, then pass in this parameter
    const char* NO_VOXEL_PERSIST = "--NoVoxelPersist";
    if (cmdOptionExists(argc, argv, NO_VOXEL_PERSIST)) {
        ::wantVoxelPersist = false;
    }
    printf("wantVoxelPersist=%s\n", debug::valueOf(::wantVoxelPersist));

    // if we want Voxel Persistance, load the local file now...
    bool persistantFileRead = false;
    if (::wantVoxelPersist) {
        printf("loading voxels from file...\n");
        persistantFileRead = ::randomTree.readFromFileV2(::wantLocalDomain ? LOCAL_VOXELS_PERSIST_FILE : VOXELS_PERSIST_FILE);
        ::randomTree.clearDirtyBit(); // the tree is clean since we just loaded it
        printf("DONE loading voxels from file... fileRead=%s\n", debug::valueOf(persistantFileRead));
        unsigned long nodeCount = ::randomTree.getVoxelCount();
        printf("Nodes after loading scene %ld nodes\n", nodeCount);
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

    const char* ADD_SCENE = "--AddScene";
    bool addScene = cmdOptionExists(argc, argv, ADD_SCENE);
    const char* NO_ADD_SCENE = "--NoAddScene";
    bool noAddScene = cmdOptionExists(argc, argv, NO_ADD_SCENE);
    if (addScene && noAddScene) {
        printf("WARNING! --AddScene and --NoAddScene are mutually exclusive. We will honor --NoAddScene\n");
    }

    // We will add a scene if...
    //      1) we attempted to load a persistant file and it wasn't there
    //      2) you asked us to add a scene
    // HOWEVER -- we will NEVER add a scene if you explicitly tell us not to!
    //
    // TEMPORARILY DISABLED!!!
    bool actuallyAddScene = false; // !noAddScene && (addScene || (::wantVoxelPersist && !persistantFileRead));
    if (actuallyAddScene) {
        addSphereScene(&randomTree);
    }
    
    // for now, initialize the environments with fixed values
    environmentData[1].setID(1);
    environmentData[1].setGravity(1.0f);
    environmentData[1].setAtmosphereCenter(glm::vec3(0.5, 0.5, (0.25 - 0.06125)) * (float)TREE_SCALE);
    environmentData[1].setAtmosphereInnerRadius(0.030625f * TREE_SCALE);
    environmentData[1].setAtmosphereOuterRadius(0.030625f * TREE_SCALE * 1.025f);
    environmentData[2].setID(2);
    environmentData[2].setGravity(1.0f);
    environmentData[2].setAtmosphereCenter(glm::vec3(0.5f, 0.5f, 0.5f) * (float)TREE_SCALE);
    environmentData[2].setAtmosphereInnerRadius(0.1875f * TREE_SCALE);
    environmentData[2].setAtmosphereOuterRadius(0.1875f * TREE_SCALE * 1.025f);
    
    pthread_t sendVoxelThread;
    pthread_create(&sendVoxelThread, NULL, distributeVoxelsToListeners, NULL);

    sockaddr agentPublicAddress;
    
    unsigned char *packetData = new unsigned char[MAX_PACKET_SIZE];
    ssize_t receivedBytes;

    // loop to send to agents requesting data
    while (true) {
    
        // check to see if we need to persist our voxel state
        persistVoxelsWhenDirty();
    
        if (agentList->getAgentSocket()->receive(&agentPublicAddress, packetData, &receivedBytes)) {
            if (packetData[0] == PACKET_HEADER_SET_VOXEL || packetData[0] == PACKET_HEADER_SET_VOXEL_DESTRUCTIVE) {
                bool destructive = (packetData[0] == PACKET_HEADER_SET_VOXEL_DESTRUCTIVE);
                PerformanceWarning warn(::shouldShowAnimationDebug,
                                        destructive ? "PACKET_HEADER_SET_VOXEL_DESTRUCTIVE" : "PACKET_HEADER_SET_VOXEL",
                                        ::shouldShowAnimationDebug);
                unsigned short int itemNumber = (*((unsigned short int*)&packetData[1]));
                if (::shouldShowAnimationDebug) {
                    printf("got %s - command from client receivedBytes=%ld itemNumber=%d\n",
                        destructive ? "PACKET_HEADER_SET_VOXEL_DESTRUCTIVE" : "PACKET_HEADER_SET_VOXEL",
                        receivedBytes,itemNumber);
                }
                int atByte = sizeof(PACKET_HEADER) + sizeof(itemNumber);
                unsigned char* voxelData = (unsigned char*)&packetData[atByte];
                while (atByte < receivedBytes) {
                    unsigned char octets = (unsigned char)*voxelData;
                    const int COLOR_SIZE_IN_BYTES = 3;
                    int voxelDataSize = bytesRequiredForCodeLength(octets) + COLOR_SIZE_IN_BYTES;
                    int voxelCodeSize = bytesRequiredForCodeLength(octets);

                    // color randomization on insert
                    int colorRandomizer = ::wantColorRandomizer ? randIntInRange (-50, 50) : 0;
                    int red   = voxelData[voxelCodeSize + 0];
                    int green = voxelData[voxelCodeSize + 1];
                    int blue  = voxelData[voxelCodeSize + 2];

                    if (::shouldShowAnimationDebug) {
                        printf("insert voxels - wantColorRandomizer=%s old r=%d,g=%d,b=%d \n",
                            (::wantColorRandomizer?"yes":"no"),red,green,blue);
                    }
                
                    red   = std::max(0, std::min(255, red   + colorRandomizer));
                    green = std::max(0, std::min(255, green + colorRandomizer));
                    blue  = std::max(0, std::min(255, blue  + colorRandomizer));

                    if (::shouldShowAnimationDebug) {
                        printf("insert voxels - wantColorRandomizer=%s NEW r=%d,g=%d,b=%d \n",
                            (::wantColorRandomizer?"yes":"no"),red,green,blue);
                    }
                    voxelData[voxelCodeSize + 0] = red;
                    voxelData[voxelCodeSize + 1] = green;
                    voxelData[voxelCodeSize + 2] = blue;

                    if (::shouldShowAnimationDebug) {
                        float* vertices = firstVertexForCode(voxelData);
                        printf("inserting voxel at: %f,%f,%f\n", vertices[0], vertices[1], vertices[2]);
                        delete []vertices;
                    }
                
                    randomTree.readCodeColorBufferToTree(voxelData, destructive);
                    // skip to next
                    voxelData += voxelDataSize;
                    atByte += voxelDataSize;
                }
            }
            if (packetData[0] == PACKET_HEADER_ERASE_VOXEL) {

                // Send these bits off to the VoxelTree class to process them
                pthread_mutex_lock(&::treeLock);
                randomTree.processRemoveVoxelBitstream((unsigned char*)packetData, receivedBytes);
                pthread_mutex_unlock(&::treeLock);
            }
            if (packetData[0] == PACKET_HEADER_Z_COMMAND) {

                // the Z command is a special command that allows the sender to send the voxel server high level semantic
                // requests, like erase all, or add sphere scene
                char* command = (char*) &packetData[1]; // start of the command
                int commandLength = strlen(command); // commands are null terminated strings
                int totalLength = sizeof(PACKET_HEADER_Z_COMMAND) + commandLength + 1; // 1 for null termination
                printf("got Z message len(%ld)= %s\n", receivedBytes, command);
                bool rebroadcast = true; // by default rebroadcast

                while (totalLength <= receivedBytes) {
                    if (strcmp(command, ERASE_ALL_COMMAND) == 0) {
                        printf("got Z message == erase all\n");
                        eraseVoxelTreeAndCleanupAgentVisitData();
                        rebroadcast = false;
                    }
                    if (strcmp(command, ADD_SCENE_COMMAND) == 0) {
                        printf("got Z message == add scene\n");
                        addSphereScene(&randomTree);
                        rebroadcast = false;
                    }
                    if (strcmp(command, TEST_COMMAND) == 0) {
                        printf("got Z message == a message, nothing to do, just report\n");
                    }
                    totalLength += commandLength + 1; // 1 for null termination
                }

                if (rebroadcast) {
                    // Now send this to the connected agents so they can also process these messages
                    printf("rebroadcasting Z message to connected agents... agentList.broadcastToAgents()\n");
                    agentList->broadcastToAgents(packetData, receivedBytes, &AGENT_TYPE_AVATAR, 1);
                }
            }
            // If we got a PACKET_HEADER_HEAD_DATA, then we're talking to an AGENT_TYPE_AVATAR, and we
            // need to make sure we have it in our agentList.
            if (packetData[0] == PACKET_HEADER_HEAD_DATA) {
                uint16_t agentID = 0;
                unpackAgentId(packetData + sizeof(PACKET_HEADER_HEAD_DATA), &agentID);
                Agent* agent = agentList->addOrUpdateAgent(&agentPublicAddress,
                                                           &agentPublicAddress,
                                                           AGENT_TYPE_AVATAR,
                                                           agentID);
                
                agentList->updateAgentWithData(agent, packetData, receivedBytes);
            }
        }
    }
    
    pthread_join(sendVoxelThread, NULL);
    pthread_mutex_destroy(&::treeLock);

    return 0;
}
