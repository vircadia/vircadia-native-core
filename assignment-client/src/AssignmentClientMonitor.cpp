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


#include <AddressManager.h> // XXX need this?


#include "AssignmentClientMonitor.h"
#include "PacketHeaders.h"

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
    // auto nodeList = DependencyManager::set<NodeList>(NodeType::Unassigned);
    auto nodeList = DependencyManager::set<LimitedNodeList>(DEFAULT_ASSIGNMENT_CLIENT_MONITOR_PORT,
                                                            DEFAULT_ASSIGNMENT_CLIENT_MONITOR_DTLS_PORT);

    connect(&nodeList->getNodeSocket(), &QUdpSocket::readyRead, this, &AssignmentClientMonitor::readPendingDatagrams);

    nodeList->putLocalPortIntoSharedMemory(ASSIGNMENT_CLIENT_MONITOR_LOCAL_PORT_SMEM_KEY, this);
    
    // use QProcess to fork off a process for each of the child assignment clients
    for (unsigned int i = 0; i < numAssignmentClientForks; i++) {
        spawnChildClient();
    }

    connect(&_checkSparesTimer, SIGNAL(timeout()), SLOT(checkSpares()));
    _checkSparesTimer.start(5000);
}

AssignmentClientMonitor::~AssignmentClientMonitor() {
    stopChildProcesses();

    foreach (AssignmentClientChildData* childStatus, _childStatus) {
        delete childStatus;
    }
}



void AssignmentClientMonitor::stopChildProcesses() {
    
    QList<QPointer<QProcess> >::Iterator it = _childProcesses.begin();
    while (it != _childProcesses.end()) {
        if (!it->isNull()) {
            qDebug() << "Monitor is terminating child process" << it->data();
            
            // don't re-spawn this child when it goes down
            disconnect(it->data(), 0, this, 0);
            
            it->data()->terminate();
            it->data()->waitForFinished();
        }
        
        it = _childProcesses.erase(it);
    }
}

void AssignmentClientMonitor::spawnChildClient() {
    QProcess *assignmentClient = new QProcess(this);
    
    _childProcesses.append(QPointer<QProcess>(assignmentClient));

    QUuid childUUID = QUuid::createUuid();

    // create a Node for this child.  this is done so we can idenitfy packets from unknown children
    DependencyManager::get<LimitedNodeList>()->addOrUpdateNode
        (childUUID, NodeType::Unassigned, HifiSockAddr("localhost", 0), HifiSockAddr("localhost", 0), false);
    
    // make sure that the output from the child process appears in our output
    assignmentClient->setProcessChannelMode(QProcess::ForwardedChannels);

    QStringList idArgs = QStringList() << "-i" << childUUID.toString();
    assignmentClient->start(applicationFilePath(), _childArguments + idArgs);
    
    // link the child processes' finished slot to our childProcessFinished slot
    connect(assignmentClient, SIGNAL(finished(int, QProcess::ExitStatus)), this,
            SLOT(childProcessFinished(int, QProcess::ExitStatus)));
    
    qDebug() << "Spawned a child client with PID" << assignmentClient->pid();
}

void AssignmentClientMonitor::childProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    qDebug("Replacing dead child assignment client with a new one");
    
    // remove the old process from our list of child processes
    qDebug() << "need to remove" << QPointer<QProcess>(qobject_cast<QProcess*>(sender()));
    _childProcesses.removeOne(QPointer<QProcess>(qobject_cast<QProcess*>(sender())));
    
    spawnChildClient();
}



void AssignmentClientMonitor::checkSpares() {
    qDebug() << "check spares:";

    QString aSpareId = "";
    unsigned int spareCount = 0;

    QHash<QString, AssignmentClientChildData*>::const_iterator i = _childStatus.constBegin();
    while (i != _childStatus.constEnd()) {
        qDebug() << "  " << i.key() << i.value()->getChildType();
        if (i.value()->getChildType() == "none") {
            spareCount ++;
            aSpareId = i.key();
        }
        ++i;
    }

    qDebug() << "spare count is" << spareCount;

    if (spareCount < 1) {
        qDebug() << "FORK";
        spawnChildClient();
    }

    if (spareCount > 1) {
        qDebug() << "KILL";
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
                // qDebug() << "packetUUID = " << packetUUID;

                SharedNodePointer matchingNode = nodeList->sendingNodeForPacket(receivedPacket);
                if (!matchingNode) {
                    qDebug() << "got packet from unknown child, id =" << packetUUID.toString();
                    // tell unknown assignment-client child to exit.
                    QByteArray diePacket = byteArrayWithPopulatedHeader(PacketTypeStopNode);
                    nodeList->writeUnverifiedDatagram(diePacket, senderSockAddr);
                }

                if (matchingNode) {
                    // update our records about how to reach this child
                    matchingNode->setLocalSocket(senderSockAddr);

                    // push past the packet header
                    QDataStream packetStream(receivedPacket);
                    packetStream.skipRawData(numBytesForPacketHeader(receivedPacket));

                    QVariantMap unpackedVariantMap;

                    packetStream >> unpackedVariantMap;

                    QJsonObject unpackedStatsJSON = QJsonObject::fromVariantMap(unpackedVariantMap);

                    // qDebug() << "ACM got stats packet, id =" << packetUUID.toString()
                    //          << "type =" << unpackedStatsJSON["assignment_type"];

                    QString key(QString(packetUUID.toString()));
                    if (_childStatus.contains(key)) {
                        delete _childStatus[ key ];
                    }

                    QString childType = unpackedStatsJSON["assignment_type"].toString();
                    auto childStatus = new AssignmentClientChildData(childType);
                    _childStatus[ key ] = childStatus;

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
