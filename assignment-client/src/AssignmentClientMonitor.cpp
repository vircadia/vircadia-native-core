//
//  AssignmentClientMonitor.cpp
//  assignment-client/src
//
//  Created by Stephen Birarda on 1/10/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <signal.h>

#include <LogHandler.h>
#include <ShutdownEventListener.h>
#include <AddressManager.h>

#include "AssignmentClientMonitor.h"
#include "PacketHeaders.h"
#include "SharedUtil.h"

const char* NUM_FORKS_PARAMETER = "-n";

const QString ASSIGNMENT_CLIENT_MONITOR_TARGET_NAME = "assignment-client-monitor";

AssignmentClientMonitor::AssignmentClientMonitor(int &argc, char **argv, const unsigned int numAssignmentClientForks) :
    QCoreApplication(argc, argv)
{    
    // start the Logging class with the parent's target name
    LogHandler::getInstance().setTargetName(ASSIGNMENT_CLIENT_MONITOR_TARGET_NAME);
    
    // setup a shutdown event listener to handle SIGTERM or WM_CLOSE for us
#ifdef _WIN32
    installNativeEventFilter(&ShutdownEventListener::getInstance());
#else
    ShutdownEventListener::getInstance();
#endif
    
    _childArguments = arguments();
    
    // remove the parameter for the number of forks so it isn't passed to the child forked processes
    int forksParameterIndex = _childArguments.indexOf(NUM_FORKS_PARAMETER);
    
    // this removes both the "-n" parameter and the number of forks passed
    _childArguments.removeAt(forksParameterIndex);
    _childArguments.removeAt(forksParameterIndex);


    // create a NodeList so we can receive stats from children
    DependencyManager::registerInheritance<LimitedNodeList, NodeList>();
    auto addressManager = DependencyManager::set<AddressManager>();
    auto nodeList = DependencyManager::set<LimitedNodeList>(DEFAULT_ASSIGNMENT_CLIENT_MONITOR_PORT,
                                                            DEFAULT_ASSIGNMENT_CLIENT_MONITOR_DTLS_PORT);

    connect(&nodeList->getNodeSocket(), &QUdpSocket::readyRead, this, &AssignmentClientMonitor::readPendingDatagrams);

    nodeList->putLocalPortIntoSharedMemory(ASSIGNMENT_CLIENT_MONITOR_LOCAL_PORT_SMEM_KEY, this);
    
    // use QProcess to fork off a process for each of the child assignment clients
    for (unsigned int i = 0; i < numAssignmentClientForks; i++) {
        spawnChildClient();
    }

    connect(&_checkSparesTimer, SIGNAL(timeout()), SLOT(checkSpares()));
    _checkSparesTimer.start(NODE_SILENCE_THRESHOLD_MSECS * 3);
}

AssignmentClientMonitor::~AssignmentClientMonitor() {
    stopChildProcesses();
}

void AssignmentClientMonitor::stopChildProcesses() {
    auto nodeList = DependencyManager::get<NodeList>();

    nodeList->eachNode([&](const SharedNodePointer& node){
            qDebug() << "asking child" << node->getUUID() << "to exit.";
            node->activateLocalSocket();
            QByteArray diePacket = byteArrayWithPopulatedHeader(PacketTypeStopNode);
            nodeList->writeUnverifiedDatagram(diePacket, *node->getActiveSocket());
        });
}

void AssignmentClientMonitor::spawnChildClient() {
    QProcess *assignmentClient = new QProcess(this);
    
    // make sure that the output from the child process appears in our output
    assignmentClient->setProcessChannelMode(QProcess::ForwardedChannels);
    
    assignmentClient->start(applicationFilePath(), _childArguments);

    qDebug() << "Spawned a child client with PID" << assignmentClient->pid();
}



