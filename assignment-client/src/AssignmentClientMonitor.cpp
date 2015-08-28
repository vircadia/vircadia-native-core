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

#include <AddressManager.h>
#include <JSONBreakableMarshal.h>
#include <LogHandler.h>
#include <udt/PacketHeaders.h>

#include "AssignmentClientMonitor.h"
#include "AssignmentClientApp.h"
#include "AssignmentClientChildData.h"
#include "SharedUtil.h"

const QString ASSIGNMENT_CLIENT_MONITOR_TARGET_NAME = "assignment-client-monitor";
const int WAIT_FOR_CHILD_MSECS = 1000;

AssignmentClientMonitor::AssignmentClientMonitor(const unsigned int numAssignmentClientForks,
                                                 const unsigned int minAssignmentClientForks,
                                                 const unsigned int maxAssignmentClientForks,
                                                 Assignment::Type requestAssignmentType, QString assignmentPool,
                                                 quint16 listenPort, QUuid walletUUID, QString assignmentServerHostname,
                                                 quint16 assignmentServerPort) :
    _numAssignmentClientForks(numAssignmentClientForks),
    _minAssignmentClientForks(minAssignmentClientForks),
    _maxAssignmentClientForks(maxAssignmentClientForks),
    _requestAssignmentType(requestAssignmentType),
    _assignmentPool(assignmentPool),
    _walletUUID(walletUUID),
    _assignmentServerHostname(assignmentServerHostname),
    _assignmentServerPort(assignmentServerPort)
{
    qDebug() << "_requestAssignmentType =" << _requestAssignmentType;

    // start the Logging class with the parent's target name
    LogHandler::getInstance().setTargetName(ASSIGNMENT_CLIENT_MONITOR_TARGET_NAME);

    // make sure we output process IDs for a monitor otherwise it's insane to parse
    LogHandler::getInstance().setShouldOutputPID(true);

    // create a NodeList so we can receive stats from children
    DependencyManager::registerInheritance<LimitedNodeList, NodeList>();
    auto addressManager = DependencyManager::set<AddressManager>();
    auto nodeList = DependencyManager::set<LimitedNodeList>(listenPort);

    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerListener(PacketType::AssignmentClientStatus, this, "handleChildStatusPacket");

    // use QProcess to fork off a process for each of the child assignment clients
    for (unsigned int i = 0; i < _numAssignmentClientForks; i++) {
        spawnChildClient();
    }

    connect(&_checkSparesTimer, &QTimer::timeout, this, &AssignmentClientMonitor::checkSpares);

    _checkSparesTimer.start(NODE_SILENCE_THRESHOLD_MSECS * 3);
}

AssignmentClientMonitor::~AssignmentClientMonitor() {
    stopChildProcesses();
}

void AssignmentClientMonitor::simultaneousWaitOnChildren(int waitMsecs) {
    QElapsedTimer waitTimer;
    waitTimer.start();

    // loop as long as we still have processes around and we're inside the wait window
    while(_childProcesses.size() > 0 && !waitTimer.hasExpired(waitMsecs)) {
        // continue processing events so we can handle a process finishing up
        QCoreApplication::processEvents();
    }
}

void AssignmentClientMonitor::childProcessFinished() {
    QProcess* childProcess = qobject_cast<QProcess*>(sender());
    qint64 processID = _childProcesses.key(childProcess);

    if (processID > 0) {
        qDebug() << "Child process" << processID << "has finished. Removing from internal map.";
        _childProcesses.remove(processID);
    }
}

void AssignmentClientMonitor::stopChildProcesses() {
    auto nodeList = DependencyManager::get<NodeList>();

    // ask child processes to terminate
    foreach(QProcess* childProcess, _childProcesses) {
        qDebug() << "Attempting to terminate child process" << childProcess->processId();
        childProcess->terminate();
    }

    simultaneousWaitOnChildren(WAIT_FOR_CHILD_MSECS);

    if (_childProcesses.size() > 0) {
        // ask even more firmly
        foreach(QProcess* childProcess, _childProcesses) {
            qDebug() << "Attempting to kill child process" << childProcess->processId();
            childProcess->kill();
        }

        simultaneousWaitOnChildren(WAIT_FOR_CHILD_MSECS);
    }
}

void AssignmentClientMonitor::aboutToQuit() {
    stopChildProcesses();

    // clear the log handler so that Qt doesn't call the destructor on LogHandler
    qInstallMessageHandler(0);
}

