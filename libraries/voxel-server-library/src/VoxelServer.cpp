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
#include <time.h>

#include <QtCore/QDebug>
#include <QtCore/QString>
#include <QtCore/QUuid>

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
#include <UUID.h>

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
        VoxelNodeData* voxelNodeData = new VoxelNodeData(newNode);
        QUuid nodeUUID = newNode->getUUID();
        qDebug("attachVoxelNodeDataToNode() newNode=%p voxelNodeData=%p\n", newNode, voxelNodeData);
        qDebug() << "attachVoxelNodeDataToNode() node UUID:" << nodeUUID << "\n";
        newNode->setLinkedData(voxelNodeData);
    }
}

VoxelServer* VoxelServer::_theInstance = NULL;

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
    
    _started = time(0);
    _startedUSecs = usecTimestampNow();
    
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

#ifdef FORCE_CRASH
    if (strcmp(ri->uri, "/force_crash") == 0 && strcmp(ri->request_method, "GET") == 0) {
        qDebug() << "About to force a crash!\n";
        int foo;
        int* forceCrash = &foo;
        mg_printf(connection, "%s", "HTTP/1.0 200 OK\r\n\r\n");
        mg_printf(connection, "%s", "forcing a crash....\r\n");
        delete[] forceCrash;
        mg_printf(connection, "%s", "did it crash....\r\n");
        return 1;
    }
