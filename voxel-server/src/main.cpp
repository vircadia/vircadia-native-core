//
//  main.cpp
//  Voxel Server
//
//  Created by Stephen Birarda on 03/06/13.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <OctalCode.h>
#include <NodeList.h>
#include <NodeTypes.h>
#include <EnvironmentData.h>
#include <VoxelTree.h>
#include "VoxelNodeData.h"
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

const char* LOCAL_VOXELS_PERSIST_FILE = "resources/voxels.svo";
const char* VOXELS_PERSIST_FILE = "/etc/highfidelity/voxel-server/resources/voxels.svo";
const int VOXEL_PERSIST_INTERVAL = 1000 * 30; // every 30 seconds

const int VOXEL_LISTEN_PORT = 40106;


const int VOXEL_SIZE_BYTES = 3 + (3 * sizeof(float));
const int VOXELS_PER_PACKET = (MAX_PACKET_SIZE - 1) / VOXEL_SIZE_BYTES;

const int MIN_BRIGHTNESS = 64;
const float DEATH_STAR_RADIUS = 4.0;
const float MAX_CUBE = 0.05f;

const int VOXEL_SEND_INTERVAL_USECS = 17 * 1000; // approximately 60fps
int PACKETS_PER_CLIENT_PER_INTERVAL = 20;
const int SENDING_TIME_TO_SPARE = 5 * 1000; // usec of sending interval to spare for calculating voxels

const int MAX_VOXEL_TREE_DEPTH_LEVELS = 4;

const int ENVIRONMENT_SEND_INTERVAL_USECS = 1000000;

VoxelTree serverTree(true); // this IS a reaveraging tree 
bool wantVoxelPersist = true;
bool wantLocalDomain = false;


bool wantColorRandomizer = false;
bool debugVoxelSending = false;
bool shouldShowAnimationDebug = false;
bool displayVoxelStats = false;

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

void eraseVoxelTreeAndCleanupNodeVisitData() {

    // As our tree to erase all it's voxels
    ::serverTree.eraseAllVoxels();
    // enumerate the nodes clean up their marker nodes
    for (NodeList::iterator node = NodeList::getInstance()->begin(); node != NodeList::getInstance()->end(); node++) {
        VoxelNodeData* nodeData = (VoxelNodeData*) node->getLinkedData();
        if (nodeData) {
            // clean up the node visit data
            nodeData->nodeBag.deleteAll();
        }
    }
}

pthread_mutex_t treeLock;

void handlePacketSend(NodeList* nodeList, 
                      NodeList::iterator& node,
                      VoxelNodeData* nodeData, 
                      int& trueBytesSent, int& truePacketsSent) {
    // If we've got a stats message ready to send, then see if we can piggyback them together
    if (nodeData->stats.isReadyToSend()) {
        // Send the stats message to the client
        unsigned char* statsMessage = nodeData->stats.getStatsMessage();
        int statsMessageLength = nodeData->stats.getStatsMessageLength();

        // If the size of the stats message and the voxel message will fit in a packet, then piggyback them
        if (nodeData->getPacketLength() + statsMessageLength < MAX_PACKET_SIZE) {

            // copy voxel message to back of stats message
            memcpy(statsMessage + statsMessageLength, nodeData->getPacket(), nodeData->getPacketLength());
            statsMessageLength += nodeData->getPacketLength();

            // actually send it
            nodeList->getNodeSocket()->send(node->getActiveSocket(), statsMessage, statsMessageLength);
        } else {
            // not enough room in the packet, send two packets
            nodeList->getNodeSocket()->send(node->getActiveSocket(), statsMessage, statsMessageLength);
            nodeList->getNodeSocket()->send(node->getActiveSocket(),
                                            nodeData->getPacket(), nodeData->getPacketLength());
        }
    } else {
        // just send the voxel packet
        nodeList->getNodeSocket()->send(node->getActiveSocket(),
                                        nodeData->getPacket(), nodeData->getPacketLength());
    }
    // remember to track our stats
    nodeData->stats.packetSent(nodeData->getPacketLength());
    trueBytesSent += nodeData->getPacketLength();
    truePacketsSent++;
    nodeData->resetVoxelPacket();
}


