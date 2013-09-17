//
//  VoxelServer.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 9/16/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
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
#include <JurisdictionSender.h>

#include "NodeWatcher.h"
#include "VoxelPersistThread.h"
#include "VoxelSendThread.h"
#include "VoxelServerPacketProcessor.h"

#ifdef _WIN32
#include "Syssocket.h"
#include "Systime.h"
#else
#include <sys/time.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#endif

#include "VoxelServer.h"
#include "VoxelServerState.h"

const char* LOCAL_VOXELS_PERSIST_FILE = "resources/voxels.svo";
const char* VOXELS_PERSIST_FILE = "/etc/highfidelity/voxel-server/resources/voxels.svo";
char voxelPersistFilename[MAX_FILENAME_LENGTH];
int PACKETS_PER_CLIENT_PER_INTERVAL = 10;
VoxelTree serverTree(true); // this IS a reaveraging tree 
bool wantVoxelPersist = true;
bool wantLocalDomain = false;
bool debugVoxelSending = false;
bool shouldShowAnimationDebug = false;
bool displayVoxelStats = false;
bool debugVoxelReceiving = false;
bool sendEnvironments = true;
bool sendMinimalEnvironment = false;
bool dumpVoxelsOnMove = false;
EnvironmentData environmentData[3];
int receivedPacketCount = 0;
JurisdictionMap* jurisdiction = NULL;
JurisdictionSender* jurisdictionSender = NULL;
VoxelServerPacketProcessor* voxelServerPacketProcessor = NULL;
VoxelPersistThread* voxelPersistThread = NULL;
pthread_mutex_t treeLock;
NodeWatcher nodeWatcher; // used to cleanup AGENT data when agents are killed

int VoxelServer::_argc = 0;
const char** VoxelServer::_argv = NULL;
bool VoxelServer::_dontKillOnMissingDomain = false;

void attachVoxelNodeDataToNode(Node* newNode) {
    if (newNode->getLinkedData() == NULL) {
        newNode->setLinkedData(new VoxelNodeData(newNode));
    }
}

VoxelServer::VoxelServer(Assignment::Command command, Assignment::Location location) :
    Assignment(command, Assignment::VoxelServerType, location) {
    
}

VoxelServer::VoxelServer(const unsigned char* dataBuffer, int numBytes) : Assignment(dataBuffer, numBytes) {
    
}

void VoxelServer::setArguments(int argc, char** argv) {
    _argc = argc;
    _argv = const_cast<const char**>(argv);
}

void VoxelServer::setupDomainAndPort(const char* domain, int port) {
    NodeList::createInstance(NODE_TYPE_VOXEL_SERVER, port);

    // Handle Local Domain testing with the --local command line
    const char* local = "--local";
    ::wantLocalDomain = strcmp(domain, local) == 0;
    if (::wantLocalDomain) {
        printf("Local Domain MODE!\n");
        NodeList::getInstance()->setDomainIPToLocalhost();
    } else {
        if (domain) {
            NodeList::getInstance()->setDomainHostname(domain);
        }
    }
    
    // If we're running in standalone mode, we don't want to kill ourselves when we haven't heard from a domain
    _dontKillOnMissingDomain = true;
}

