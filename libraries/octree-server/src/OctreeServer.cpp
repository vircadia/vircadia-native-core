//
//  OctreeServer.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 9/16/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QTimer>
#include <QtCore/QUuid>

#include <Logging.h>
#include <UUID.h>

#include "civetweb.h"

#include "OctreeServer.h"
#include "OctreeServerConsts.h"

void OctreeServer::attachQueryNodeToNode(Node* newNode) {
    if (newNode->getLinkedData() == NULL) {
        if (GetInstance()) {
            OctreeQueryNode* newQueryNodeData = GetInstance()->createOctreeQueryNode(newNode);
            newQueryNodeData->resetOctreePacket(true); // don't bump sequence
            newNode->setLinkedData(newQueryNodeData);
        }
    }
}

void OctreeServer::nodeAdded(Node* node) {
    // do nothing
}

void OctreeServer::nodeKilled(Node* node) {
    // Use this to cleanup our node
    if (node->getType() == NODE_TYPE_AGENT) {
        OctreeQueryNode* nodeData = (OctreeQueryNode*)node->getLinkedData();
        if (nodeData) {
            node->setLinkedData(NULL);
            delete nodeData;
        }
    }
};


OctreeServer* OctreeServer::_theInstance = NULL;

OctreeServer::OctreeServer(const unsigned char* dataBuffer, int numBytes) :
    ThreadedAssignment(dataBuffer, numBytes)
{
    _argc = 0;
    _argv = NULL;
    _tree = NULL;
    _packetsPerClientPerInterval = 10;
    _wantPersist = true;
    _debugSending = false;
    _debugReceiving = false;
    _verboseDebug = false;
    _jurisdiction = NULL;
    _jurisdictionSender = NULL;
    _octreeInboundPacketProcessor = NULL;
    _persistThread = NULL;
    _parsedArgV = NULL;
    
    _started = time(0);
    _startedUSecs = usecTimestampNow();
    
    _theInstance = this;
}

OctreeServer::~OctreeServer() {
    if (_parsedArgV) {
        for (int i = 0; i < _argc; i++) {
            delete[] _parsedArgV[i];
        }
        delete[] _parsedArgV;
    }
    
    if (_jurisdictionSender) {
        _jurisdictionSender->terminate();
        delete _jurisdictionSender;
    }
    
    if (_octreeInboundPacketProcessor) {
        _octreeInboundPacketProcessor->terminate();
        delete _octreeInboundPacketProcessor;
    }
    
    if (_persistThread) {
        _persistThread->terminate();
        delete _persistThread;
    }
    
    // tell our NodeList we're done with notifications
    NodeList::getInstance()->removeHook(this);

    delete _jurisdiction;
    _jurisdiction = NULL;
    
    qDebug() << "OctreeServer::run()... DONE\n";
}

