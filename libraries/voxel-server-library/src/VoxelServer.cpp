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
#include <VoxelTree.h>
#include "VoxelNodeData.h"
#include <SharedUtil.h>
#include <PacketHeaders.h>
#include <SceneUtils.h>
#include <PerfStat.h>
#include <JurisdictionSender.h>

#ifdef _WIN32
#include "Syssocket.h"
#include "Systime.h"
#else
#include <sys/time.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#endif

#include "VoxelServer.h"
#include "VoxelServerConsts.h"

const char* LOCAL_VOXELS_PERSIST_FILE = "resources/voxels.svo";
const char* VOXELS_PERSIST_FILE = "/etc/highfidelity/voxel-server/resources/voxels.svo";

void attachVoxelNodeDataToNode(Node* newNode) {
    if (newNode->getLinkedData() == NULL) {
        newNode->setLinkedData(new VoxelNodeData(newNode));
    }
}

VoxelServer::VoxelServer(Assignment::Command command, Assignment::Location location) :
    Assignment(command, Assignment::VoxelServerType, location),
    _serverTree(true) {
    _argc = 0;
    _argv = NULL;
    _dontKillOnMissingDomain = false;

    _PACKETS_PER_CLIENT_PER_INTERVAL = 10;
    _wantVoxelPersist = true;
    _wantLocalDomain = false;
    _debugVoxelSending = false;
    _shouldShowAnimationDebug = false;
    _displayVoxelStats = false;
    _debugVoxelReceiving = false;
    _sendEnvironments = true;
    _sendMinimalEnvironment = false;
    _dumpVoxelsOnMove = false;
    _jurisdiction = NULL;
    _jurisdictionSender = NULL;
    _voxelServerPacketProcessor = NULL;
    _voxelPersistThread = NULL;
    
}

VoxelServer::VoxelServer(const unsigned char* dataBuffer, int numBytes) : Assignment(dataBuffer, numBytes) {
}

void VoxelServer::setArguments(int argc, char** argv) {
    _argc = argc;
    _argv = const_cast<const char**>(argv);
}