//int main(int argc, const char * argv[]) {
void VoxelServer::run() {
    pthread_mutex_init(&::treeLock, NULL);
    
    qInstallMessageHandler(sharedMessageHandler);
    
    const char* JURISDICTION_FILE = "--jurisdictionFile";
    const char* jurisdictionFile = getCmdOption(_argc, _argv, JURISDICTION_FILE);
    if (jurisdictionFile) {
        printf("jurisdictionFile=%s\n", jurisdictionFile);

        printf("about to readFromFile().... jurisdictionFile=%s\n", jurisdictionFile);
        jurisdiction = new JurisdictionMap(jurisdictionFile);
        printf("after readFromFile().... jurisdictionFile=%s\n", jurisdictionFile);
    } else {
        const char* JURISDICTION_ROOT = "--jurisdictionRoot";
        const char* jurisdictionRoot = getCmdOption(_argc, _argv, JURISDICTION_ROOT);
        if (jurisdictionRoot) {
            printf("jurisdictionRoot=%s\n", jurisdictionRoot);
        }

        const char* JURISDICTION_ENDNODES = "--jurisdictionEndNodes";
        const char* jurisdictionEndNodes = getCmdOption(_argc, _argv, JURISDICTION_ENDNODES);
        if (jurisdictionEndNodes) {
            printf("jurisdictionEndNodes=%s\n", jurisdictionEndNodes);
        }

        if (jurisdictionRoot || jurisdictionEndNodes) {
            ::jurisdiction = new JurisdictionMap(jurisdictionRoot, jurisdictionEndNodes);
        }
    }
    
    // should we send environments? Default is yes, but this command line suppresses sending
    const char* DUMP_VOXELS_ON_MOVE = "--dumpVoxelsOnMove";
    ::dumpVoxelsOnMove = cmdOptionExists(_argc, _argv, DUMP_VOXELS_ON_MOVE);
    printf("dumpVoxelsOnMove=%s\n", debug::valueOf(::dumpVoxelsOnMove));
    
    // should we send environments? Default is yes, but this command line suppresses sending
    const char* DONT_SEND_ENVIRONMENTS = "--dontSendEnvironments";
    bool dontSendEnvironments =  getCmdOption(_argc, _argv, DONT_SEND_ENVIRONMENTS);
    if (dontSendEnvironments) {
        printf("Sending environments suppressed...\n");
        ::sendEnvironments = false;
    } else { 
        // should we send environments? Default is yes, but this command line suppresses sending
        const char* MINIMAL_ENVIRONMENT = "--MinimalEnvironment";
        ::sendMinimalEnvironment =  getCmdOption(_argc, _argv, MINIMAL_ENVIRONMENT);
        printf("Using Minimal Environment=%s\n", debug::valueOf(::sendMinimalEnvironment));
    }
    printf("Sending environments=%s\n", debug::valueOf(::sendEnvironments));
    
    NodeList* nodeList = NodeList::getInstance();
    nodeList->setOwnerType(NODE_TYPE_VOXEL_SERVER);
    
    setvbuf(stdout, NULL, _IOLBF, 0);

    // tell our NodeList about our desire to get notifications
    nodeList->addHook(&nodeWatcher);
    nodeList->linkedDataCreateCallback = &attachVoxelNodeDataToNode;

    nodeList->startSilentNodeRemovalThread();
    srand((unsigned)time(0));
    
    const char* DISPLAY_VOXEL_STATS = "--displayVoxelStats";
    ::displayVoxelStats =  getCmdOption(_argc, _argv, DISPLAY_VOXEL_STATS);
    printf("displayVoxelStats=%s\n", debug::valueOf(::displayVoxelStats));

    const char* DEBUG_VOXEL_SENDING = "--debugVoxelSending";
    ::debugVoxelSending =  getCmdOption(_argc, _argv, DEBUG_VOXEL_SENDING);
    printf("debugVoxelSending=%s\n", debug::valueOf(::debugVoxelSending));

    const char* DEBUG_VOXEL_RECEIVING = "--debugVoxelReceiving";
    ::debugVoxelReceiving =  getCmdOption(_argc, _argv, DEBUG_VOXEL_RECEIVING);
    printf("debugVoxelReceiving=%s\n", debug::valueOf(::debugVoxelReceiving));

    const char* WANT_ANIMATION_DEBUG = "--shouldShowAnimationDebug";
    ::shouldShowAnimationDebug =  getCmdOption(_argc, _argv, WANT_ANIMATION_DEBUG);
    printf("shouldShowAnimationDebug=%s\n", debug::valueOf(::shouldShowAnimationDebug));

    // By default we will voxel persist, if you want to disable this, then pass in this parameter
    const char* NO_VOXEL_PERSIST = "--NoVoxelPersist";
    if ( getCmdOption(_argc, _argv, NO_VOXEL_PERSIST)) {
        ::wantVoxelPersist = false;
    }
    printf("wantVoxelPersist=%s\n", debug::valueOf(::wantVoxelPersist));

    // if we want Voxel Persistence, load the local file now...
    bool persistantFileRead = false;
    if (::wantVoxelPersist) {

        // Check to see if the user passed in a command line option for setting packet send rate
        const char* VOXELS_PERSIST_FILENAME = "--voxelsPersistFilename";
        const char* voxelsPersistFilenameParameter = getCmdOption(_argc, _argv, VOXELS_PERSIST_FILENAME);
        if (voxelsPersistFilenameParameter) {
            strcpy(voxelPersistFilename, voxelsPersistFilenameParameter);
        } else {
            //strcpy(voxelPersistFilename, ::wantLocalDomain ? LOCAL_VOXELS_PERSIST_FILE : VOXELS_PERSIST_FILE);
            strcpy(voxelPersistFilename, LOCAL_VOXELS_PERSIST_FILE);
        }

        printf("loading voxels from file: %s...\n", voxelPersistFilename);

        persistantFileRead = ::serverTree.readFromSVOFile(::voxelPersistFilename);
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
        
        // now set up VoxelPersistThread
        ::voxelPersistThread = new VoxelPersistThread(&::serverTree, ::voxelPersistFilename);
        if (::voxelPersistThread) {
            ::voxelPersistThread->initialize(true);
        }
    }

    // Check to see if the user passed in a command line option for loading an old style local
    // Voxel File. If so, load it now. This is not the same as a voxel persist file
    const char* INPUT_FILE = "-i";
    const char* voxelsFilename = getCmdOption(_argc, _argv, INPUT_FILE);
    if (voxelsFilename) {
        serverTree.readFromSVOFile(voxelsFilename);
    }

    // Check to see if the user passed in a command line option for setting packet send rate
    const char* PACKETS_PER_SECOND = "--packetsPerSecond";
    const char* packetsPerSecond = getCmdOption(_argc, _argv, PACKETS_PER_SECOND);
    if (packetsPerSecond) {
        PACKETS_PER_CLIENT_PER_INTERVAL = atoi(packetsPerSecond)/INTERVALS_PER_SECOND;
        if (PACKETS_PER_CLIENT_PER_INTERVAL < 1) {
            PACKETS_PER_CLIENT_PER_INTERVAL = 1;
        }
        printf("packetsPerSecond=%s PACKETS_PER_CLIENT_PER_INTERVAL=%d\n", packetsPerSecond, PACKETS_PER_CLIENT_PER_INTERVAL);
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

    sockaddr senderAddress;
    
    unsigned char *packetData = new unsigned char[MAX_PACKET_SIZE];
    ssize_t packetLength;
    
    timeval lastDomainServerCheckIn = {};

    // set up our jurisdiction broadcaster...
    ::jurisdictionSender = new JurisdictionSender(::jurisdiction);
    if (::jurisdictionSender) {
        ::jurisdictionSender->initialize(true);
    }
    
    // set up our VoxelServerPacketProcessor
    ::voxelServerPacketProcessor = new VoxelServerPacketProcessor();
    if (::voxelServerPacketProcessor) {
        ::voxelServerPacketProcessor->initialize(true);
    }

    // loop to send to nodes requesting data
    while (true) {
    
        if (!_dontKillOnMissingDomain &&
            NodeList::getInstance()->getNumNoReplyDomainCheckIns() == MAX_SILENT_DOMAIN_SERVER_CHECK_INS) {
            break;
        }
        
        // send a check in packet to the domain server if DOMAIN_SERVER_CHECK_IN_USECS has elapsed
        if (usecTimestampNow() - usecTimestamp(&lastDomainServerCheckIn) >= DOMAIN_SERVER_CHECK_IN_USECS) {
            gettimeofday(&lastDomainServerCheckIn, NULL);
            NodeList::getInstance()->sendDomainServerCheckIn();
        }
        
        if (nodeList->getNodeSocket()->receive(&senderAddress, packetData, &packetLength) &&
            packetVersionMatch(packetData)) {

            int numBytesPacketHeader = numBytesForPacketHeader(packetData);

            if (packetData[0] == PACKET_TYPE_HEAD_DATA) {
                // If we got a PACKET_TYPE_HEAD_DATA, then we're talking to an NODE_TYPE_AVATAR, and we
                // need to make sure we have it in our nodeList.
                uint16_t nodeID = 0;
                unpackNodeId(packetData + numBytesPacketHeader, &nodeID);
                Node* node = NodeList::getInstance()->addOrUpdateNode(&senderAddress,
                                                       &senderAddress,
                                                       NODE_TYPE_AGENT,
                                                       nodeID);

                NodeList::getInstance()->updateNodeWithData(node, packetData, packetLength);
            } else if (packetData[0] == PACKET_TYPE_PING) {
                // If the packet is a ping, let processNodeData handle it.
                NodeList::getInstance()->processNodeData(&senderAddress, packetData, packetLength);
            } else if (packetData[0] == PACKET_TYPE_DOMAIN) {
                NodeList::getInstance()->processNodeData(&senderAddress, packetData, packetLength);
            } else if (packetData[0] == PACKET_TYPE_VOXEL_JURISDICTION_REQUEST) {
                if (::jurisdictionSender) {
                    ::jurisdictionSender->queueReceivedPacket(senderAddress, packetData, packetLength);
                }
            } else if (::voxelServerPacketProcessor) {
                ::voxelServerPacketProcessor->queueReceivedPacket(senderAddress, packetData, packetLength);
            } else {
                printf("unknown packet ignored... packetData[0]=%c\n", packetData[0]);
            }
        }
    }
    
    if (::jurisdiction) {
        delete ::jurisdiction;
    }
    
    if (::jurisdictionSender) {
        ::jurisdictionSender->terminate();
        delete ::jurisdictionSender;
    }

    if (::voxelServerPacketProcessor) {
        ::voxelServerPacketProcessor->terminate();
        delete ::voxelServerPacketProcessor;
    }

    if (::voxelPersistThread) {
        ::voxelPersistThread->terminate();
        delete ::voxelPersistThread;
    }
    
    // tell our NodeList we're done with notifications
    nodeList->removeHook(&nodeWatcher);
    
    pthread_mutex_destroy(&::treeLock);
}