void OctreeServer::initMongoose(int port) {
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

int OctreeServer::civetwebRequestHandler(struct mg_connection* connection) {
    const struct mg_request_info* ri = mg_get_request_info(connection);
    
    OctreeServer* theServer = GetInstance();

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

    bool showStats = false;
    if (strcmp(ri->uri, "/") == 0 && strcmp(ri->request_method, "GET") == 0) {
        showStats = true;
    }

    if (strcmp(ri->uri, "/resetStats") == 0 && strcmp(ri->request_method, "GET") == 0) {
        theServer->_octreeInboundPacketProcessor->resetStats();
        showStats = true;
    }
    
    if (showStats) {
        uint64_t checkSum;
        // return a 200
        mg_printf(connection, "%s", "HTTP/1.0 200 OK\r\n");
        mg_printf(connection, "%s", "Content-Type: text/html\r\n\r\n");
        mg_printf(connection, "%s", "<html><doc>\r\n");
        mg_printf(connection, "%s", "<pre>\r\n");
        mg_printf(connection, "<b>Your %s Server is running... <a href='/'>[RELOAD]</a></b>\r\n", theServer->getMyServerName());

        tm* localtm = localtime(&theServer->_started);
        const int MAX_TIME_LENGTH = 128;
        char buffer[MAX_TIME_LENGTH];
        strftime(buffer, MAX_TIME_LENGTH, "%m/%d/%Y %X", localtm);
        mg_printf(connection, "Running since: %s", buffer);

        // Convert now to tm struct for UTC
        tm* gmtm = gmtime(&theServer->_started);
        if (gmtm != NULL) {
            strftime(buffer, MAX_TIME_LENGTH, "%m/%d/%Y %X", gmtm);
            mg_printf(connection, " [%s UTM] ", buffer);
        }
        mg_printf(connection, "%s", "\r\n");

        uint64_t now  = usecTimestampNow();
        const int USECS_PER_MSEC = 1000;
        uint64_t msecsElapsed = (now - theServer->_startedUSecs) / USECS_PER_MSEC;
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


        // display voxel file load time
        if (theServer->isInitialLoadComplete()) {
            time_t* loadCompleted = theServer->getLoadCompleted();
            if (loadCompleted) {
                tm* voxelsLoadedAtLocal = localtime(loadCompleted);
                const int MAX_TIME_LENGTH = 128;
                char buffer[MAX_TIME_LENGTH];
                strftime(buffer, MAX_TIME_LENGTH, "%m/%d/%Y %X", voxelsLoadedAtLocal);
                mg_printf(connection, "%s File Loaded At: %s", theServer->getMyServerName(), buffer);

                // Convert now to tm struct for UTC
                tm* voxelsLoadedAtUTM = gmtime(theServer->getLoadCompleted());
                if (gmtm != NULL) {
                    strftime(buffer, MAX_TIME_LENGTH, "%m/%d/%Y %X", voxelsLoadedAtUTM);
                    mg_printf(connection, " [%s UTM] ", buffer);
                }
            } else {
                mg_printf(connection, "%s File Persist Disabled...\r\n", theServer->getMyServerName());
            }
            mg_printf(connection, "%s", "\r\n");


            uint64_t msecsElapsed = theServer->getLoadElapsedTime() / USECS_PER_MSEC;;
            float seconds = (msecsElapsed % MSECS_PER_MIN)/(float)MSECS_PER_SEC;
            int minutes = (msecsElapsed/(MSECS_PER_MIN)) % MIN_PER_HOUR;
            int hours = (msecsElapsed/(MSECS_PER_MIN * MIN_PER_HOUR));

            mg_printf(connection, "%s File Load Took: ", theServer->getMyServerName());
            if (hours > 0) {
                mg_printf(connection, "%d hour%s ", hours, (hours > 1) ? "s" : "" );
            }
            if (minutes > 0) {
                mg_printf(connection, "%d minute%s ", minutes, (minutes > 1) ? "s" : "");
            }
            if (seconds >= 0) {
                mg_printf(connection, "%.3f seconds", seconds);
            }
            mg_printf(connection, "%s", "\r\n");

        } else {
            mg_printf(connection, "%s", "Voxels not yet loaded...\r\n");
        }

        mg_printf(connection, "%s", "\r\n");

        mg_printf(connection, "%s", "\r\n");

        mg_printf(connection, "%s", "<b>Configuration:</b>\r\n");

        for (int i = 1; i < theServer->_argc; i++) {
            mg_printf(connection, "%s ", theServer->_argv[i]);
        }
        mg_printf(connection, "%s", "\r\n"); // one to end the config line
        mg_printf(connection, "%s", "\r\n"); // two more for spacing
        mg_printf(connection, "%s", "\r\n");

        // display scene stats
        unsigned long nodeCount = OctreeElement::getNodeCount();
        unsigned long internalNodeCount = OctreeElement::getInternalNodeCount();
        unsigned long leafNodeCount = OctreeElement::getLeafNodeCount();
        
        QLocale locale(QLocale::English);
        const float AS_PERCENT = 100.0;
        mg_printf(connection, "%s", "<b>Current Nodes in scene:</b>\r\n");
        mg_printf(connection, "       Total Nodes: %s nodes\r\n",
                    locale.toString((uint)nodeCount).rightJustified(16, ' ').toLocal8Bit().constData());
        mg_printf(connection, "    Internal Nodes: %s nodes (%5.2f%%)\r\n",
            locale.toString((uint)internalNodeCount).rightJustified(16, ' ').toLocal8Bit().constData(),
            ((float)internalNodeCount / (float)nodeCount) * AS_PERCENT);
        mg_printf(connection, "        Leaf Nodes: %s nodes (%5.2f%%)\r\n", 
            locale.toString((uint)leafNodeCount).rightJustified(16, ' ').toLocal8Bit().constData(),
            ((float)leafNodeCount / (float)nodeCount) * AS_PERCENT);
        mg_printf(connection, "%s", "\r\n");
        mg_printf(connection, "%s", "\r\n");


        // display outbound packet stats
        mg_printf(connection, "<b>%s Outbound Packet Statistics...</b>\r\n", theServer->getMyServerName());
        uint64_t totalOutboundPackets = OctreeSendThread::_totalPackets;
        uint64_t totalOutboundBytes = OctreeSendThread::_totalBytes;
        uint64_t totalWastedBytes = OctreeSendThread::_totalWastedBytes;
        uint64_t totalBytesOfOctalCodes = OctreePacketData::getTotalBytesOfOctalCodes();
        uint64_t totalBytesOfBitMasks = OctreePacketData::getTotalBytesOfBitMasks();
        uint64_t totalBytesOfColor = OctreePacketData::getTotalBytesOfColor();

        const int COLUMN_WIDTH = 10;
        mg_printf(connection, "           Total Outbound Packets: %s packets\r\n",
            locale.toString((uint)totalOutboundPackets).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
        mg_printf(connection, "             Total Outbound Bytes: %s bytes\r\n",
            locale.toString((uint)totalOutboundBytes).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
        mg_printf(connection, "               Total Wasted Bytes: %s bytes\r\n",
            locale.toString((uint)totalWastedBytes).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
        mg_printf(connection, "            Total OctalCode Bytes: %s bytes (%5.2f%%)\r\n",
            locale.toString((uint)totalBytesOfOctalCodes).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData(),
            ((float)totalBytesOfOctalCodes / (float)totalOutboundBytes) * AS_PERCENT);
        mg_printf(connection, "             Total BitMasks Bytes: %s bytes (%5.2f%%)\r\n",
            locale.toString((uint)totalBytesOfBitMasks).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData(),
            ((float)totalBytesOfBitMasks / (float)totalOutboundBytes) * AS_PERCENT);
        mg_printf(connection, "                Total Color Bytes: %s bytes (%5.2f%%)\r\n",
            locale.toString((uint)totalBytesOfColor).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData(),
            ((float)totalBytesOfColor / (float)totalOutboundBytes) * AS_PERCENT);

        mg_printf(connection, "%s", "\r\n");
        mg_printf(connection, "%s", "\r\n");

        // display inbound packet stats
        mg_printf(connection, "<b>%s Edit Statistics... <a href='/resetStats'>[RESET]</a></b>\r\n", 
                        theServer->getMyServerName());
        uint64_t averageTransitTimePerPacket = theServer->_octreeInboundPacketProcessor->getAverageTransitTimePerPacket();
        uint64_t averageProcessTimePerPacket = theServer->_octreeInboundPacketProcessor->getAverageProcessTimePerPacket();
        uint64_t averageLockWaitTimePerPacket = theServer->_octreeInboundPacketProcessor->getAverageLockWaitTimePerPacket();
        uint64_t averageProcessTimePerElement = theServer->_octreeInboundPacketProcessor->getAverageProcessTimePerElement();
        uint64_t averageLockWaitTimePerElement = theServer->_octreeInboundPacketProcessor->getAverageLockWaitTimePerElement();
        uint64_t totalElementsProcessed = theServer->_octreeInboundPacketProcessor->getTotalElementsProcessed();
        uint64_t totalPacketsProcessed = theServer->_octreeInboundPacketProcessor->getTotalPacketsProcessed();

        float averageElementsPerPacket = totalPacketsProcessed == 0 ? 0 : totalElementsProcessed / totalPacketsProcessed;

        mg_printf(connection, "           Total Inbound Packets: %s packets\r\n",
            locale.toString((uint)totalPacketsProcessed).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
        mg_printf(connection, "          Total Inbound Elements: %s elements\r\n",
            locale.toString((uint)totalElementsProcessed).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
        mg_printf(connection, " Average Inbound Elements/Packet: %f elements/packet\r\n", averageElementsPerPacket);
        mg_printf(connection, "     Average Transit Time/Packet: %s usecs\r\n", 
            locale.toString((uint)averageTransitTimePerPacket).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
        mg_printf(connection, "     Average Process Time/Packet: %s usecs\r\n",
            locale.toString((uint)averageProcessTimePerPacket).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
        mg_printf(connection, "   Average Wait Lock Time/Packet: %s usecs\r\n", 
            locale.toString((uint)averageLockWaitTimePerPacket).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
        mg_printf(connection, "    Average Process Time/Element: %s usecs\r\n",
            locale.toString((uint)averageProcessTimePerElement).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
        mg_printf(connection, "  Average Wait Lock Time/Element: %s usecs\r\n", 
            locale.toString((uint)averageLockWaitTimePerElement).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());


        int senderNumber = 0;
        NodeToSenderStatsMap& allSenderStats = theServer->_octreeInboundPacketProcessor->getSingleSenderStats();
        for (NodeToSenderStatsMapIterator i = allSenderStats.begin(); i != allSenderStats.end(); i++) {
            senderNumber++;
            QUuid senderID = i->first;
            SingleSenderStats& senderStats = i->second;

            mg_printf(connection, "\r\n             Stats for sender %d uuid: %s\r\n", senderNumber, 
                senderID.toString().toLocal8Bit().constData());

            averageTransitTimePerPacket = senderStats.getAverageTransitTimePerPacket();
            averageProcessTimePerPacket = senderStats.getAverageProcessTimePerPacket();
            averageLockWaitTimePerPacket = senderStats.getAverageLockWaitTimePerPacket();
            averageProcessTimePerElement = senderStats.getAverageProcessTimePerElement();
            averageLockWaitTimePerElement = senderStats.getAverageLockWaitTimePerElement();
            totalElementsProcessed = senderStats.getTotalElementsProcessed();
            totalPacketsProcessed = senderStats.getTotalPacketsProcessed();

            averageElementsPerPacket = totalPacketsProcessed == 0 ? 0 : totalElementsProcessed / totalPacketsProcessed;

            mg_printf(connection, "               Total Inbound Packets: %s packets\r\n",
                locale.toString((uint)totalPacketsProcessed).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
            mg_printf(connection, "              Total Inbound Elements: %s elements\r\n",
                locale.toString((uint)totalElementsProcessed).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
            mg_printf(connection, "     Average Inbound Elements/Packet: %f elements/packet\r\n", averageElementsPerPacket);
            mg_printf(connection, "         Average Transit Time/Packet: %s usecs\r\n", 
                locale.toString((uint)averageTransitTimePerPacket).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
            mg_printf(connection, "         Average Process Time/Packet: %s usecs\r\n",
                locale.toString((uint)averageProcessTimePerPacket).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
            mg_printf(connection, "       Average Wait Lock Time/Packet: %s usecs\r\n", 
                locale.toString((uint)averageLockWaitTimePerPacket).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
            mg_printf(connection, "        Average Process Time/Element: %s usecs\r\n",
                locale.toString((uint)averageProcessTimePerElement).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
            mg_printf(connection, "      Average Wait Lock Time/Element: %s usecs\r\n", 
                locale.toString((uint)averageLockWaitTimePerElement).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());

        }


        mg_printf(connection, "%s", "\r\n");
        mg_printf(connection, "%s", "\r\n");

        // display memory usage stats
        mg_printf(connection, "%s", "<b>Current Memory Usage Statistics</b>\r\n");
        mg_printf(connection, "\r\nOctreeElement size... %ld bytes\r\n", sizeof(OctreeElement));
        mg_printf(connection, "%s", "\r\n");

        const char* memoryScaleLabel;
        const float MEGABYTES = 1000000.f;
        const float GIGABYTES = 1000000000.f;
        float memoryScale;
        if (OctreeElement::getTotalMemoryUsage() / MEGABYTES < 1000.0f) {
            memoryScaleLabel = "MB";
            memoryScale = MEGABYTES;
        } else {
            memoryScaleLabel = "GB";
            memoryScale = GIGABYTES;
        }

        mg_printf(connection, "Element Node Memory Usage:       %8.2f %s\r\n", 
            OctreeElement::getVoxelMemoryUsage() / memoryScale, memoryScaleLabel);
        mg_printf(connection, "Octcode Memory Usage:            %8.2f %s\r\n", 
            OctreeElement::getOctcodeMemoryUsage() / memoryScale, memoryScaleLabel);
        mg_printf(connection, "External Children Memory Usage:  %8.2f %s\r\n", 
            OctreeElement::getExternalChildrenMemoryUsage() / memoryScale, memoryScaleLabel);
        mg_printf(connection, "%s", "                                 -----------\r\n");
        mg_printf(connection, "                         Total:  %8.2f %s\r\n", 
            OctreeElement::getTotalMemoryUsage() / memoryScale, memoryScaleLabel);

        mg_printf(connection, "%s", "\r\n");
        mg_printf(connection, "%s", "OctreeElement Children Population Statistics...\r\n");
        checkSum = 0;
        for (int i=0; i <= NUMBER_OF_CHILDREN; i++) {
            checkSum += OctreeElement::getChildrenCount(i);
            mg_printf(connection, "    Nodes with %d children:      %s nodes (%5.2f%%)\r\n", i, 
                locale.toString((uint)OctreeElement::getChildrenCount(i)).rightJustified(16, ' ').toLocal8Bit().constData(),
                ((float)OctreeElement::getChildrenCount(i) / (float)nodeCount) * AS_PERCENT);
        }
        mg_printf(connection, "%s", "                                ----------------------\r\n");
        mg_printf(connection, "                    Total:      %s nodes\r\n", 
            locale.toString((uint)checkSum).rightJustified(16, ' ').toLocal8Bit().constData());

#ifdef BLENDED_UNION_CHILDREN
        mg_printf(connection, "%s", "\r\n");
        mg_printf(connection, "%s", "OctreeElement Children Encoding Statistics...\r\n");
        
        mg_printf(connection, "    Single or No Children:      %10.llu nodes (%5.2f%%)\r\n",
            OctreeElement::getSingleChildrenCount(), ((float)OctreeElement::getSingleChildrenCount() / (float)nodeCount) * AS_PERCENT);
        mg_printf(connection, "    Two Children as Offset:     %10.llu nodes (%5.2f%%)\r\n", 
            OctreeElement::getTwoChildrenOffsetCount(), 
            ((float)OctreeElement::getTwoChildrenOffsetCount() / (float)nodeCount) * AS_PERCENT);
        mg_printf(connection, "    Two Children as External:   %10.llu nodes (%5.2f%%)\r\n", 
            OctreeElement::getTwoChildrenExternalCount(), 
            ((float)OctreeElement::getTwoChildrenExternalCount() / (float)nodeCount) * AS_PERCENT);
        mg_printf(connection, "    Three Children as Offset:   %10.llu nodes (%5.2f%%)\r\n", 
            OctreeElement::getThreeChildrenOffsetCount(), 
            ((float)OctreeElement::getThreeChildrenOffsetCount() / (float)nodeCount) * AS_PERCENT);
        mg_printf(connection, "    Three Children as External: %10.llu nodes (%5.2f%%)\r\n", 
            OctreeElement::getThreeChildrenExternalCount(), 
            ((float)OctreeElement::getThreeChildrenExternalCount() / (float)nodeCount) * AS_PERCENT);
        mg_printf(connection, "    Children as External Array: %10.llu nodes (%5.2f%%)\r\n",
            OctreeElement::getExternalChildrenCount(), 
            ((float)OctreeElement::getExternalChildrenCount() / (float)nodeCount) * AS_PERCENT);

        checkSum = OctreeElement::getSingleChildrenCount() +
                            OctreeElement::getTwoChildrenOffsetCount() + OctreeElement::getTwoChildrenExternalCount() + 
                            OctreeElement::getThreeChildrenOffsetCount() + OctreeElement::getThreeChildrenExternalCount() + 
                            OctreeElement::getExternalChildrenCount();

        mg_printf(connection, "%s", "                                ----------------\r\n");
        mg_printf(connection, "                         Total: %10.llu nodes\r\n", checkSum);
        mg_printf(connection, "                      Expected: %10.lu nodes\r\n", nodeCount);

        mg_printf(connection, "%s", "\r\n");
        mg_printf(connection, "%s", "In other news....\r\n");
        mg_printf(connection, "could store 4 children internally:     %10.llu nodes\r\n",
            OctreeElement::getCouldStoreFourChildrenInternally());
        mg_printf(connection, "could NOT store 4 children internally: %10.llu nodes\r\n", 
            OctreeElement::getCouldNotStoreFourChildrenInternally());
#endif

        mg_printf(connection, "%s", "\r\n");
        mg_printf(connection, "%s", "\r\n");
        mg_printf(connection, "%s", "</pre>\r\n");

        mg_printf(connection, "%s", "</doc></html>");

        return 1;
    } else {
        // have mongoose process this request from the document_root
        return 0;
    }
}


void OctreeServer::setArguments(int argc, char** argv) {
    _argc = argc;
    _argv = const_cast<const char**>(argv);

    qDebug("OctreeServer::setArguments()\n");
    for (int i = 0; i < _argc; i++) {
        qDebug("_argv[%d]=%s\n", i, _argv[i]);
    }

}

void OctreeServer::parsePayload() {
    
    if (getNumPayloadBytes() > 0) {
        QString config((const char*) _payload);
        
        // Now, parse the config
        QStringList configList = config.split(" ");
        
        int argCount = configList.size() + 1;

        qDebug("OctreeServer::parsePayload()... argCount=%d\n",argCount);

        _parsedArgV = new char*[argCount];
        const char* dummy = "config-from-payload";
        _parsedArgV[0] = new char[strlen(dummy) + sizeof(char)];
        strcpy(_parsedArgV[0], dummy);

        for (int i = 1; i < argCount; i++) {
            QString configItem = configList.at(i-1);
            _parsedArgV[i] = new char[configItem.length() + sizeof(char)];
            strcpy(_parsedArgV[i], configItem.toLocal8Bit().constData());
            qDebug("OctreeServer::parsePayload()... _parsedArgV[%d]=%s\n", i, _parsedArgV[i]);
        }

        setArguments(argCount, _parsedArgV);
    }
}

void OctreeServer::processDatagram(const QByteArray& dataByteArray, const HifiSockAddr& senderSockAddr) {
    NodeList* nodeList = NodeList::getInstance();
    
    PACKET_TYPE packetType = dataByteArray[0];
    
    if (packetType == getMyQueryMessageType()) {
        bool debug = false;
        if (debug) {
            qDebug() << "Got PACKET_TYPE_VOXEL_QUERY at " << usecTimestampNow() << "\n";
        }
        
        int numBytesPacketHeader = numBytesForPacketHeader((unsigned char*) dataByteArray.data());
        
        // If we got a PACKET_TYPE_VOXEL_QUERY, then we're talking to an NODE_TYPE_AVATAR, and we
        // need to make sure we have it in our nodeList.
        QUuid nodeUUID = QUuid::fromRfc4122(dataByteArray.mid(numBytesPacketHeader,
                                                              NUM_BYTES_RFC4122_UUID));
        
        Node* node = nodeList->nodeWithUUID(nodeUUID);
        
        if (node) {
            nodeList->updateNodeWithData(node, senderSockAddr, (unsigned char *) dataByteArray.data(),
                                         dataByteArray.size());
            if (!node->getActiveSocket()) {
                // we don't have an active socket for this node, but they're talking to us
                // this means they've heard from us and can reply, let's assume public is active
                node->activatePublicSocket();
            }
            OctreeQueryNode* nodeData = (OctreeQueryNode*) node->getLinkedData();
            if (nodeData && !nodeData->isOctreeSendThreadInitalized()) {
                nodeData->initializeOctreeSendThread(this);
            }
        }
    } else if (packetType == PACKET_TYPE_JURISDICTION_REQUEST) {
        _jurisdictionSender->queueReceivedPacket(senderSockAddr, (unsigned char*) dataByteArray.data(),
                                                 dataByteArray.size());
    } else if (_octreeInboundPacketProcessor && getOctree()->handlesEditPacketType(packetType)) {
       _octreeInboundPacketProcessor->queueReceivedPacket(senderSockAddr, (unsigned char*) dataByteArray.data(),
                                                                    dataByteArray.size());
   } else {
       // let processNodeData handle it.
       NodeList::getInstance()->processNodeData(senderSockAddr, (unsigned char*) dataByteArray.data(),
                                                dataByteArray.size());
    }
}

void OctreeServer::run() {
    // Before we do anything else, create our tree...
    _tree = createTree();
    
    // change the logging target name while this is running
    Logging::setTargetName(getMyLoggingServerTargetName());

    // Now would be a good time to parse our arguments, if we got them as assignment
    if (getNumPayloadBytes() > 0) {
        parsePayload();
    }

    beforeRun(); // after payload has been processed

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

    NodeList* nodeList = NodeList::getInstance();
    nodeList->setOwnerType(getMyNodeType());
    
    // we need to ask the DS about agents so we can ping/reply with them
    const char nodeTypesOfInterest[] = { NODE_TYPE_AGENT, NODE_TYPE_ANIMATION_SERVER};
    nodeList->setNodeTypesOfInterest(nodeTypesOfInterest, sizeof(nodeTypesOfInterest));
    
    setvbuf(stdout, NULL, _IOLBF, 0);

    // tell our NodeList about our desire to get notifications
    nodeList->addHook(this);
    nodeList->linkedDataCreateCallback = &OctreeServer::attachQueryNodeToNode;

    srand((unsigned)time(0));
    
    const char* VERBOSE_DEBUG = "--verboseDebug";
    _verboseDebug =  cmdOptionExists(_argc, _argv, VERBOSE_DEBUG);
    qDebug("verboseDebug=%s\n", debug::valueOf(_verboseDebug));

    const char* DEBUG_SENDING = "--debugSending";
    _debugSending =  cmdOptionExists(_argc, _argv, DEBUG_SENDING);
    qDebug("debugSending=%s\n", debug::valueOf(_debugSending));

    const char* DEBUG_RECEIVING = "--debugReceiving";
    _debugReceiving =  cmdOptionExists(_argc, _argv, DEBUG_RECEIVING);
    qDebug("debugReceiving=%s\n", debug::valueOf(_debugReceiving));

    // By default we will persist, if you want to disable this, then pass in this parameter
    const char* NO_PERSIST = "--NoPersist";
    if (cmdOptionExists(_argc, _argv, NO_PERSIST)) {
        _wantPersist = false;
    }
    qDebug("wantPersist=%s\n", debug::valueOf(_wantPersist));

    // if we want Persistence, set up the local file and persist thread
    if (_wantPersist) {

        // Check to see if the user passed in a command line option for setting packet send rate
        const char* PERSIST_FILENAME = "--persistFilename";
        const char* persistFilenameParameter = getCmdOption(_argc, _argv, PERSIST_FILENAME);
        if (persistFilenameParameter) {
            strcpy(_persistFilename, persistFilenameParameter);
        } else {
            strcpy(_persistFilename, getMyDefaultPersistFilename());
        }

        qDebug("persistFilename=%s\n", _persistFilename);

        // now set up PersistThread
        _persistThread = new OctreePersistThread(_tree, _persistFilename);
        if (_persistThread) {
            _persistThread->initialize(true);
        }
    }
    
    // Debug option to demonstrate that the server's local time does not 
    // need to be in sync with any other network node. This forces clock 
    // skew for the individual server node
    const char* CLOCK_SKEW = "--clockSkew";
    const char* clockSkewOption = getCmdOption(_argc, _argv, CLOCK_SKEW);
    if (clockSkewOption) {
        int clockSkew = atoi(clockSkewOption);
        usecTimestampNowForceClockSkew(clockSkew);
        qDebug("clockSkewOption=%s clockSkew=%d\n", clockSkewOption, clockSkew);
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

    HifiSockAddr senderSockAddr;
    
    // set up our jurisdiction broadcaster...
    if (_jurisdiction) {
        _jurisdiction->setNodeType(getMyNodeType());
    }
    _jurisdictionSender = new JurisdictionSender(_jurisdiction, getMyNodeType());
    _jurisdictionSender->initialize(true);
    
    // set up our OctreeServerPacketProcessor
    _octreeInboundPacketProcessor = new OctreeInboundPacketProcessor(this);
    _octreeInboundPacketProcessor->initialize(true);

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
    
    QTimer* domainServerTimer = new QTimer(this);
    connect(domainServerTimer, SIGNAL(timeout()), this, SLOT(checkInWithDomainServerOrExit()));
    domainServerTimer->start(DOMAIN_SERVER_CHECK_IN_USECS / 1000);
    
    QTimer* silentNodeTimer = new QTimer(this);
    connect(silentNodeTimer, SIGNAL(timeout()), nodeList, SLOT(removeSilentNodes()));
    silentNodeTimer->start(NODE_SILENCE_THRESHOLD_USECS / 1000);
    
    QTimer* pingNodesTimer = new QTimer(this);
    connect(pingNodesTimer, SIGNAL(timeout()), nodeList, SLOT(pingInactiveNodes()));
    pingNodesTimer->start(PING_INACTIVE_NODE_INTERVAL_USECS / 1000);
}
