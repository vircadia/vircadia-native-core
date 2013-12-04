//
//  ParticleSever.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/2/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
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
//#include <ParticleTree.h>
#include "ParticleReceiverData.h"
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

#include "ParticleSever.h"
#include "ParticleSeverConsts.h"

const char* LOCAL_PARTICLES_PERSIST_FILE = "resources/particles.svo";
const char* PARTICLES_PERSIST_FILE = "/etc/highfidelity/particle-server/resources/particles.svo";

void attachParticleReceiverDataToNode(Node* newNode) {
    if (newNode->getLinkedData() == NULL) {
        ParticleReceiverData* particleNodeData = new ParticleReceiverData(newNode);
        newNode->setLinkedData(particleNodeData);
    }
}

ParticleSever* ParticleSever::_theInstance = NULL;

ParticleSever::ParticleSever(const unsigned char* dataBuffer, int numBytes) : Assignment(dataBuffer, numBytes),
    _serverTree(true) {
    _argc = 0;
    _argv = NULL;

    _packetsPerClientPerInterval = 10;
    _wantParticlePersist = true;
    _wantLocalDomain = false;
    _debugParticleSending = false;
    _shouldShowAnimationDebug = false;
    _displayParticleStats = false;
    _debugParticleReceiving = false;
    _sendEnvironments = true;
    _sendMinimalEnvironment = false;
    _dumpParticlesOnMove = false;
    _verboseDebug = false;
    _jurisdiction = NULL;
    _jurisdictionSender = NULL;
    _ParticleSeverPacketProcessor = NULL;
    _particlePersistThread = NULL;
    _parsedArgV = NULL;
    
    _started = time(0);
    _startedUSecs = usecTimestampNow();
    
    _theInstance = this;
}

ParticleSever::~ParticleSever() {
    if (_parsedArgV) {
        for (int i = 0; i < _argc; i++) {
            delete[] _parsedArgV[i];
        }
        delete[] _parsedArgV;
    }
}