void AssignmentClientMonitor::spawnChildClient() {
    QProcess* assignmentClient = new QProcess(this);


    // unparse the parts of the command-line that the child cares about
    QStringList _childArguments;
    if (_assignmentPool != "") {
        _childArguments.append("--" + ASSIGNMENT_POOL_OPTION);
        _childArguments.append(_assignmentPool);
    }
    if (!_walletUUID.isNull()) {
        _childArguments.append("--" + ASSIGNMENT_WALLET_DESTINATION_ID_OPTION);
        _childArguments.append(_walletUUID.toString());
    }
    if (_assignmentServerHostname != "") {
        _childArguments.append("--" + CUSTOM_ASSIGNMENT_SERVER_HOSTNAME_OPTION);
        _childArguments.append(_assignmentServerHostname);
    }
    if (_assignmentServerPort != DEFAULT_DOMAIN_SERVER_PORT) {
        _childArguments.append("--" + CUSTOM_ASSIGNMENT_SERVER_PORT_OPTION);
        _childArguments.append(QString::number(_assignmentServerPort));
    }
    if (_requestAssignmentType != Assignment::AllTypes) {
        _childArguments.append("--" + ASSIGNMENT_TYPE_OVERRIDE_OPTION);
        _childArguments.append(QString::number(_requestAssignmentType));
    }

    // tell children which assignment monitor port to use
    // for now they simply talk to us on localhost
    _childArguments.append("--" + ASSIGNMENT_CLIENT_MONITOR_PORT_OPTION);
    _childArguments.append(QString::number(DependencyManager::get<NodeList>()->getLocalSockAddr().getPort()));

    // make sure that the output from the child process appears in our output
    assignmentClient->setProcessChannelMode(QProcess::ForwardedChannels);

    assignmentClient->start(QCoreApplication::applicationFilePath(), _childArguments);

    // make sure we hear that this process has finished when it does
    connect(assignmentClient, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(childProcessFinished()));

    qDebug() << "Spawned a child client with PID" << assignmentClient->pid();
    _childProcesses.insert(assignmentClient->processId(), assignmentClient);
}

void AssignmentClientMonitor::checkSpares() {
    auto nodeList = DependencyManager::get<NodeList>();
    QUuid aSpareId = "";
    unsigned int spareCount = 0;
    unsigned int totalCount = 0;

    nodeList->removeSilentNodes();

    nodeList->eachNode([&](const SharedNodePointer& node) {
        AssignmentClientChildData* childData = static_cast<AssignmentClientChildData*>(node->getLinkedData());
        totalCount ++;
        if (childData->getChildType() == Assignment::Type::AllTypes) {
            ++spareCount;
            aSpareId = node->getUUID();
        }
    });

    // Spawn or kill children, as needed.  If --min or --max weren't specified, allow the child count
    // to drift up or down as far as needed.
    if (spareCount < 1 || totalCount < _minAssignmentClientForks) {
        if (!_maxAssignmentClientForks || totalCount < _maxAssignmentClientForks) {
            spawnChildClient();
        }
    }

    if (spareCount > 1) {
        if (!_minAssignmentClientForks || totalCount > _minAssignmentClientForks) {
            // kill aSpareId
            qDebug() << "asking child" << aSpareId << "to exit.";
            SharedNodePointer childNode = nodeList->nodeWithUUID(aSpareId);
            childNode->activateLocalSocket();

            auto diePacket = NLPacket::create(PacketType::StopNode, 0);
            nodeList->sendPacket(std::move(diePacket), *childNode);
        }
    }
}

void AssignmentClientMonitor::handleChildStatusPacket(QSharedPointer<NLPacket> packet) {
    // read out the sender ID
    QUuid senderID = QUuid::fromRfc4122(packet->readWithoutCopy(NUM_BYTES_RFC4122_UUID));

    auto nodeList = DependencyManager::get<NodeList>();

    SharedNodePointer matchingNode = nodeList->nodeWithUUID(senderID);
    const HifiSockAddr& senderSockAddr = packet->getSenderSockAddr();

    AssignmentClientChildData* childData = nullptr;

    if (!matchingNode) {
        // The parent only expects to be talking with prorams running on this same machine.
        if (senderSockAddr.getAddress() == QHostAddress::LocalHost ||
                senderSockAddr.getAddress() == QHostAddress::LocalHostIPv6) {
             
            if (!senderID.isNull()) {
                // We don't have this node yet - we should add it
                matchingNode = DependencyManager::get<LimitedNodeList>()->addOrUpdateNode
                    (senderID, NodeType::Unassigned, senderSockAddr, senderSockAddr, false, false);
                
                childData = new AssignmentClientChildData(Assignment::Type::AllTypes);
                matchingNode->setLinkedData(childData);
            } else {
                // tell unknown assignment-client child to exit.
                qDebug() << "Asking unknown child at" << senderSockAddr << "to exit.";

                auto diePacket = NLPacket::create(PacketType::StopNode, 0);
                nodeList->sendPacket(std::move(diePacket), senderSockAddr);

                return;
            }
        }
    } else {
        childData = dynamic_cast<AssignmentClientChildData*>(matchingNode->getLinkedData());
    }

    if (childData) {
        // update our records about how to reach this child
        matchingNode->setLocalSocket(senderSockAddr);

        // get child's assignment type out of the packet
        quint8 assignmentType;
        packet->readPrimitive(&assignmentType);
        
        childData->setChildType((Assignment::Type) assignmentType);
        
        // note when this child talked
        matchingNode->setLastHeardMicrostamp(usecTimestampNow());
    }
}
