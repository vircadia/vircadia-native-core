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

#include <QDebug>
#include <QString>

#include <Logging.h>
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

VoxelServer* VoxelServer::_theInstance = NULL;

VoxelServer::VoxelServer(Assignment::Command command, Assignment::Location location) :
    Assignment(command, Assignment::VoxelServerType, location),
    _serverTree(true) {
    _argc = 0;
    _argv = NULL;

    _packetsPerClientPerInterval = 10;
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
    _parsedArgV = NULL;
    
    _theInstance = this;
}


VoxelServer::VoxelServer(const unsigned char* dataBuffer, int numBytes) : Assignment(dataBuffer, numBytes),
    _serverTree(true) {
    _argc = 0;
    _argv = NULL;

    _packetsPerClientPerInterval = 10;
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
    _parsedArgV = NULL;

    _theInstance = this;
}

VoxelServer::~VoxelServer() {
    if (_parsedArgV) {
        for (int i = 0; i < _argc; i++) {
            delete[] _parsedArgV[i];
        }
        delete[] _parsedArgV;
    }
}

void VoxelServer::initMongoose(int port) {
    // setup the mongoose web server
    struct mg_callbacks callbacks = {};

    QString documentRoot = QString("%1/resources/web").arg(QCoreApplication::applicationDirPath());
    QString listenPort = QString("%1").arg(port);
    

    // list of options. Last element must be NULL.
    const char* options[] = {
        "listening_ports", listenPort.toLocal8Bit().constData(), 
        "document_root", documentRoot.toLocal8Bit().constData(), 
        NULL };

    callbacks.begin_request = civetwebRequestHandler;

    // Start the web server.
    mg_start(&callbacks, NULL, options);
}

int VoxelServer::civetwebRequestHandler(struct mg_connection* connection) {
    const struct mg_request_info* ri = mg_get_request_info(connection);
    
    if (strcmp(ri->uri, "/") == 0 && strcmp(ri->request_method, "GET") == 0) {
        // return a 200
        mg_printf(connection, "%s", "HTTP/1.0 200 OK\r\n\r\n");
        mg_printf(connection, "%s", "Your Voxel Server is running.\r\n");
        mg_printf(connection, "%s", "Current Statistics\r\n");
        mg_printf(connection, "Voxel Node Memory Usage: %f MB\r\n", VoxelNode::getVoxelMemoryUsage() / 1000000.f);
        mg_printf(connection, "Octcode Memory Usage: %f MB\r\n", VoxelNode::getOctcodeMemoryUsage() / 1000000.f);

        VoxelTree* theTree = VoxelServer::GetInstance()->getTree();
        unsigned long nodeCount         = theTree->rootNode->getSubTreeNodeCount();
        unsigned long internalNodeCount = theTree->rootNode->getSubTreeInternalNodeCount();
        unsigned long leafNodeCount     = theTree->rootNode->getSubTreeLeafNodeCount();

        mg_printf(connection, "%s", "Current Nodes in scene\r\n");
        mg_printf(connection, "    Total Nodes: %lu nodes\r\n", nodeCount);
        mg_printf(connection, "    Internal Nodes: %lu nodes\r\n", internalNodeCount);
        mg_printf(connection, "    Leaf Nodes: %lu leaves\r\n", leafNodeCount);

        return 1;
    } else {
        // have mongoose process this request from the document_root
        return 0;
    }
}


void VoxelServer::setArguments(int argc, char** argv) {
    _argc = argc;
    _argv = const_cast<const char**>(argv);

    qDebug("VoxelServer::setArguments()\n");
    for (int i = 0; i < _argc; i++) {
        qDebug("_argv[%d]=%s\n", i, _argv[i]);
    }

}

void VoxelServer::parsePayload() {
    
    if (getNumPayloadBytes() > 0) {
        QString config((const char*) _payload);
        
        // Now, parse the config
        QStringList configList = config.split(" ");
        
        int argCount = configList.size() + 1;

        qDebug("VoxelServer::parsePayload()... argCount=%d\n",argCount);

        _parsedArgV = new char*[argCount];
        const char* dummy = "config-from-payload";
        _parsedArgV[0] = new char[strlen(dummy) + sizeof(char)];
        strcpy(_parsedArgV[0], dummy);

        for (int i = 1; i < argCount; i++) {
            QString configItem = configList.at(i-1);
            _parsedArgV[i] = new char[configItem.length() + sizeof(char)];
            strcpy(_parsedArgV[i], configItem.toLocal8Bit().constData());
            qDebug("VoxelServer::parsePayload()... _parsedArgV[%d]=%s\n", i, _parsedArgV[i]);
        }

        setArguments(argCount, _parsedArgV);
    }
}