void AssignmentClientMonitor::checkSpares() {
    auto nodeList = DependencyManager::get<NodeList>();
    QUuid aSpareId = "";
    unsigned int spareCount = 0;

    nodeList->removeSilentNodes();

    nodeList->eachNode([&](const SharedNodePointer& node){
            AssignmentClientChildData *childData = static_cast<AssignmentClientChildData*>(node->getLinkedData());
            if (childData->getChildType() == "none") {
                spareCount ++;
                aSpareId = node->getUUID();
            }
        });

    if (spareCount != 1) {
        qDebug() << "spare count is" << spareCount;
    }

    if (spareCount < 1) {
        spawnChildClient();
    }

    if (spareCount > 1) {
        // kill aSpareId
        qDebug() << "asking child" << aSpareId << "to exit.";
        SharedNodePointer childNode = nodeList->nodeWithUUID(aSpareId);
        childNode->activateLocalSocket();
        QByteArray diePacket = byteArrayWithPopulatedHeader(PacketTypeStopNode);
        nodeList->writeUnverifiedDatagram(diePacket, childNode);
    }
}


void AssignmentClientMonitor::readPendingDatagrams() {
    auto nodeList = DependencyManager::get<NodeList>();

    QByteArray receivedPacket;
    HifiSockAddr senderSockAddr;

    while (nodeList->getNodeSocket().hasPendingDatagrams()) {
        receivedPacket.resize(nodeList->getNodeSocket().pendingDatagramSize());
        nodeList->getNodeSocket().readDatagram(receivedPacket.data(), receivedPacket.size(),
                                               senderSockAddr.getAddressPointer(), senderSockAddr.getPortPointer());

        if (nodeList->packetVersionAndHashMatch(receivedPacket)) {
            if (packetTypeForPacket(receivedPacket) == PacketTypeNodeJsonStats) {
                QUuid packetUUID = uuidFromPacketHeader(receivedPacket);
                SharedNodePointer matchingNode = nodeList->sendingNodeForPacket(receivedPacket);
                if (!matchingNode) {
                    // The parent only expects to be talking with prorams running on this same machine.
                    if (senderSockAddr.getAddress() == QHostAddress::LocalHost ||
                        senderSockAddr.getAddress() == QHostAddress::LocalHostIPv6) {
                        if (!packetUUID.isNull()) {
                            matchingNode = DependencyManager::get<LimitedNodeList>()->addOrUpdateNode
                                (packetUUID, NodeType::Unassigned, senderSockAddr, senderSockAddr, false);
                            AssignmentClientChildData *childData = new AssignmentClientChildData("unknown");
                            matchingNode->setLinkedData(childData);
                        } else {
                            // tell unknown assignment-client child to exit.
                            qDebug() << "asking unknown child to exit.";
                            QByteArray diePacket = byteArrayWithPopulatedHeader(PacketTypeStopNode);
                            nodeList->writeUnverifiedDatagram(diePacket, senderSockAddr);
                        }
                    }
                }

                if (matchingNode) {
                    // update our records about how to reach this child
                    matchingNode->setLocalSocket(senderSockAddr);

                    // push past the packet header
                    QDataStream packetStream(receivedPacket);
                    packetStream.skipRawData(numBytesForPacketHeader(receivedPacket));
                    // decode json
                    QVariantMap unpackedVariantMap;
                    packetStream >> unpackedVariantMap;
                    QJsonObject unpackedStatsJSON = QJsonObject::fromVariantMap(unpackedVariantMap);

                    // get child's assignment type out of the decoded json
                    QString childType = unpackedStatsJSON["assignment_type"].toString();
                    AssignmentClientChildData *childData =
                        static_cast<AssignmentClientChildData*>(matchingNode->getLinkedData());
                    childData->setChildType(childType);
                    // note when this child talked
                    matchingNode->setLastHeardMicrostamp(usecTimestampNow());
                }
            } else {
                // have the NodeList attempt to handle it
                nodeList->processNodeData(senderSockAddr, receivedPacket);
            }
        }
    }
}


AssignmentClientChildData::AssignmentClientChildData(QString childType) {
    _childType = childType;
}


AssignmentClientChildData::~AssignmentClientChildData() {
}