void ParticleSever::initMongoose(int port) {
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

int ParticleSever::civetwebRequestHandler(struct mg_connection* connection) {
    const struct mg_request_info* ri = mg_get_request_info(connection);
    
    ParticleSever* theServer = GetInstance();

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
        theServer->_ParticleSeverPacketProcessor->resetStats();
        showStats = true;
    }
    
    if (showStats) {
        uint64_t checkSum;
        // return a 200
        mg_printf(connection, "%s", "HTTP/1.0 200 OK\r\n");
        mg_printf(connection, "%s", "Content-Type: text/html\r\n\r\n");
        mg_printf(connection, "%s", "<html><doc>\r\n");
        mg_printf(connection, "%s", "<pre>\r\n");
        mg_printf(connection, "%s", "<b>Your Particle Server is running... <a href='/'>[RELOAD]</a></b>\r\n");

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


        // display particle file load time
        if (theServer->isInitialLoadComplete()) {
            time_t* loadCompleted = theServer->getLoadCompleted();
            if (loadCompleted) {
                tm* particlesLoadedAtLocal = localtime(loadCompleted);
                const int MAX_TIME_LENGTH = 128;
                char buffer[MAX_TIME_LENGTH];
                strftime(buffer, MAX_TIME_LENGTH, "%m/%d/%Y %X", particlesLoadedAtLocal);
                mg_printf(connection, "Particles Loaded At: %s", buffer);

                // Convert now to tm struct for UTC
                tm* particlesLoadedAtUTM = gmtime(theServer->getLoadCompleted());
                if (gmtm != NULL) {
                    strftime(buffer, MAX_TIME_LENGTH, "%m/%d/%Y %X", particlesLoadedAtUTM);
                    mg_printf(connection, " [%s UTM] ", buffer);
                }
            } else {
                mg_printf(connection, "%s", "Particle Persist Disabled...\r\n");
            }
            mg_printf(connection, "%s", "\r\n");


            uint64_t msecsElapsed = theServer->getLoadElapsedTime() / USECS_PER_MSEC;;
            float seconds = (msecsElapsed % MSECS_PER_MIN)/(float)MSECS_PER_SEC;
            int minutes = (msecsElapsed/(MSECS_PER_MIN)) % MIN_PER_HOUR;
            int hours = (msecsElapsed/(MSECS_PER_MIN * MIN_PER_HOUR));

            mg_printf(connection, "%s", "Particles Load Took: ");
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
            mg_printf(connection, "%s", "Particles not yet loaded...\r\n");
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
        unsigned long nodeCount = ParticleNode::getNodeCount();
        unsigned long internalNodeCount = ParticleNode::getInternalNodeCount();
        unsigned long leafNodeCount = ParticleNode::getLeafNodeCount();
        
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
        mg_printf(connection, "%s", "<b>Particle Packet Statistics...</b>\r\n");
        uint64_t totalOutboundPackets = ParticleSendThread::_totalPackets;
        uint64_t totalOutboundBytes = ParticleSendThread::_totalBytes;
        uint64_t totalWastedBytes = ParticleSendThread::_totalWastedBytes;
        uint64_t totalBytesOfOctalCodes = ParticlePacketData::getTotalBytesOfOctalCodes();
        uint64_t totalBytesOfBitMasks = ParticlePacketData::getTotalBytesOfBitMasks();
        uint64_t totalBytesOfColor = ParticlePacketData::getTotalBytesOfColor();

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
        mg_printf(connection, "%s", "<b>Particle Edit Statistics... <a href='/resetStats'>[RESET]</a></b>\r\n");
        uint64_t averageTransitTimePerPacket = theServer->_ParticleSeverPacketProcessor->getAverageTransitTimePerPacket();
        uint64_t averageProcessTimePerPacket = theServer->_particleServerPacketProcessor->getAverageProcessTimePerPacket();
        uint64_t averageLockWaitTimePerPacket = theServer->_particleServerPacketProcessor->getAverageLockWaitTimePerPacket();
        uint64_t averageProcessTimePerParticle = theServer->_particleServerPacketProcessor->getAverageProcessTimePerParticle();
        uint64_t averageLockWaitTimePerParticle = theServer->_particleServerPacketProcessor->getAverageLockWaitTimePerParticle();
        uint64_t totalParticlesProcessed = theServer->_particleServerPacketProcessor->getTotalParticlesProcessed();
        uint64_t totalPacketsProcessed = theServer->_particleServerPacketProcessor->getTotalPacketsProcessed();

        float averageParticlesPerPacket = totalPacketsProcessed == 0 ? 0 : totalParticlesProcessed / totalPacketsProcessed;

        mg_printf(connection, "           Total Inbound Packets: %s packets\r\n",
            locale.toString((uint)totalPacketsProcessed).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
        mg_printf(connection, "            Total Inbound Particles: %s particles\r\n",
            locale.toString((uint)totalParticlesProcessed).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
        mg_printf(connection, "   Average Inbound Particles/Packet: %f particles/packet\r\n", averageParticlesPerPacket);
        mg_printf(connection, "     Average Transit Time/Packet: %s usecs\r\n", 
            locale.toString((uint)averageTransitTimePerPacket).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
        mg_printf(connection, "     Average Process Time/Packet: %s usecs\r\n",
            locale.toString((uint)averageProcessTimePerPacket).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
        mg_printf(connection, "   Average Wait Lock Time/Packet: %s usecs\r\n", 
            locale.toString((uint)averageLockWaitTimePerPacket).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
        mg_printf(connection, "      Average Process Time/Particle: %s usecs\r\n",
            locale.toString((uint)averageProcessTimePerParticle).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
        mg_printf(connection, "    Average Wait Lock Time/Particle: %s usecs\r\n", 
            locale.toString((uint)averageLockWaitTimePerParticle).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());


        int senderNumber = 0;
        NodeToSenderStatsMap& allSenderStats = theServer->_particleServerPacketProcessor->getSingleSenderStats();
        for (NodeToSenderStatsMapIterator i = allSenderStats.begin(); i != allSenderStats.end(); i++) {
            senderNumber++;
            QUuid senderID = i->first;
            SingleSenderStats& senderStats = i->second;

            mg_printf(connection, "\r\n             Stats for sender %d uuid: %s\r\n", senderNumber, 
                senderID.toString().toLocal8Bit().constData());

            averageTransitTimePerPacket = senderStats.getAverageTransitTimePerPacket();
            averageProcessTimePerPacket = senderStats.getAverageProcessTimePerPacket();
            averageLockWaitTimePerPacket = senderStats.getAverageLockWaitTimePerPacket();
            averageProcessTimePerParticle = senderStats.getAverageProcessTimePerParticle();
            averageLockWaitTimePerParticle = senderStats.getAverageLockWaitTimePerParticle();
            totalParticlesProcessed = senderStats.getTotalParticlesProcessed();
            totalPacketsProcessed = senderStats.getTotalPacketsProcessed();

            averageParticlesPerPacket = totalPacketsProcessed == 0 ? 0 : totalParticlesProcessed / totalPacketsProcessed;

            mg_printf(connection, "               Total Inbound Packets: %s packets\r\n",
                locale.toString((uint)totalPacketsProcessed).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
            mg_printf(connection, "                Total Inbound Particles: %s particles\r\n",
                locale.toString((uint)totalParticlesProcessed).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
            mg_printf(connection, "       Average Inbound Particles/Packet: %f particles/packet\r\n", averageParticlesPerPacket);
            mg_printf(connection, "         Average Transit Time/Packet: %s usecs\r\n", 
                locale.toString((uint)averageTransitTimePerPacket).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
            mg_printf(connection, "         Average Process Time/Packet: %s usecs\r\n",
                locale.toString((uint)averageProcessTimePerPacket).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
            mg_printf(connection, "       Average Wait Lock Time/Packet: %s usecs\r\n", 
                locale.toString((uint)averageLockWaitTimePerPacket).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
            mg_printf(connection, "          Average Process Time/Particle: %s usecs\r\n",
                locale.toString((uint)averageProcessTimePerParticle).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());
            mg_printf(connection, "        Average Wait Lock Time/Particle: %s usecs\r\n", 
                locale.toString((uint)averageLockWaitTimePerParticle).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData());

        }


        mg_printf(connection, "%s", "\r\n");
        mg_printf(connection, "%s", "\r\n");

        // display memory usage stats
        mg_printf(connection, "%s", "<b>Current Memory Usage Statistics</b>\r\n");
        mg_printf(connection, "\r\nParticleNode size... %ld bytes\r\n", sizeof(ParticleNode));
        mg_printf(connection, "%s", "\r\n");

        const char* memoryScaleLabel;
        const float MEGABYTES = 1000000.f;
        const float GIGABYTES = 1000000000.f;
        float memoryScale;
        if (ParticleNode::getTotalMemoryUsage() / MEGABYTES < 1000.0f) {
            memoryScaleLabel = "MB";
            memoryScale = MEGABYTES;
        } else {
            memoryScaleLabel = "GB";
            memoryScale = GIGABYTES;
        }

        mg_printf(connection, "Particle Node Memory Usage:         %8.2f %s\r\n", 
            ParticleNode::getParticleMemoryUsage() / memoryScale, memoryScaleLabel);
        mg_printf(connection, "Octcode Memory Usage:            %8.2f %s\r\n", 
            ParticleNode::getOctcodeMemoryUsage() / memoryScale, memoryScaleLabel);
        mg_printf(connection, "External Children Memory Usage:  %8.2f %s\r\n", 
            ParticleNode::getExternalChildrenMemoryUsage() / memoryScale, memoryScaleLabel);
        mg_printf(connection, "%s", "                                 -----------\r\n");
        mg_printf(connection, "                         Total:  %8.2f %s\r\n", 
            ParticleNode::getTotalMemoryUsage() / memoryScale, memoryScaleLabel);

        mg_printf(connection, "%s", "\r\n");
        mg_printf(connection, "%s", "ParticleNode Children Population Statistics...\r\n");
        checkSum = 0;
        for (int i=0; i <= NUMBER_OF_CHILDREN; i++) {
            checkSum += ParticleNode::getChildrenCount(i);
            mg_printf(connection, "    Nodes with %d children:      %s nodes (%5.2f%%)\r\n", i, 
                locale.toString((uint)ParticleNode::getChildrenCount(i)).rightJustified(16, ' ').toLocal8Bit().constData(),
                ((float)ParticleNode::getChildrenCount(i) / (float)nodeCount) * AS_PERCENT);
        }
        mg_printf(connection, "%s", "                                ----------------------\r\n");
        mg_printf(connection, "                    Total:      %s nodes\r\n", 
            locale.toString((uint)checkSum).rightJustified(16, ' ').toLocal8Bit().constData());

#ifdef BLENDED_UNION_CHILDREN
        mg_printf(connection, "%s", "\r\n");
        mg_printf(connection, "%s", "ParticleNode Children Encoding Statistics...\r\n");
        
        mg_printf(connection, "    Single or No Children:      %10.llu nodes (%5.2f%%)\r\n",
            ParticleNode::getSingleChildrenCount(), ((float)ParticleNode::getSingleChildrenCount() / (float)nodeCount) * AS_PERCENT);
        mg_printf(connection, "    Two Children as Offset:     %10.llu nodes (%5.2f%%)\r\n", 
            ParticleNode::getTwoChildrenOffsetCount(), 
            ((float)ParticleNode::getTwoChildrenOffsetCount() / (float)nodeCount) * AS_PERCENT);
        mg_printf(connection, "    Two Children as External:   %10.llu nodes (%5.2f%%)\r\n", 
            ParticleNode::getTwoChildrenExternalCount(), 
            ((float)ParticleNode::getTwoChildrenExternalCount() / (float)nodeCount) * AS_PERCENT);
        mg_printf(connection, "    Three Children as Offset:   %10.llu nodes (%5.2f%%)\r\n", 
            ParticleNode::getThreeChildrenOffsetCount(), 
            ((float)ParticleNode::getThreeChildrenOffsetCount() / (float)nodeCount) * AS_PERCENT);
        mg_printf(connection, "    Three Children as External: %10.llu nodes (%5.2f%%)\r\n", 
            ParticleNode::getThreeChildrenExternalCount(), 
            ((float)ParticleNode::getThreeChildrenExternalCount() / (float)nodeCount) * AS_PERCENT);
        mg_printf(connection, "    Children as External Array: %10.llu nodes (%5.2f%%)\r\n",
            ParticleNode::getExternalChildrenCount(), 
            ((float)ParticleNode::getExternalChildrenCount() / (float)nodeCount) * AS_PERCENT);

        checkSum = ParticleNode::getSingleChildrenCount() +
                            ParticleNode::getTwoChildrenOffsetCount() + ParticleNode::getTwoChildrenExternalCount() + 
                            ParticleNode::getThreeChildrenOffsetCount() + ParticleNode::getThreeChildrenExternalCount() + 
                            ParticleNode::getExternalChildrenCount();

        mg_printf(connection, "%s", "                                ----------------\r\n");
        mg_printf(connection, "                         Total: %10.llu nodes\r\n", checkSum);
        mg_printf(connection, "                      Expected: %10.lu nodes\r\n", nodeCount);

        mg_printf(connection, "%s", "\r\n");
        mg_printf(connection, "%s", "In other news....\r\n");
        mg_printf(connection, "could store 4 children internally:     %10.llu nodes\r\n",
            ParticleNode::getCouldStoreFourChildrenInternally());
        mg_printf(connection, "could NOT store 4 children internally: %10.llu nodes\r\n", 
            ParticleNode::getCouldNotStoreFourChildrenInternally());
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


void ParticleSever::setArguments(int argc, char** argv) {
    _argc = argc;
    _argv = const_cast<const char**>(argv);

    qDebug("ParticleSever::setArguments()\n");
    for (int i = 0; i < _argc; i++) {
        qDebug("_argv[%d]=%s\n", i, _argv[i]);
    }

}

void ParticleSever::parsePayload() {
    
    if (getNumPayloadBytes() > 0) {
        QString config((const char*) _payload);
        
        // Now, parse the config
        QStringList configList = config.split(" ");
        
        int argCount = configList.size() + 1;

        qDebug("ParticleSever::parsePayload()... argCount=%d\n",argCount);

        _parsedArgV = new char*[argCount];
        const char* dummy = "config-from-payload";
        _parsedArgV[0] = new char[strlen(dummy) + sizeof(char)];
        strcpy(_parsedArgV[0], dummy);

        for (int i = 1; i < argCount; i++) {
            QString configItem = configList.at(i-1);
            _parsedArgV[i] = new char[configItem.length() + sizeof(char)];
            strcpy(_parsedArgV[i], configItem.toLocal8Bit().constData());
            qDebug("ParticleSever::parsePayload()... _parsedArgV[%d]=%s\n", i, _parsedArgV[i]);
        }

        setArguments(argCount, _parsedArgV);
    }
}

//int main(int argc, const char * argv[]) {
void ParticleSever::run() {
    
    const char PARTICLE_SERVER_LOGGING_TARGET_NAME[] = "particle-server";
    
    // change the logging target name while this is running
    Logging::setTargetName(PARTICLE_SERVER_LOGGING_TARGET_NAME);

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
    const char* DUMP_PARTICLES_ON_MOVE = "--dumpParticlesOnMove";
    _dumpParticlesOnMove = cmdOptionExists(_argc, _argv, DUMP_PARTICLES_ON_MOVE);
    qDebug("dumpParticlesOnMove=%s\n", debug::valueOf(_dumpParticlesOnMove));
    
    // should we send environments? Default is yes, but this command line suppresses sending
    const char* SEND_ENVIRONMENTS = "--sendEnvironments";
    bool dontSendEnvironments =  !cmdOptionExists(_argc, _argv, SEND_ENVIRONMENTS);
    if (dontSendEnvironments) {
        qDebug("Sending environments suppressed...\n");
        _sendEnvironments = false;
    } else { 
        // should we send environments? Default is yes, but this command line suppresses sending
        const char* MINIMAL_ENVIRONMENT = "--minimalEnvironment";
        _sendMinimalEnvironment =  cmdOptionExists(_argc, _argv, MINIMAL_ENVIRONMENT);
        qDebug("Using Minimal Environment=%s\n", debug::valueOf(_sendMinimalEnvironment));
    }
    qDebug("Sending environments=%s\n", debug::valueOf(_sendEnvironments));
    
    NodeList* nodeList = NodeList::getInstance();
    nodeList->setOwnerType(NODE_TYPE_PARTICLE_SERVER);
    
    // we need to ask the DS about agents so we can ping/reply with them
    const char nodeTypesOfInterest[] = { NODE_TYPE_AGENT, NODE_TYPE_ANIMATION_SERVER};
    nodeList->setNodeTypesOfInterest(nodeTypesOfInterest, sizeof(nodeTypesOfInterest));
    
    setvbuf(stdout, NULL, _IOLBF, 0);

    // tell our NodeList about our desire to get notifications
    nodeList->addHook(&_nodeWatcher);
    nodeList->linkedDataCreateCallback = &attachParticleReceiverDataToNode;

    nodeList->startSilentNodeRemovalThread();
    srand((unsigned)time(0));
    
    const char* DISPLAY_PARTICLE_STATS = "--displayParticleStats";
    _displayParticleStats =  cmdOptionExists(_argc, _argv, DISPLAY_PARTICLE_STATS);
    qDebug("displayParticleStats=%s\n", debug::valueOf(_displayParticleStats));

    const char* VERBOSE_DEBUG = "--verboseDebug";
    _verboseDebug =  cmdOptionExists(_argc, _argv, VERBOSE_DEBUG);
    qDebug("verboseDebug=%s\n", debug::valueOf(_verboseDebug));

    const char* DEBUG_PARTICLE_SENDING = "--debugParticleSending";
    _debugParticleSending =  cmdOptionExists(_argc, _argv, DEBUG_PARTICLE_SENDING);
    qDebug("debugParticleSending=%s\n", debug::valueOf(_debugParticleSending));

    const char* DEBUG_PARTICLE_RECEIVING = "--debugParticleReceiving";
    _debugParticleReceiving =  cmdOptionExists(_argc, _argv, DEBUG_PARTICLE_RECEIVING);
    qDebug("debugParticleReceiving=%s\n", debug::valueOf(_debugParticleReceiving));

    const char* WANT_ANIMATION_DEBUG = "--shouldShowAnimationDebug";
    _shouldShowAnimationDebug =  cmdOptionExists(_argc, _argv, WANT_ANIMATION_DEBUG);
    qDebug("shouldShowAnimationDebug=%s\n", debug::valueOf(_shouldShowAnimationDebug));

    // By default we will particle persist, if you want to disable this, then pass in this parameter
    const char* NO_PARTICLE_PERSIST = "--NoParticlePersist";
    if (cmdOptionExists(_argc, _argv, NO_PARTICLE_PERSIST)) {
        _wantParticlePersist = false;
    }
    qDebug("wantParticlePersist=%s\n", debug::valueOf(_wantParticlePersist));

    // if we want Particle Persistence, set up the local file and persist thread
    if (_wantParticlePersist) {

        // Check to see if the user passed in a command line option for setting packet send rate
        const char* PARTICLES_PERSIST_FILENAME = "--particlesPersistFilename";
        const char* particlesPersistFilenameParameter = getCmdOption(_argc, _argv, PARTICLES_PERSIST_FILENAME);
        if (particlesPersistFilenameParameter) {
            strcpy(_particlePersistFilename, particlesPersistFilenameParameter);
        } else {
            //strcpy(particlePersistFilename, _wantLocalDomain ? LOCAL_PARTICLES_PERSIST_FILE : PARTICLES_PERSIST_FILE);
            strcpy(_particlePersistFilename, LOCAL_PARTICLES_PERSIST_FILE);
        }

        qDebug("particlePersistFilename=%s\n", _particlePersistFilename);

        // now set up ParticlePersistThread
        _particlePersistThread = new ParticlePersistThread(&_serverTree, _particlePersistFilename);
        if (_particlePersistThread) {
            _particlePersistThread->initialize(true);
        }
    }

    // Check to see if the user passed in a command line option for loading an old style local
    // Particle File. If so, load it now. This is not the same as a particle persist file
    const char* INPUT_FILE = "-i";
    const char* particlesFilename = getCmdOption(_argc, _argv, INPUT_FILE);
    if (particlesFilename) {
        _serverTree.readFromSVOFile(particlesFilename);
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
    
    // set up our ParticleSeverPacketProcessor
    _particleServerPacketProcessor = new ParticleSeverPacketProcessor(this);
    if (_particleServerPacketProcessor) {
        _particleServerPacketProcessor->initialize(true);
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
        // check for >= in case one gets past the goalie
        if (NodeList::getInstance()->getNumNoReplyDomainCheckIns() >= MAX_SILENT_DOMAIN_SERVER_CHECK_INS) {
            qDebug() << "Exit loop... getInstance()->getNumNoReplyDomainCheckIns() >= MAX_SILENT_DOMAIN_SERVER_CHECK_INS\n";
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

            if (packetData[0] == PACKET_TYPE_PARTICLE_QUERY) {
                bool debug = false;
                if (debug) {
                    qDebug("Got PACKET_TYPE_PARTICLE_QUERY at %llu.\n", usecTimestampNow());
                }
            
                // If we got a PACKET_TYPE_PARTICLE_QUERY, then we're talking to an NODE_TYPE_AVATAR, and we
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
                    ParticleReceiverData* nodeData = (ParticleReceiverData*) node->getLinkedData();
                    if (nodeData && !nodeData->isParticleSendThreadInitalized()) {
                        nodeData->initializeParticleSendThread(this);
                    }
                }
            } else if (packetData[0] == PACKET_TYPE_PARTICLE_JURISDICTION_REQUEST) {
                if (_jurisdictionSender) {
                    _jurisdictionSender->queueReceivedPacket(senderAddress, packetData, packetLength);
                }
            } else if (_particleServerPacketProcessor &&
                       (packetData[0] == PACKET_TYPE_SET_VOXEL
                        || packetData[0] == PACKET_TYPE_SET_PARTICLE_DESTRUCTIVE
                        || packetData[0] == PACKET_TYPE_ERASE_VOXEL)) {


                const char* messageName;
                switch (packetData[0]) {
                    case PACKET_TYPE_SET_VOXEL: 
                        messageName = "PACKET_TYPE_SET_VOXEL"; 
                        break;
                    case PACKET_TYPE_SET_PARTICLE_DESTRUCTIVE: 
                        messageName = "PACKET_TYPE_SET_PARTICLE_DESTRUCTIVE"; 
                        break;
                    case PACKET_TYPE_ERASE_VOXEL: 
                        messageName = "PACKET_TYPE_ERASE_VOXEL"; 
                        break;
                }
                _particleServerPacketProcessor->queueReceivedPacket(senderAddress, packetData, packetLength);
            } else {
                // let processNodeData handle it.
                NodeList::getInstance()->processNodeData(&senderAddress, packetData, packetLength);
            }
        }
    }

    // call NodeList::clear() so that all of our node specific objects, including our sending threads, are
    // properly shutdown and cleaned up.
    NodeList::getInstance()->clear();
    
    if (_jurisdictionSender) {
        _jurisdictionSender->terminate();
        delete _jurisdictionSender;
    }

    if (_particleServerPacketProcessor) {
        _particleServerPacketProcessor->terminate();
        delete _particleServerPacketProcessor;
    }

    if (_particlePersistThread) {
        _particlePersistThread->terminate();
        delete _particlePersistThread;
    }
    
    // tell our NodeList we're done with notifications
    nodeList->removeHook(&_nodeWatcher);

    delete _jurisdiction;
    _jurisdiction = NULL;

    qDebug() << "ParticleSever::run()... DONE\n";
}


void ParticleSever::nodeAdded(Node* node) {
    // do nothing
}

void ParticleSever::nodeKilled(Node* node) {
    // Use this to cleanup our node
    if (node->getType() == NODE_TYPE_AGENT) {
        ParticleReceiverData* nodeData = (ParticleReceiverData*)node->getLinkedData();
        if (nodeData) {
            node->setLinkedData(NULL);
            delete nodeData;
        }
    }
};