//int main(int argc, const char * argv[]) {
void VoxelServer::run() {
    
    const char VOXEL_SERVER_LOGGING_TARGET_NAME[] = "voxel-server";
    
    // change the logging target name while this is running
    Logging::setTargetName(VOXEL_SERVER_LOGGING_TARGET_NAME);

    // Now would be a good time to parse our arguments, if we got them as assignment
    if (getNumPayloadBytes() > 0) {
        parsePayload();
    }

    pthread_mutex_init(&_treeLock, NULL);
    
    qInstallMessageHandler(Logging::verboseMessageHandler);
    
    const char* STATUS_PORT = "--statusPort";
    const char* statusPort = getCmdOption(_argc, _argv, STATUS_PORT);
    if (statusPort) {
        int statusPortNumber = atoi(statusPort);
        initMongoose(statusPortNumber);
    }

    
    const char* JURISDICTION_FILE = "--jurisdictionFile";
    const char* jurisdictionFile = getCmdOption(_argc, _argv, JURISDICTION_FILE);
    if (jurisdictionFile) {
        qDebug("jurisdictionFile=%s\n", jurisdictionFile);

        qDebug("about to readFromFile().... jurisdictionFile=%s\n", jurisdictionFile);
        _jurisdiction = new JurisdictionMap(jurisdictionFile);
        qDebug("after readFromFile().... jurisdictionFile=%s\n", jurisdictionFile);
    } else {
        const char* JURISDICTION_ROOT = "--jurisdictionRoot";
        const char* jurisdictionRoot = getCmdOption(_argc, _argv, JURISDICTION_ROOT);
        if (jurisdictionRoot) {
            qDebug("jurisdictionRoot=%s\n", jurisdictionRoot);
        }

        const char* JURISDICTION_ENDNODES = "--jurisdictionEndNodes";
        const char* jurisdictionEndNodes = getCmdOption(_argc, _argv, JURISDICTION_ENDNODES);
        if (jurisdictionEndNodes) {
            qDebug("jurisdictionEndNodes=%s\n", jurisdictionEndNodes);
        }

        if (jurisdictionRoot || jurisdictionEndNodes) {
            _jurisdiction = new JurisdictionMap(jurisdictionRoot, jurisdictionEndNodes);
        }
    }

    // should we send environments? Default is yes, but this command line suppresses sending
    const char* DUMP_VOXELS_ON_MOVE = "--dumpVoxelsOnMove";
    _dumpVoxelsOnMove = cmdOptionExists(_argc, _argv, DUMP_VOXELS_ON_MOVE);
    qDebug("dumpVoxelsOnMove=%s\n", debug::valueOf(_dumpVoxelsOnMove));
    
    // should we send environments? Default is yes, but this command line suppresses sending
    const char* DONT_SEND_ENVIRONMENTS = "--dontSendEnvironments";
    bool dontSendEnvironments =  getCmdOption(_argc, _argv, DONT_SEND_ENVIRONMENTS);
    if (dontSendEnvironments) {
        qDebug("Sending environments suppressed...\n");
        _sendEnvironments = false;
    } else { 
        // should we send environments? Default is yes, but this command line suppresses sending
        const char* MINIMAL_ENVIRONMENT = "--MinimalEnvironment";
        _sendMinimalEnvironment =  getCmdOption(_argc, _argv, MINIMAL_ENVIRONMENT);
        qDebug("Using Minimal Environment=%s\n", debug::valueOf(_sendMinimalEnvironment));
    }
    qDebug("Sending environments=%s\n", debug::valueOf(_sendEnvironments));
    
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
    qDebug("displayVoxelStats=%s\n", debug::valueOf(_displayVoxelStats));

    const char* DEBUG_VOXEL_SENDING = "--debugVoxelSending";
    _debugVoxelSending =  getCmdOption(_argc, _argv, DEBUG_VOXEL_SENDING);
    qDebug("debugVoxelSending=%s\n", debug::valueOf(_debugVoxelSending));

    const char* DEBUG_VOXEL_RECEIVING = "--debugVoxelReceiving";
    _debugVoxelReceiving =  getCmdOption(_argc, _argv, DEBUG_VOXEL_RECEIVING);
    qDebug("debugVoxelReceiving=%s\n", debug::valueOf(_debugVoxelReceiving));

    const char* WANT_ANIMATION_DEBUG = "--shouldShowAnimationDebug";
    _shouldShowAnimationDebug =  getCmdOption(_argc, _argv, WANT_ANIMATION_DEBUG);
    qDebug("shouldShowAnimationDebug=%s\n", debug::valueOf(_shouldShowAnimationDebug));

    // By default we will voxel persist, if you want to disable this, then pass in this parameter
    const char* NO_VOXEL_PERSIST = "--NoVoxelPersist";
    if (getCmdOption(_argc, _argv, NO_VOXEL_PERSIST)) {
        _wantVoxelPersist = false;
    }
    qDebug("wantVoxelPersist=%s\n", debug::valueOf(_wantVoxelPersist));

    // if we want Voxel Persistence, set up the local file and persist thread
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

        qDebug("voxelPersistFilename=%s\n", _voxelPersistFilename);

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
        _packetsPerClientPerInterval = atoi(packetsPerSecond) / INTERVALS_PER_SECOND;
        if (_packetsPerClientPerInterval < 1) {
            _packetsPerClientPerInterval = 1;
        }
        qDebug("packetsPerSecond=%s PACKETS_PER_CLIENT_PER_INTERVAL=%d\n", packetsPerSecond, _packetsPerClientPerInterval);
    }

    sockaddr senderAddress;
    
    unsigned char* packetData = new unsigned char[MAX_PACKET_SIZE];
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

    qDebug("Now running...\n");
    
    // loop to send to nodes requesting data
    while (true) {
    
        if (NodeList::getInstance()->getNumNoReplyDomainCheckIns() == MAX_SILENT_DOMAIN_SERVER_CHECK_INS) {
            break;
        }
        
        // send a check in packet to the domain server if DOMAIN_SERVER_CHECK_IN_USECS has elapsed
        if (usecTimestampNow() - usecTimestamp(&lastDomainServerCheckIn) >= DOMAIN_SERVER_CHECK_IN_USECS) {
            gettimeofday(&lastDomainServerCheckIn, NULL);
            NodeList::getInstance()->sendDomainServerCheckIn(_uuid.toRfc4122().constData());
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
                
                VoxelNodeData* nodeData = (VoxelNodeData*) node->getLinkedData();
                if (nodeData && !nodeData->isVoxelSendThreadInitalized()) {
                    nodeData->initializeVoxelSendThread(this);
                }
                
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
                qDebug("unknown packet ignored... packetData[0]=%c\n", packetData[0]);
            }
        }
    }
    
    delete _jurisdiction;
    
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


