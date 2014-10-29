//
//  ThreadedAssignment.cpp
//  libraries/shared/src
//
//  Created by Stephen Birarda on 12/3/2013.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonObject>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <LogHandler.h>

#include "ThreadedAssignment.h"

ThreadedAssignment::ThreadedAssignment(const QByteArray& packet) :
    Assignment(packet),
    _isFinished(false),
    _datagramProcessingThread(NULL)
{
    
}

void ThreadedAssignment::setFinished(bool isFinished) {
    _isFinished = isFinished;

    if (_isFinished) {
        aboutToFinish();
        
        NodeList* nodeList = NodeList::getInstance();
        
        // if we have a datagram processing thread, quit it and wait on it to make sure that
        // the node socket is back on the same thread as the NodeList
        
        if (_datagramProcessingThread) {
            // tell the datagram processing thread to quit and wait until it is done, then return the node socket to the NodeList
            _datagramProcessingThread->quit();
            _datagramProcessingThread->wait();
            
            // set node socket parent back to NodeList
            nodeList->getNodeSocket().setParent(nodeList);
        }
        
        // move the NodeList back to the QCoreApplication instance's thread
        nodeList->moveToThread(QCoreApplication::instance()->thread());
        
        emit finished();
    }
}

void ThreadedAssignment::commonInit(const QString& targetName, NodeType_t nodeType, bool shouldSendStats) {
    // change the logging target name while the assignment is running
    LogHandler::getInstance().setTargetName(targetName);
    
    NodeList* nodeList = NodeList::getInstance();
    nodeList->setOwnerType(nodeType);
    
    // this is a temp fix for Qt 5.3 - rebinding the node socket gives us readyRead for the socket on this thread
    nodeList->rebindNodeSocket();
    
    QTimer* domainServerTimer = new QTimer(this);
    connect(domainServerTimer, SIGNAL(timeout()), this, SLOT(checkInWithDomainServerOrExit()));
    domainServerTimer->start(DOMAIN_SERVER_CHECK_IN_MSECS);
    
    QTimer* silentNodeRemovalTimer = new QTimer(this);
    connect(silentNodeRemovalTimer, SIGNAL(timeout()), nodeList, SLOT(removeSilentNodes()));
    silentNodeRemovalTimer->start(NODE_SILENCE_THRESHOLD_MSECS);
    
    if (shouldSendStats) {
        // send a stats packet every 1 second
        QTimer* statsTimer = new QTimer(this);
        connect(statsTimer, &QTimer::timeout, this, &ThreadedAssignment::sendStatsPacket);
        statsTimer->start(1000);
    }
}

void ThreadedAssignment::addPacketStatsAndSendStatsPacket(QJsonObject &statsObject) {
    NodeList* nodeList = NodeList::getInstance();
    
    float packetsPerSecond, bytesPerSecond;
    nodeList->getPacketStats(packetsPerSecond, bytesPerSecond);
    nodeList->resetPacketStats();
    
    statsObject["packets_per_second"] = packetsPerSecond;
    statsObject["bytes_per_second"] = bytesPerSecond;
    
    nodeList->sendStatsToDomainServer(statsObject);
}

void ThreadedAssignment::sendStatsPacket() {
    QJsonObject statsObject;
    addPacketStatsAndSendStatsPacket(statsObject);
}

void ThreadedAssignment::checkInWithDomainServerOrExit() {
    if (NodeList::getInstance()->getNumNoReplyDomainCheckIns() == MAX_SILENT_DOMAIN_SERVER_CHECK_INS) {
        setFinished(true);
    } else {
        NodeList::getInstance()->sendDomainServerCheckIn();
    }
}

bool ThreadedAssignment::readAvailableDatagram(QByteArray& destinationByteArray, HifiSockAddr& senderSockAddr) {
    NodeList* nodeList = NodeList::getInstance();
    
    if (nodeList->getNodeSocket().hasPendingDatagrams()) {
        destinationByteArray.resize(nodeList->getNodeSocket().pendingDatagramSize());
        nodeList->getNodeSocket().readDatagram(destinationByteArray.data(), destinationByteArray.size(),
                                               senderSockAddr.getAddressPointer(), senderSockAddr.getPortPointer());
        return true;
    } else {
        return false;
    }
}