#endif
    
    if (strcmp(ri->uri, "/") == 0 && strcmp(ri->request_method, "GET") == 0) {
        uint64_t checkSum;
        // return a 200
        mg_printf(connection, "%s", "HTTP/1.0 200 OK\r\n\r\n");
        mg_printf(connection, "%s", "Your Voxel Server is running.\r\n");


        mg_printf(connection, "%s", "\r\n");
        tm* localtm = localtime(&GetInstance()->_started);
        const int MAX_TIME_LENGTH = 128;
        char buffer[MAX_TIME_LENGTH];
        strftime(buffer, MAX_TIME_LENGTH, "%m/%d/%Y %X", localtm);
        mg_printf(connection, "Running since: %s", buffer);

        // Convert now to tm struct for UTC
        tm* gmtm = gmtime(&GetInstance()->_started);
        if (gmtm != NULL) {
            strftime(buffer, MAX_TIME_LENGTH, "%m/%d/%Y %X", gmtm);
            mg_printf(connection, " [%s UTM] ", buffer);
        }
        mg_printf(connection, "%s", "\r\n");

        uint64_t now  = usecTimestampNow();
        const int USECS_PER_MSEC = 1000;
        uint64_t msecsElapsed = (now - GetInstance()->_startedUSecs) / USECS_PER_MSEC;
        const int MSECS_PER_SEC = 1000;
        const int SECS_PER_MIN = 60;
        const int MIN_PER_HOUR = 60;
        const int MSECS_PER_MIN = MSECS_PER_SEC * SECS_PER_MIN;

        float seconds = (msecsElapsed % MSECS_PER_MIN)/(float)MSECS_PER_SEC;
        int minutes = (msecsElapsed/(MSECS_PER_MIN)) % MIN_PER_HOUR;
        int hours = (msecsElapsed/(MSECS_PER_MIN * MIN_PER_HOUR));

        mg_printf(connection, "%s", "Uptime: ");
        if (hours > 0) {
            mg_printf(connection, "%d hour%s ", hours, (hours > 1) ? "s" : "" );
        }
        if (minutes > 0) {
            mg_printf(connection, "%d minute%s ", minutes, (minutes > 1) ? "s" : "");
        }
        if (seconds > 0) {
            mg_printf(connection, "%.3f seconds ", seconds);
        }
        mg_printf(connection, "%s", "\r\n");
        mg_printf(connection, "%s", "\r\n");

        mg_printf(connection, "%s", "\r\n");
        mg_printf(connection, "%s", "Configuration: \r\n     ");
        for (int i = 1; i < GetInstance()->_argc; i++) {
            mg_printf(connection, "%s ", GetInstance()->_argv[i]);
        }
        mg_printf(connection, "%s", "\r\n");
        mg_printf(connection, "%s", "\r\n");


        mg_printf(connection, "%s", "Current Statistics\r\n");
        mg_printf(connection, "%s", "\r\n");

        const char* memoryScaleLabel;
        const float MEGABYTES = 1000000.f;
        const float GIGABYTES = 1000000000.f;
        float memoryScale;
        if (VoxelNode::getTotalMemoryUsage() / MEGABYTES < 1000.0f) {
            memoryScaleLabel = "MB";
            memoryScale = MEGABYTES;
        } else {
            memoryScaleLabel = "GB";
            memoryScale = GIGABYTES;
        }

        mg_printf(connection, "Voxel Node Memory Usage:         %8.2f %s\r\n", 
            VoxelNode::getVoxelMemoryUsage() / memoryScale, memoryScaleLabel);
        mg_printf(connection, "Octcode Memory Usage:            %8.2f %s\r\n", 
            VoxelNode::getOctcodeMemoryUsage() / memoryScale, memoryScaleLabel);
        mg_printf(connection, "External Children Memory Usage:  %8.2f %s\r\n", 
            VoxelNode::getExternalChildrenMemoryUsage() / memoryScale, memoryScaleLabel);
        mg_printf(connection, "%s", "                                 -----------\r\n");
        mg_printf(connection, "                         Total:  %8.2f %s\r\n", 
            VoxelNode::getTotalMemoryUsage() / memoryScale, memoryScaleLabel);

        mg_printf(connection, "\r\nVoxelNode size... %ld bytes\r\n", sizeof(VoxelNode));

        unsigned long nodeCount = VoxelNode::getNodeCount();
        unsigned long internalNodeCount = VoxelNode::getInternalNodeCount();
        unsigned long leafNodeCount = VoxelNode::getLeafNodeCount();
        
        
        QLocale locale(QLocale::English);

        const float AS_PERCENT = 100.0;

        mg_printf(connection, "%s", "\r\n");
        mg_printf(connection, "%s", "Current Nodes in scene\r\n");
        mg_printf(connection, "       Total Nodes: %s nodes\r\n",
                    locale.toString((uint)nodeCount).rightJustified(16, ' ').toLocal8Bit().constData());
        mg_printf(connection, "    Internal Nodes: %s nodes (%5.2f%%)\r\n",
            locale.toString((uint)internalNodeCount).rightJustified(16, ' ').toLocal8Bit().constData(),
            ((float)internalNodeCount/(float)nodeCount) * AS_PERCENT);
        mg_printf(connection, "        Leaf Nodes: %s nodes (%5.2f%%)\r\n", 
            locale.toString((uint)leafNodeCount).rightJustified(16, ' ').toLocal8Bit().constData(),
            ((float)leafNodeCount/(float)nodeCount) * AS_PERCENT);

        mg_printf(connection, "%s", "\r\n");
        mg_printf(connection, "%s", "VoxelNode Children Population Statistics...\r\n");
        checkSum = 0;
        for (int i=0; i <= NUMBER_OF_CHILDREN; i++) {
            checkSum += VoxelNode::getChildrenCount(i);
            mg_printf(connection, "    Nodes with %d children:      %s nodes (%5.2f%%)\r\n", i, 
                locale.toString((uint)VoxelNode::getChildrenCount(i)).rightJustified(16, ' ').toLocal8Bit().constData(),
                ((float)VoxelNode::getChildrenCount(i)/(float)nodeCount) * AS_PERCENT);
        }
        mg_printf(connection, "%s", "                                ----------------------\r\n");
        mg_printf(connection, "                    Total:      %s nodes\r\n", 
            locale.toString((uint)checkSum).rightJustified(16, ' ').toLocal8Bit().constData());

#ifdef BLENDED_UNION_CHILDREN
        mg_printf(connection, "%s", "\r\n");
        mg_printf(connection, "%s", "VoxelNode Children Encoding Statistics...\r\n");
        
        mg_printf(connection, "    Single or No Children:      %10.llu nodes (%5.2f%%)\r\n",
            VoxelNode::getSingleChildrenCount(), ((float)VoxelNode::getSingleChildrenCount()/(float)nodeCount) * AS_PERCENT);
        mg_printf(connection, "    Two Children as Offset:     %10.llu nodes (%5.2f%%)\r\n", 
            VoxelNode::getTwoChildrenOffsetCount(), 
            ((float)VoxelNode::getTwoChildrenOffsetCount()/(float)nodeCount) * AS_PERCENT);
        mg_printf(connection, "    Two Children as External:   %10.llu nodes (%5.2f%%)\r\n", 
            VoxelNode::getTwoChildrenExternalCount(), 
            ((float)VoxelNode::getTwoChildrenExternalCount()/(float)nodeCount) * AS_PERCENT);
        mg_printf(connection, "    Three Children as Offset:   %10.llu nodes (%5.2f%%)\r\n", 
            VoxelNode::getThreeChildrenOffsetCount(), 
            ((float)VoxelNode::getThreeChildrenOffsetCount()/(float)nodeCount) * AS_PERCENT);
        mg_printf(connection, "    Three Children as External: %10.llu nodes (%5.2f%%)\r\n", 
            VoxelNode::getThreeChildrenExternalCount(), 
            ((float)VoxelNode::getThreeChildrenExternalCount()/(float)nodeCount) * AS_PERCENT);
        mg_printf(connection, "    Children as External Array: %10.llu nodes (%5.2f%%)\r\n",
            VoxelNode::getExternalChildrenCount(), 
            ((float)VoxelNode::getExternalChildrenCount()/(float)nodeCount) * AS_PERCENT);

        checkSum = VoxelNode::getSingleChildrenCount() +
                            VoxelNode::getTwoChildrenOffsetCount() + VoxelNode::getTwoChildrenExternalCount() + 
                            VoxelNode::getThreeChildrenOffsetCount() + VoxelNode::getThreeChildrenExternalCount() + 
                            VoxelNode::getExternalChildrenCount();

        mg_printf(connection, "%s", "                                ----------------\r\n");
        mg_printf(connection, "                         Total: %10.llu nodes\r\n", checkSum);
        mg_printf(connection, "                      Expected: %10.lu nodes\r\n", nodeCount);

        mg_printf(connection, "%s", "\r\n");
        mg_printf(connection, "%s", "In other news....\r\n");
        mg_printf(connection, "could store 4 children internally:     %10.llu nodes\r\n",
            VoxelNode::getCouldStoreFourChildrenInternally());
        mg_printf(connection, "could NOT store 4 children internally: %10.llu nodes\r\n", 
            VoxelNode::getCouldNotStoreFourChildrenInternally());
#endif

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
    const char* SEND_ENVIRONMENTS = "--sendEnvironments";
    bool dontSendEnvironments =  !getCmdOption(_argc, _argv, SEND_ENVIRONMENTS);
    if (dontSendEnvironments) {
        qDebug("Sending environments suppressed...\n");
        _sendEnvironments = false;
    } else { 
        // should we send environments? Default is yes, but this command line suppresses sending
        const char* MINIMAL_ENVIRONMENT = "--minimalEnvironment";
        _sendMinimalEnvironment =  getCmdOption(_argc, _argv, MINIMAL_ENVIRONMENT);
        qDebug("Using Minimal Environment=%s\n", debug::valueOf(_sendMinimalEnvironment));
    }
    qDebug("Sending environments=%s\n", debug::valueOf(_sendEnvironments));
    
    NodeList* nodeList = NodeList::getInstance();
    nodeList->setOwnerType(NODE_TYPE_VOXEL_SERVER);
    
    // we need to ask the DS about agents so we can ping/reply with them
    nodeList->setNodeTypesOfInterest(&NODE_TYPE_AGENT, 1);
    
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

    // Convert now to tm struct for local timezone
    tm* localtm = localtime(&_started);
    const int MAX_TIME_LENGTH = 128;
    char localBuffer[MAX_TIME_LENGTH] = { 0 };
    char utcBuffer[MAX_TIME_LENGTH] = { 0 };
    strftime(localBuffer, MAX_TIME_LENGTH, "%m/%d/%Y %X", localtm);
    // Convert now to tm struct for UTC
    tm* gmtm = gmtime(&_started);
    if (gmtm != NULL) {
        strftime(utcBuffer, MAX_TIME_LENGTH, " [%m/%d/%Y %X UTC]", gmtm);
    }
    qDebug() << "Now running... started at: " << localBuffer << utcBuffer << "\n";
    
    
    // loop to send to nodes requesting data
    while (true) {
    
        if (NodeList::getInstance()->getNumNoReplyDomainCheckIns() == MAX_SILENT_DOMAIN_SERVER_CHECK_INS) {
            break;
        }
        
        // send a check in packet to the domain server if DOMAIN_SERVER_CHECK_IN_USECS has elapsed
        if (usecTimestampNow() - usecTimestamp(&lastDomainServerCheckIn) >= DOMAIN_SERVER_CHECK_IN_USECS) {
            gettimeofday(&lastDomainServerCheckIn, NULL);
            NodeList::getInstance()->sendDomainServerCheckIn();
        }
        
        // ping our inactive nodes to punch holes with them
        nodeList->possiblyPingInactiveNodes();
        
        if (nodeList->getNodeSocket()->receive(&senderAddress, packetData, &packetLength) &&
            packetVersionMatch(packetData)) {

            int numBytesPacketHeader = numBytesForPacketHeader(packetData);

            if (packetData[0] == PACKET_TYPE_VOXEL_QUERY) {
                // If we got a PACKET_TYPE_VOXEL_QUERY, then we're talking to an NODE_TYPE_AVATAR, and we
                // need to make sure we have it in our nodeList.
                QUuid nodeUUID = QUuid::fromRfc4122(QByteArray((char*)packetData + numBytesPacketHeader,
                                                               NUM_BYTES_RFC4122_UUID));
                
                Node* node = nodeList->nodeWithUUID(nodeUUID);
                
                if (node) {
                    nodeList->updateNodeWithData(node, &senderAddress, packetData, packetLength);
                    
                    if (!node->getActiveSocket()) {
                        // we don't have an active socket for this node, but they're talking to us
                        // this means they've heard from us and can reply, let's assume public is active
                        node->activatePublicSocket();
                    }
                    
                    VoxelNodeData* nodeData = (VoxelNodeData*) node->getLinkedData();
                    if (nodeData && !nodeData->isVoxelSendThreadInitalized()) {
                        nodeData->initializeVoxelSendThread(this);
                    }
                }
            } else if (packetData[0] == PACKET_TYPE_VOXEL_JURISDICTION_REQUEST) {
                if (_jurisdictionSender) {
                    _jurisdictionSender->queueReceivedPacket(senderAddress, packetData, packetLength);
                }
            } else if (_voxelServerPacketProcessor &&
                       (packetData[0] == PACKET_TYPE_SET_VOXEL
                        || packetData[0] == PACKET_TYPE_SET_VOXEL_DESTRUCTIVE
                        || packetData[0] == PACKET_TYPE_ERASE_VOXEL
                        || packetData[0] == PACKET_TYPE_Z_COMMAND)) {
                _voxelServerPacketProcessor->queueReceivedPacket(senderAddress, packetData, packetLength);
            } else {
                // let processNodeData handle it.
                NodeList::getInstance()->processNodeData(&senderAddress, packetData, packetLength);
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
}


