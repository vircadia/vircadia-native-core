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

#include "ThreadedAssignment.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <LogHandler.h>
#include <shared/QtHelpers.h>

#include <platform/Platform.h>
#include "NetworkLogging.h"

ThreadedAssignment::ThreadedAssignment(ReceivedMessage& message) :
    Assignment(message),
    _isFinished(false),
    _domainServerTimer(this),
    _statsTimer(this)
{
    // use <mixer-type> as a temporary targetName name until commonInit can be called later
    LogHandler::getInstance().setTargetName(QString("<%1>").arg(getTypeName()));

    static const int STATS_TIMEOUT_MS = 1000;
    _statsTimer.setInterval(STATS_TIMEOUT_MS); // 1s, Qt::CoarseTimer acceptable
    connect(&_statsTimer, &QTimer::timeout, this, &ThreadedAssignment::sendStatsPacket);

    connect(&_domainServerTimer, &QTimer::timeout, this, &ThreadedAssignment::checkInWithDomainServerOrExit);
    _domainServerTimer.setInterval(DOMAIN_SERVER_CHECK_IN_MSECS); // 1s, Qt::CoarseTimer acceptable

    // if the NL tells us we got a DS response, clear our member variable of queued check-ins
    auto nodeList = DependencyManager::get<NodeList>();
    connect(nodeList.data(), &NodeList::receivedDomainServerList, this, &ThreadedAssignment::clearQueuedCheckIns);

    platform::create();
    if (!platform::enumeratePlatform()) {
        qCDebug(networking) << "Failed to enumerate platform.";
    }
}

ThreadedAssignment::~ThreadedAssignment() {
    stop();
    platform::destroy();
}

void ThreadedAssignment::setFinished(bool isFinished) {
    if (_isFinished != isFinished) {
         _isFinished = isFinished;

        if (_isFinished) {

            qCDebug(networking) << "ThreadedAssignment::setFinished(true) called - finishing up.";

            auto nodeList = DependencyManager::get<NodeList>();

            auto& packetReceiver = nodeList->getPacketReceiver();

            // we should de-register immediately for any of our packets
            packetReceiver.unregisterListener(this);

            // we should also tell the packet receiver to drop packets while we're cleaning up
            packetReceiver.setShouldDropPackets(true);

            // send a disconnect packet to the domain
            nodeList->getDomainHandler().disconnect("Finished");

            // stop our owned timers
            _domainServerTimer.stop();
            _statsTimer.stop();

            // call our virtual aboutToFinish method - this gives the ThreadedAssignment subclass a chance to cleanup
            aboutToFinish();

            emit finished();
        }
    }
}

void ThreadedAssignment::commonInit(const QString& targetName, NodeType_t nodeType) {
    // change the logging target name while the assignment is running
    LogHandler::getInstance().setTargetName(targetName);

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->setOwnerType(nodeType);

    // send a domain-server check in immediately and start the timer to fire them every DOMAIN_SERVER_CHECK_IN_MSECS
    checkInWithDomainServerOrExit();
    _domainServerTimer.start();

    // start sending stats packet once we connect to the domain
    connect(&nodeList->getDomainHandler(), &DomainHandler::connectedToDomain,
            &_statsTimer, static_cast<void (QTimer::*)()>(&QTimer::start));

    // stop sending stats if we disconnect
    connect(&nodeList->getDomainHandler(), &DomainHandler::disconnectedFromDomain, &_statsTimer, &QTimer::stop);
}

void ThreadedAssignment::addPacketStatsAndSendStatsPacket(QJsonObject statsObject) {
    auto nodeList = DependencyManager::get<NodeList>();

#ifdef DEBUG_EVENT_QUEUE
    statsObject["nodelist_event_queue_size"] = ::hifi::qt::getEventQueueSize(nodeList->thread());
#endif

    QJsonObject ioStats;
    ioStats["inbound_kbps"] = nodeList->getInboundKbps();
    ioStats["inbound_pps"] = nodeList->getInboundPPS();
    ioStats["outbound_kbps"] = nodeList->getOutboundKbps();
    ioStats["outbound_pps"] = nodeList->getOutboundPPS();

    statsObject["io_stats"] = ioStats;

    QJsonObject assignmentStats;
    assignmentStats["numQueuedCheckIns"] = _numQueuedCheckIns;

    statsObject["assignmentStats"] = assignmentStats;

    nodeList->sendStatsToDomainServer(statsObject);
}

void ThreadedAssignment::sendStatsPacket() {
    QJsonObject statsObject;
    addPacketStatsAndSendStatsPacket(statsObject);
}

void ThreadedAssignment::checkInWithDomainServerOrExit() {
    // verify that the number of queued check-ins is not >= our max
    // the number of queued check-ins is cleared anytime we get a response from the domain-server
    if (_numQueuedCheckIns >= MAX_SILENT_DOMAIN_SERVER_CHECK_INS) {
        qCDebug(networking) << "At least" << MAX_SILENT_DOMAIN_SERVER_CHECK_INS << "have been queued without a response from domain-server"
            << "Stopping the current assignment";
        stop();
    } else {
        auto nodeList = DependencyManager::get<NodeList>();
        // Call sendDomainServerCheckIn directly instead of putting it on
        // the event queue.  Under high load, the event queue can back up
        // longer than the total timeout period and cause a restart
        nodeList->sendDomainServerCheckIn();

        // increase the number of queued check ins
        _numQueuedCheckIns++;
        if (_numQueuedCheckIns > 1) {
            qCDebug(networking) << "Number of queued checkins = " << _numQueuedCheckIns;
        }
    }
}

void ThreadedAssignment::domainSettingsRequestFailed() {
    qCDebug(networking) << "Failed to retreive settings object from domain-server. Bailing on assignment.";
    stop();
}