// Version of voxel distributor that sends the deepest LOD level at once
void deepestLevelVoxelDistributor(NodeList* nodeList, 
                                  NodeList::iterator& node,
                                  VoxelNodeData* nodeData,
                                  bool viewFrustumChanged) {


    pthread_mutex_lock(&::treeLock);

    int truePacketsSent = 0;
    int trueBytesSent = 0;

    // FOR NOW... node tells us if it wants to receive only view frustum deltas
    bool wantDelta = viewFrustumChanged && nodeData->getWantDelta();

    // If our packet already has content in it, then we must use the color choice of the waiting packet.    
    // If we're starting a fresh packet, then... 
    //     If we're moving, and the client asked for low res, then we force monochrome, otherwise, use 
    //     the clients requested color state.
    bool wantColor = LOW_RES_MONO && nodeData->getWantLowResMoving() && viewFrustumChanged ? false : nodeData->getWantColor();

    // If we have a packet waiting, and our desired want color, doesn't match the current waiting packets color
    // then let's just send that waiting packet.    
    if (wantColor != nodeData->getCurrentPacketIsColor()) {
    
        if (nodeData->isPacketWaiting()) {
            if (::debugVoxelSending) {
                printf("wantColor=%s --- SENDING PARTIAL PACKET! nodeData->getCurrentPacketIsColor()=%s\n", 
                       debug::valueOf(wantColor), debug::valueOf(nodeData->getCurrentPacketIsColor()));
            }

            handlePacketSend(nodeList, node, nodeData, trueBytesSent, truePacketsSent);

        } else {
            if (::debugVoxelSending) {
                printf("wantColor=%s --- FIXING HEADER! nodeData->getCurrentPacketIsColor()=%s\n", 
                       debug::valueOf(wantColor), debug::valueOf(nodeData->getCurrentPacketIsColor()));
            }
            nodeData->resetVoxelPacket();
        }
    }
    
    if (::debugVoxelSending) {
        printf("wantColor=%s getCurrentPacketIsColor()=%s, viewFrustumChanged=%s, getWantLowResMoving()=%s\n", 
               debug::valueOf(wantColor), debug::valueOf(nodeData->getCurrentPacketIsColor()),
               debug::valueOf(viewFrustumChanged), debug::valueOf(nodeData->getWantLowResMoving()));
    }

    const ViewFrustum* lastViewFrustum =  wantDelta ? &nodeData->getLastKnownViewFrustum() : NULL;

    if (::debugVoxelSending) {
        printf("deepestLevelVoxelDistributor() viewFrustumChanged=%s, nodeBag.isEmpty=%s, viewSent=%s\n",
                debug::valueOf(viewFrustumChanged), debug::valueOf(nodeData->nodeBag.isEmpty()), 
                debug::valueOf(nodeData->getViewSent())
            );
    }
    
    // If the current view frustum has changed OR we have nothing to send, then search against 
    // the current view frustum for things to send.
    if (viewFrustumChanged || nodeData->nodeBag.isEmpty()) {
        uint64_t now = usecTimestampNow();
        if (::debugVoxelSending) {
            printf("(viewFrustumChanged=%s || nodeData->nodeBag.isEmpty() =%s)...\n",
                   debug::valueOf(viewFrustumChanged), debug::valueOf(nodeData->nodeBag.isEmpty()));
            if (nodeData->getLastTimeBagEmpty() > 0) {
                float elapsedSceneSend = (now - nodeData->getLastTimeBagEmpty()) / 1000000.0f;
                if (viewFrustumChanged) {
                    printf("viewFrustumChanged resetting after elapsed time to send scene = %f seconds", elapsedSceneSend);
                } else {
                    printf("elapsed time to send scene = %f seconds", elapsedSceneSend);
                }
                printf(" [occlusionCulling:%s, wantDelta:%s, wantColor:%s ]\n", 
                       debug::valueOf(nodeData->getWantOcclusionCulling()), debug::valueOf(wantDelta), 
                       debug::valueOf(wantColor));
            }
        }
                
        // if our view has changed, we need to reset these things...
        if (viewFrustumChanged) {
            nodeData->nodeBag.deleteAll();
            nodeData->map.erase();
        } 
        
        if (!viewFrustumChanged && !nodeData->getWantDelta()) {
            // only set our last sent time if we weren't resetting due to frustum change
            uint64_t now = usecTimestampNow();
            nodeData->setLastTimeBagEmpty(now);
        }
        
        nodeData->stats.sceneCompleted();
        
        if (::displayVoxelStats) {
            nodeData->stats.printDebugDetails();
        }
        
        // This is the start of "resending" the scene.
        nodeData->nodeBag.insert(serverTree.rootNode);
        
        // start tracking our stats
        bool isFullScene = (!viewFrustumChanged || !nodeData->getWantDelta()) && nodeData->getViewFrustumJustStoppedChanging();
        nodeData->stats.sceneStarted(isFullScene, viewFrustumChanged, ::serverTree.rootNode);
    }

    // If we have something in our nodeBag, then turn them into packets and send them out...
    if (!nodeData->nodeBag.isEmpty()) {
        static unsigned char tempOutputBuffer[MAX_VOXEL_PACKET_SIZE - 1]; // save on allocs by making this static
        int bytesWritten = 0;
        int packetsSentThisInterval = 0;
        uint64_t start = usecTimestampNow();

        bool shouldSendEnvironments = shouldDo(ENVIRONMENT_SEND_INTERVAL_USECS, VOXEL_SEND_INTERVAL_USECS);
        while (packetsSentThisInterval < PACKETS_PER_CLIENT_PER_INTERVAL - (shouldSendEnvironments ? 1 : 0)) {        
            // Check to see if we're taking too long, and if so bail early...
            uint64_t now = usecTimestampNow();
            long elapsedUsec = (now - start);
            long elapsedUsecPerPacket = (truePacketsSent == 0) ? 0 : (elapsedUsec / truePacketsSent);
            long usecRemaining = (VOXEL_SEND_INTERVAL_USECS - elapsedUsec);
            
            if (elapsedUsecPerPacket + SENDING_TIME_TO_SPARE > usecRemaining) {
                if (::debugVoxelSending) {
                    printf("packetLoop() usecRemaining=%ld bailing early took %ld usecs to generate %d bytes in %d packets (%ld usec avg), %d nodes still to send\n",
                            usecRemaining, elapsedUsec, trueBytesSent, truePacketsSent, elapsedUsecPerPacket,
                            nodeData->nodeBag.count());
                }
                break;
            }            
            
            if (!nodeData->nodeBag.isEmpty()) {
                VoxelNode* subTree = nodeData->nodeBag.extract();
                bool wantOcclusionCulling = nodeData->getWantOcclusionCulling();
                CoverageMap* coverageMap = wantOcclusionCulling ? &nodeData->map : IGNORE_COVERAGE_MAP;
                int boundaryLevelAdjust = viewFrustumChanged && nodeData->getWantLowResMoving() 
                                          ? LOW_RES_MOVING_ADJUST : NO_BOUNDARY_ADJUST;

                bool isFullScene = (!viewFrustumChanged || !nodeData->getWantDelta()) && 
                                 nodeData->getViewFrustumJustStoppedChanging();
                
                EncodeBitstreamParams params(INT_MAX, &nodeData->getCurrentViewFrustum(), wantColor, 
                                             WANT_EXISTS_BITS, DONT_CHOP, wantDelta, lastViewFrustum,
                                             wantOcclusionCulling, coverageMap, boundaryLevelAdjust,
                                             nodeData->getLastTimeBagEmpty(),
                                             isFullScene, &nodeData->stats);
                      
                nodeData->stats.encodeStarted();
                bytesWritten = serverTree.encodeTreeBitstream(subTree, &tempOutputBuffer[0], MAX_VOXEL_PACKET_SIZE - 1,
                                                              nodeData->nodeBag, params);
                nodeData->stats.encodeStopped();

                if (nodeData->getAvailable() >= bytesWritten) {
                    nodeData->writeToPacket(&tempOutputBuffer[0], bytesWritten);
                } else {
                    handlePacketSend(nodeList, node, nodeData, trueBytesSent, truePacketsSent);
                    packetsSentThisInterval++;
                    nodeData->resetVoxelPacket();
                    nodeData->writeToPacket(&tempOutputBuffer[0], bytesWritten);
                }
            } else {
                if (nodeData->isPacketWaiting()) {
                    handlePacketSend(nodeList, node, nodeData, trueBytesSent, truePacketsSent);
                    nodeData->resetVoxelPacket();
                }
                packetsSentThisInterval = PACKETS_PER_CLIENT_PER_INTERVAL; // done for now, no nodes left
            }
        }
        // send the environment packet
        if (shouldSendEnvironments) {
            int numBytesPacketHeader = populateTypeAndVersion(tempOutputBuffer, PACKET_TYPE_ENVIRONMENT_DATA);
            int envPacketLength = numBytesPacketHeader;
            
            for (int i = 0; i < sizeof(environmentData) / sizeof(EnvironmentData); i++) {
                envPacketLength += environmentData[i].getBroadcastData(tempOutputBuffer + envPacketLength);
            }
            
            nodeList->getNodeSocket()->send(node->getActiveSocket(), tempOutputBuffer, envPacketLength);
            trueBytesSent += envPacketLength;
            truePacketsSent++;
        }
        
        uint64_t end = usecTimestampNow();
        int elapsedmsec = (end - start)/1000;
        if (elapsedmsec > 100) {
            if (elapsedmsec > 1000) {
                int elapsedsec = (end - start)/1000000;
                printf("WARNING! packetLoop() took %d seconds to generate %d bytes in %d packets %d nodes still to send\n",
                        elapsedsec, trueBytesSent, truePacketsSent, nodeData->nodeBag.count());
            } else {
                printf("WARNING! packetLoop() took %d milliseconds to generate %d bytes in %d packets, %d nodes still to send\n",
                        elapsedmsec, trueBytesSent, truePacketsSent, nodeData->nodeBag.count());
            }
        } else if (::debugVoxelSending) {
            printf("packetLoop() took %d milliseconds to generate %d bytes in %d packets, %d nodes still to send\n",
                    elapsedmsec, trueBytesSent, truePacketsSent, nodeData->nodeBag.count());
        }
        
        // if after sending packets we've emptied our bag, then we want to remember that we've sent all 
        // the voxels from the current view frustum
        if (nodeData->nodeBag.isEmpty()) {
            nodeData->updateLastKnownViewFrustum();
            nodeData->setViewSent(true);
            if (::debugVoxelSending) {
                nodeData->map.printStats();
            }
            nodeData->map.erase(); // It would be nice if we could save this, and only reset it when the view frustum changes
        }
        
    } // end if bag wasn't empty, and so we sent stuff...

    pthread_mutex_unlock(&::treeLock);
}