void VoxelServer::setupStandAlone(const char* domain, int port) {
    NodeList::createInstance(NODE_TYPE_VOXEL_SERVER, port);

    // Handle Local Domain testing with the --local command line
    const char* local = "--local";
    _wantLocalDomain = strcmp(domain, local) == 0;
    if (_wantLocalDomain) {
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
    pthread_mutex_init(&_treeLock, NULL);
    
    qInstallMessageHandler(sharedMessageHandler);
    
    const char* JURISDICTION_FILE = "--jurisdictionFile";
    const char* jurisdictionFile = getCmdOption(_argc, _argv, JURISDICTION_FILE);
    if (jurisdictionFile) {
        printf("jurisdictionFile=%s\n", jurisdictionFile);

        printf("about to readFromFile().... jurisdictionFile=%s\n", jurisdictionFile);
        _jurisdiction = new JurisdictionMap(jurisdictionFile);
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
            _jurisdiction = new JurisdictionMap(jurisdictionRoot, jurisdictionEndNodes);
        }
    }
    
    // should we send environments? Default is yes, but this command line suppresses sending
    const char* DUMP_VOXELS_ON_MOVE = "--dumpVoxelsOnMove";
    _dumpVoxelsOnMove = cmdOptionExists(_argc, _argv, DUMP_VOXELS_ON_MOVE);
    printf("dumpVoxelsOnMove=%s\n", debug::valueOf(_dumpVoxelsOnMove));
    
    // should we send environments? Default is yes, but this command line suppresses sending
    const char* DONT_SEND_ENVIRONMENTS = "--dontSendEnvironments";
    bool dontSendEnvironments =  getCmdOption(_argc, _argv, DONT_SEND_ENVIRONMENTS);
    if (dontSendEnvironments) {
        printf("Sending environments suppressed...\n");
        _sendEnvironments = false;
    } else { 
        // should we send environments? Default is yes, but this command line suppresses sending
        const char* MINIMAL_ENVIRONMENT = "--MinimalEnvironment";
        _sendMinimalEnvironment =  getCmdOption(_argc, _argv, MINIMAL_ENVIRONMENT);
        printf("Using Minimal Environment=%s\n", debug::valueOf(_sendMinimalEnvironment));
    }
    printf("Sending environments=%s\n", debug::valueOf(_sendEnvironments));
    
    NodeList* nodeList = NodeList::getInstance();
    nodeList->setOwnerType(NODE_TYPE_VOXEL_SERVER);
    
    setvbuf(stdout, NULL, _IOLBF, 0);

    // tell our NodeList about our desire to get notifications
    nodeList->addHook(&_nodeWatcher);
    nodeList->linkedDataCreateCallback = &attachVoxelNodeDataToNode;

    nodeList->startSilentNodeRemovalThread();
    srand((unsigned)time(0));
    
    const char* DISPLAY_VOXEL_STATS = "--displayVoxelStats";
    _displayVoxelStats =  getCmdOption(_argc, _argv, DISPLAY_VOXEL_STATS);
    printf("displayVoxelStats=%s\n", debug::valueOf(_displayVoxelStats));

    const char* DEBUG_VOXEL_SENDING = "--debugVoxelSending";
    _debugVoxelSending =  getCmdOption(_argc, _argv, DEBUG_VOXEL_SENDING);
    printf("debugVoxelSending=%s\n", debug::valueOf(_debugVoxelSending));

    const char* DEBUG_VOXEL_RECEIVING = "--debugVoxelReceiving";
    _debugVoxelReceiving =  getCmdOption(_argc, _argv, DEBUG_VOXEL_RECEIVING);
    printf("debugVoxelReceiving=%s\n", debug::valueOf(_debugVoxelReceiving));

    const char* WANT_ANIMATION_DEBUG = "--shouldShowAnimationDebug";
    _shouldShowAnimationDebug =  getCmdOption(_argc, _argv, WANT_ANIMATION_DEBUG);
    printf("shouldShowAnimationDebug=%s\n", debug::valueOf(_shouldShowAnimationDebug));

    // By default we will voxel persist, if you want to disable this, then pass in this parameter
    const char* NO_VOXEL_PERSIST = "--NoVoxelPersist";
    if ( getCmdOption(_argc, _argv, NO_VOXEL_PERSIST)) {
        _wantVoxelPersist = false;
    }
    printf("wantVoxelPersist=%s\n", debug::valueOf(_wantVoxelPersist));

    // if we want Voxel Persistence, load the local file now...
    bool persistantFileRead = false;
    if (_wantVoxelPersist) {

        // Check to see if the user passed in a command line option for setting packet send rate
        const char* VOXELS_PERSIST_FILENAME = "--voxelsPersistFilename";
        const char* voxelsPersistFilenameParameter = getCmdOption(_argc, _argv, VOXELS_PERSIST_FILENAME);
        if (voxelsPersistFilenameParameter) {
            strcpy(_voxelPersistFilename, voxelsPersistFilenameParameter);
        } else {
            //strcpy(voxelPersistFilename, _wantLocalDomain ? LOCAL_VOXELS_PERSIST_FILE : VOXELS_PERSIST_FILE);
            strcpy(_voxelPersistFilename, LOCAL_VOXELS_PERSIST_FILE);
        }

        printf("loading voxels from file: %s...\n", _voxelPersistFilename);

        persistantFileRead = _serverTree.readFromSVOFile(_voxelPersistFilename);
        if (persistantFileRead) {
            PerformanceWarning warn(_shouldShowAnimationDebug,
                                    "persistVoxelsWhenDirty() - reaverageVoxelColors()", _shouldShowAnimationDebug);
            
            // after done inserting all these voxels, then reaverage colors
            _serverTree.reaverageVoxelColors(_serverTree.rootNode);
            printf("Voxels reAveraged\n");
        }
        
        _serverTree.clearDirtyBit(); // the tree is clean since we just loaded it
        printf("DONE loading voxels from file... fileRead=%s\n", debug::valueOf(persistantFileRead));
        unsigned long nodeCount         = _serverTree.rootNode->getSubTreeNodeCount();
        unsigned long internalNodeCount = _serverTree.rootNode->getSubTreeInternalNodeCount();
        unsigned long leafNodeCount     = _serverTree.rootNode->getSubTreeLeafNodeCount();
        printf("Nodes after loading scene %lu nodes %lu internal %lu leaves\n", nodeCount, internalNodeCount, leafNodeCount);
        
        // now set up VoxelPersistThread
        _voxelPersistThread = new VoxelPersistThread(&_serverTree, _voxelPersistFilename);
        if (_voxelPersistThread) {
            _voxelPersistThread->initialize(true);
        }
    }

    // Check to see if the user passed in a command line option for loading an old style local
    // Voxel File. If so, load it now. This is not the same as a voxel persist file
    const char* INPUT_FILE = "-i";
    const char* voxelsFilename = getCmdOption(_argc, _argv, INPUT_FILE);
    if (voxelsFilename) {
        _serverTree.readFromSVOFile(voxelsFilename);
    }

    // Check to see if the user passed in a command line option for setting packet send rate
    const char* PACKETS_PER_SECOND = "--packetsPerSecond";
    const char* packetsPerSecond = getCmdOption(_argc, _argv, PACKETS_PER_SECOND);
    if (packetsPerSecond) {
        _PACKETS_PER_CLIENT_PER_INTERVAL = atoi(packetsPerSecond)/INTERVALS_PER_SECOND;
        if (_PACKETS_PER_CLIENT_PER_INTERVAL < 1) {
            _PACKETS_PER_CLIENT_PER_INTERVAL = 1;
        }
        printf("packetsPerSecond=%s PACKETS_PER_CLIENT_PER_INTERVAL=%d\n", packetsPerSecond, _PACKETS_PER_CLIENT_PER_INTERVAL);
    }
    
    // for now, initialize the environments with fixed values
    _environmentData[1].setID(1);
    _environmentData[1].setGravity(1.0f);
    _environmentData[1].setAtmosphereCenter(glm::vec3(0.5, 0.5, (0.25 - 0.06125)) * (float)TREE_SCALE);
    _environmentData[1].setAtmosphereInnerRadius(0.030625f * TREE_SCALE);
    _environmentData[1].setAtmosphereOuterRadius(0.030625f * TREE_SCALE * 1.05f);
    _environmentData[2].setID(2);
    _environmentData[2].setGravity(1.0f);
    _environmentData[2].setAtmosphereCenter(glm::vec3(0.5f, 0.5f, 0.5f) * (float)TREE_SCALE);
    _environmentData[2].setAtmosphereInnerRadius(0.1875f * TREE_SCALE);
    _environmentData[2].setAtmosphereOuterRadius(0.1875f * TREE_SCALE * 1.05f);
    _environmentData[2].setScatteringWavelengths(glm::vec3(0.475f, 0.570f, 0.650f)); // swaps red and blue

    sockaddr senderAddress;
    
    unsigned char *packetData = new unsigned char[MAX_PACKET_SIZE];
    ssize_t packetLength;
    
    timeval lastDomainServerCheckIn = {};

    // set up our jurisdiction broadcaster...
    _jurisdictionSender = new JurisdictionSender(_jurisdiction);
    if (_jurisdictionSender) {
        _jurisdictionSender->initialize(true);
    }
    
    // set up our VoxelServerPacketProcessor
    _voxelServerPacketProcessor = new VoxelServerPacketProcessor(this);
    if (_voxelServerPacketProcessor) {
        _voxelServerPacketProcessor->initialize(true);
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
                if (_jurisdictionSender) {
                    _jurisdictionSender->queueReceivedPacket(senderAddress, packetData, packetLength);
                }
            } else if (_voxelServerPacketProcessor) {
                _voxelServerPacketProcessor->queueReceivedPacket(senderAddress, packetData, packetLength);
            } else {
                printf("unknown packet ignored... packetData[0]=%c\n", packetData[0]);
            }
        }
    }
    
    if (_jurisdiction) {
        delete _jurisdiction;
    }
    
    if (_jurisdictionSender) {
        _jurisdictionSender->terminate();
        delete _jurisdictionSender;
    }

    if (_voxelServerPacketProcessor) {
        _voxelServerPacketProcessor->terminate();
        delete _voxelServerPacketProcessor;
    }

    if (_voxelPersistThread) {
        _voxelPersistThread->terminate();
        delete _voxelPersistThread;
    }
    
    // tell our NodeList we're done with notifications
    nodeList->removeHook(&_nodeWatcher);
    
    pthread_mutex_destroy(&_treeLock);
}


