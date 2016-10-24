//
//  ConnectionMonitor.cpp
//  interface/src
//
//  Created by Ryan Huffman on 8/4/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ConnectionMonitor.h"

#include "ui/DialogsManager.h"

#include <DependencyManager.h>
#include <DomainHandler.h>
#include <NodeList.h>

// Because the connection monitor is created at startup, the time we wait on initial load
// should be longer to allow the application to initialize.
static const int ON_INITIAL_LOAD_DISPLAY_AFTER_DISCONNECTED_FOR_X_MS = 10000;
static const int DISPLAY_AFTER_DISCONNECTED_FOR_X_MS = 5000;

void ConnectionMonitor::init() {
    // Connect to domain disconnected message
    auto nodeList = DependencyManager::get<NodeList>();
    const DomainHandler& domainHandler = nodeList->getDomainHandler();
    connect(&domainHandler, &DomainHandler::resetting, this, &ConnectionMonitor::startTimer);
    connect(&domainHandler, &DomainHandler::disconnectedFromDomain, this, &ConnectionMonitor::startTimer);
    connect(&domainHandler, &DomainHandler::connectedToDomain, this, &ConnectionMonitor::stopTimer);
    connect(&domainHandler, &DomainHandler::authRequired, this, &ConnectionMonitor::stopTimer);
    connect(&domainHandler, &DomainHandler::domainConnectionRefused, this, &ConnectionMonitor::stopTimer);

    _timer.setSingleShot(true);
    _timer.setInterval(ON_INITIAL_LOAD_DISPLAY_AFTER_DISCONNECTED_FOR_X_MS);
    if (!domainHandler.isConnected()) {
        _timer.start();
    }

    connect(&_timer, &QTimer::timeout, this, []() {
        qDebug() << "ConnectionMonitor: Showing connection failure window";
        DependencyManager::get<DialogsManager>()->setDomainConnectionFailureVisibility(true);
    });
}

void ConnectionMonitor::startTimer() {
    qDebug() << "ConnectionMonitor: Starting timer";
    _timer.setInterval(DISPLAY_AFTER_DISCONNECTED_FOR_X_MS);
    _timer.start();
}

void ConnectionMonitor::stopTimer() {
    qDebug() << "ConnectionMonitor: Stopping timer";
    _timer.stop();
    DependencyManager::get<DialogsManager>()->setDomainConnectionFailureVisibility(false);
}