uint64_t lastPersistVoxels = 0;
void persistVoxelsWhenDirty() {
    uint64_t now = usecTimestampNow();
    if (::lastPersistVoxels == 0) {
        ::lastPersistVoxels = now;
    }
    int sinceLastTime = (now - ::lastPersistVoxels) / 1000;

    // check the dirty bit and persist here...
    if (::wantVoxelPersist && ::serverTree.isDirty() && sinceLastTime > VOXEL_PERSIST_INTERVAL) {
        {
            PerformanceWarning warn(::shouldShowAnimationDebug, 
                                    "persistVoxelsWhenDirty() - writeToSVOFile()", ::shouldShowAnimationDebug);

            printf("saving voxels to file...\n");
            serverTree.writeToSVOFile(::wantLocalDomain ? LOCAL_VOXELS_PERSIST_FILE : VOXELS_PERSIST_FILE);
            serverTree.clearDirtyBit(); // tree is clean after saving
            printf("DONE saving voxels to file...\n");
        }
        ::lastPersistVoxels = usecTimestampNow();
    }
}

void *distributeVoxelsToListeners(void *args) {
    
    NodeList* nodeList = NodeList::getInstance();
    timeval lastSendTime;
    
    while (true) {
        gettimeofday(&lastSendTime, NULL);
        
        // enumerate the nodes to send 3 packets to each
        for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
            VoxelNodeData* nodeData = (VoxelNodeData*) node->getLinkedData();

            // Sometimes the node data has not yet been linked, in which case we can't really do anything
            if (nodeData) {
                bool viewFrustumChanged = nodeData->updateCurrentViewFrustum();
                if (::debugVoxelSending) {
                    printf("nodeData->updateCurrentViewFrustum() changed=%s\n", debug::valueOf(viewFrustumChanged));
                }
                deepestLevelVoxelDistributor(nodeList, node, nodeData, viewFrustumChanged);
            }
        }
        
        // dynamically sleep until we need to fire off the next set of voxels
        int usecToSleep =  VOXEL_SEND_INTERVAL_USECS - (usecTimestampNow() - usecTimestamp(&lastSendTime));
        
        if (usecToSleep > 0) {
            usleep(usecToSleep);
        } else {
            if (::debugVoxelSending) {
                std::cout << "Last send took too much time, not sleeping!\n";
            }
        }
    }
    
    pthread_exit(0);
}

