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
        if (_domainServerTimer) {
            _domainServerTimer->stop();
            delete _domainServerTimer;
            _domainServerTimer = nullptr;
        }
        if (_statsTimer) {
            _statsTimer->stop();
            delete _statsTimer;
            _statsTimer = nullptr;
        }

        aboutToFinish();
        
        auto nodeList = DependencyManager::get<NodeList>();
        
        // if we have a datagram processing thread, quit it and wait on it to make sure that
        // the node socket is back on the same thread as the NodeList
        
        if (_datagramProcessingThread) {
            // tell the datagram processing thread to quit and wait until it is done, then return the node socket to the NodeList
            _datagramProcessingThread->quit();
            _datagramProcessingThread->wait();
            
            // set node socket parent back to NodeList
            nodeList->getNodeSocket().setParent(nodeList.data());
        }
        
        // move the NodeList back to the QCoreApplication instance's thread
        nodeList->moveToThread(QCoreApplication::instance()->thread());
        
        emit finished();
    }
}

void ThreadedAssignment::commonInit(const QString& targetName, NodeType_t nodeType, bool shouldSendStats) {
    // change the logging target name while the assignment is running
    LogHandler::getInstance().setTargetName(targetName);
    
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->setOwnerType(nodeType);
    
    // this is a temp fix for Qt 5.3 - rebinding the node socket gives us readyRead for the socket on this thread
    nodeList->rebindNodeSocket();
    
    _domainServerTimer = new QTimer();
    connect(_domainServerTimer, SIGNAL(timeout()), this, SLOT(checkInWithDomainServerOrExit()));
    _domainServerTimer->start(DOMAIN_SERVER_CHECK_IN_MSECS);
    
    if (shouldSendStats) {
        // send a stats packet every 1 second
        _statsTimer = new QTimer();
        connect(_statsTimer, &QTimer::timeout, this, &ThreadedAssignment::sendStatsPacket);
        _statsTimer->start(1000);
    }
}

void ThreadedAssignment::addPacketStatsAndSendStatsPacket(QJsonObject &statsObject) {
    auto nodeList = DependencyManager::get<NodeList>();
    
    float packetsPerSecond, bytesPerSecond;
    // XXX can BandwidthRecorder be used for this?
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
    if (DependencyManager::get<NodeList>()->getNumNoReplyDomainCheckIns() == MAX_SILENT_DOMAIN_SERVER_CHECK_INS) {
        setFinished(true);
    } else {
        DependencyManager::get<NodeList>()->sendDomainServerCheckIn();
    }
}

bool ThreadedAssignment::readAvailableDatagram(QByteArray& destinationByteArray, HifiSockAddr& senderSockAddr) {
    auto nodeList = DependencyManager::get<NodeList>();
    
    if (nodeList->getNodeSocket().hasPendingDatagrams()) {
        destinationByteArray.resize(nodeList->getNodeSocket().pendingDatagramSize());
        nodeList->getNodeSocket().readDatagram(destinationByteArray.data(), destinationByteArray.size(),
                                               senderSockAddr.getAddressPointer(), senderSockAddr.getPortPointer());
        return true;
    } else {
        return false;
    }
}