void attachVoxelNodeDataToNode(Node* newNode) {
    if (newNode->getLinkedData() == NULL) {
        newNode->setLinkedData(new VoxelNodeData(newNode));
    }
}

int main(int argc, const char * argv[]) {
    pthread_mutex_init(&::treeLock, NULL);
    
    qInstallMessageHandler(sharedMessageHandler);
    
    NodeList* nodeList = NodeList::createInstance(NODE_TYPE_VOXEL_SERVER, VOXEL_LISTEN_PORT);
    setvbuf(stdout, NULL, _IOLBF, 0);

    // Handle Local Domain testing with the --local command line
    const char* local = "--local";
    ::wantLocalDomain = cmdOptionExists(argc, argv,local);
    if (::wantLocalDomain) {
        printf("Local Domain MODE!\n");
        nodeList->setDomainIPToLocalhost();
    }

    nodeList->linkedDataCreateCallback = &attachVoxelNodeDataToNode;
    nodeList->startSilentNodeRemovalThread();
    
    srand((unsigned)time(0));
    
    const char* DISPLAY_VOXEL_STATS = "--displayVoxelStats";
    ::displayVoxelStats = cmdOptionExists(argc, argv, DISPLAY_VOXEL_STATS);
    printf("displayVoxelStats=%s\n", debug::valueOf(::displayVoxelStats));

    const char* DEBUG_VOXEL_SENDING = "--debugVoxelSending";
    ::debugVoxelSending = cmdOptionExists(argc, argv, DEBUG_VOXEL_SENDING);
    printf("debugVoxelSending=%s\n", debug::valueOf(::debugVoxelSending));

    const char* WANT_ANIMATION_DEBUG = "--shouldShowAnimationDebug";
    ::shouldShowAnimationDebug = cmdOptionExists(argc, argv, WANT_ANIMATION_DEBUG);
    printf("shouldShowAnimationDebug=%s\n", debug::valueOf(::shouldShowAnimationDebug));

    const char* WANT_COLOR_RANDOMIZER = "--wantColorRandomizer";
    ::wantColorRandomizer = cmdOptionExists(argc, argv, WANT_COLOR_RANDOMIZER);
    printf("wantColorRandomizer=%s\n", debug::valueOf(::wantColorRandomizer));

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
        persistantFileRead = ::serverTree.readFromSVOFile(::wantLocalDomain ? LOCAL_VOXELS_PERSIST_FILE : VOXELS_PERSIST_FILE);
        if (persistantFileRead) {
            PerformanceWarning warn(::shouldShowAnimationDebug,
                                    "persistVoxelsWhenDirty() - reaverageVoxelColors()", ::shouldShowAnimationDebug);
            
            // after done inserting all these voxels, then reaverage colors
            serverTree.reaverageVoxelColors(serverTree.rootNode);
            printf("Voxels reAveraged\n");
        }
        
        ::serverTree.clearDirtyBit(); // the tree is clean since we just loaded it
        printf("DONE loading voxels from file... fileRead=%s\n", debug::valueOf(persistantFileRead));
        unsigned long nodeCount         = ::serverTree.rootNode->getSubTreeNodeCount();
        unsigned long internalNodeCount = ::serverTree.rootNode->getSubTreeInternalNodeCount();
        unsigned long leafNodeCount     = ::serverTree.rootNode->getSubTreeLeafNodeCount();
        printf("Nodes after loading scene %lu nodes %lu internal %lu leaves\n", nodeCount, internalNodeCount, leafNodeCount);
    }

    // Check to see if the user passed in a command line option for loading an old style local
    // Voxel File. If so, load it now. This is not the same as a voxel persist file
    const char* INPUT_FILE = "-i";
    const char* voxelsFilename = getCmdOption(argc, argv, INPUT_FILE);
    if (voxelsFilename) {
        serverTree.readFromSVOFile(voxelsFilename);
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
        randomlyFillVoxelTree(MAX_VOXEL_TREE_DEPTH_LEVELS, serverTree.rootNode);
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
        addSphereScene(&serverTree);
    }
    
    // for now, initialize the environments with fixed values
    environmentData[1].setID(1);
    environmentData[1].setGravity(1.0f);
    environmentData[1].setAtmosphereCenter(glm::vec3(0.5, 0.5, (0.25 - 0.06125)) * (float)TREE_SCALE);
    environmentData[1].setAtmosphereInnerRadius(0.030625f * TREE_SCALE);
    environmentData[1].setAtmosphereOuterRadius(0.030625f * TREE_SCALE * 1.05f);
    environmentData[2].setID(2);
    environmentData[2].setGravity(1.0f);
    environmentData[2].setAtmosphereCenter(glm::vec3(0.5f, 0.5f, 0.5f) * (float)TREE_SCALE);
    environmentData[2].setAtmosphereInnerRadius(0.1875f * TREE_SCALE);
    environmentData[2].setAtmosphereOuterRadius(0.1875f * TREE_SCALE * 1.05f);
    environmentData[2].setScatteringWavelengths(glm::vec3(0.475f, 0.570f, 0.650f)); // swaps red and blue
    
    pthread_t sendVoxelThread;
    pthread_create(&sendVoxelThread, NULL, distributeVoxelsToListeners, NULL);

    sockaddr nodePublicAddress;
    
    unsigned char *packetData = new unsigned char[MAX_PACKET_SIZE];
    ssize_t receivedBytes;
    
    timeval lastDomainServerCheckIn = {};

    // loop to send to nodes requesting data
    
    while (true) {

        // send a check in packet to the domain server if DOMAIN_SERVER_CHECK_IN_USECS has elapsed
        if (usecTimestampNow() - usecTimestamp(&lastDomainServerCheckIn) >= DOMAIN_SERVER_CHECK_IN_USECS) {
            gettimeofday(&lastDomainServerCheckIn, NULL);
            NodeList::getInstance()->sendDomainServerCheckIn();
        }
        
        // check to see if we need to persist our voxel state
        persistVoxelsWhenDirty();
    
        if (nodeList->getNodeSocket()->receive(&nodePublicAddress, packetData, &receivedBytes) &&
            packetVersionMatch(packetData)) {
            
            int numBytesPacketHeader = numBytesForPacketHeader(packetData);
            
            if (packetData[0] == PACKET_TYPE_SET_VOXEL || packetData[0] == PACKET_TYPE_SET_VOXEL_DESTRUCTIVE) {
                bool destructive = (packetData[0] == PACKET_TYPE_SET_VOXEL_DESTRUCTIVE);
                PerformanceWarning warn(::shouldShowAnimationDebug,
                                        destructive ? "PACKET_TYPE_SET_VOXEL_DESTRUCTIVE" : "PACKET_TYPE_SET_VOXEL",
                                        ::shouldShowAnimationDebug);
                
                unsigned short int itemNumber = (*((unsigned short int*)(packetData + numBytesPacketHeader)));
                if (::shouldShowAnimationDebug) {
                    printf("got %s - command from client receivedBytes=%ld itemNumber=%d\n",
                        destructive ? "PACKET_TYPE_SET_VOXEL_DESTRUCTIVE" : "PACKET_TYPE_SET_VOXEL",
                        receivedBytes,itemNumber);
                }
                int atByte = numBytesPacketHeader + sizeof(itemNumber);
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
                
                    serverTree.readCodeColorBufferToTree(voxelData, destructive);
                    // skip to next
                    voxelData += voxelDataSize;
                    atByte += voxelDataSize;
                }
            } else if (packetData[0] == PACKET_TYPE_ERASE_VOXEL) {

                // Send these bits off to the VoxelTree class to process them
                pthread_mutex_lock(&::treeLock);
                serverTree.processRemoveVoxelBitstream((unsigned char*)packetData, receivedBytes);
                pthread_mutex_unlock(&::treeLock);
            } else if (packetData[0] == PACKET_TYPE_Z_COMMAND) {

                // the Z command is a special command that allows the sender to send the voxel server high level semantic
                // requests, like erase all, or add sphere scene
                
                char* command = (char*) &packetData[numBytesPacketHeader]; // start of the command
                int commandLength = strlen(command); // commands are null terminated strings
                int totalLength = numBytesPacketHeader + commandLength + 1; // 1 for null termination
                printf("got Z message len(%ld)= %s\n", receivedBytes, command);
                bool rebroadcast = true; // by default rebroadcast

                while (totalLength <= receivedBytes) {
                    if (strcmp(command, ERASE_ALL_COMMAND) == 0) {
                        printf("got Z message == erase all\n");
                        eraseVoxelTreeAndCleanupNodeVisitData();
                        rebroadcast = false;
                    }
                    if (strcmp(command, ADD_SCENE_COMMAND) == 0) {
                        printf("got Z message == add scene\n");
                        addSphereScene(&serverTree);
                        rebroadcast = false;
                    }
                    if (strcmp(command, TEST_COMMAND) == 0) {
                        printf("got Z message == a message, nothing to do, just report\n");
                    }
                    totalLength += commandLength + 1; // 1 for null termination
                }

                if (rebroadcast) {
                    // Now send this to the connected nodes so they can also process these messages
                    printf("rebroadcasting Z message to connected nodes... nodeList.broadcastToNodes()\n");
                    nodeList->broadcastToNodes(packetData, receivedBytes, &NODE_TYPE_AGENT, 1);
                }
            } else if (packetData[0] == PACKET_TYPE_HEAD_DATA) {
                // If we got a PACKET_TYPE_HEAD_DATA, then we're talking to an NODE_TYPE_AVATAR, and we
                // need to make sure we have it in our nodeList.
                
                uint16_t nodeID = 0;
                unpackNodeId(packetData + numBytesPacketHeader, &nodeID);
                Node* node = nodeList->addOrUpdateNode(&nodePublicAddress,
                                                       &nodePublicAddress,
                                                       NODE_TYPE_AGENT,
                                                       nodeID);
                
                nodeList->updateNodeWithData(node, packetData, receivedBytes);
            } else if (packetData[0] == PACKET_TYPE_PING) {
                // If the packet is a ping, let processNodeData handle it.
                nodeList->processNodeData(&nodePublicAddress, packetData, receivedBytes);
            }
        }
    }
    
    pthread_join(sendVoxelThread, NULL);
    pthread_mutex_destroy(&::treeLock);

    return 0;
}


