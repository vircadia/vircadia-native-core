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

#include "Application.h"
#include "ui/DialogsManager.h"

#include <DependencyManager.h>
#include <DomainHandler.h>
#include <AddressManager.h>
#include <NodeList.h>

// Because the connection monitor is created at startup, the time we wait on initial load
// should be longer to allow the application to initialize.
static const int ON_INITIAL_LOAD_REDIRECT_AFTER_DISCONNECTED_FOR_X_MS = 10000;
static const int REDIRECT_AFTER_DISCONNECTED_FOR_X_MS = 5000;

void ConnectionMonitor::init() {
    // Connect to domain disconnected message
    auto nodeList = DependencyManager::get<NodeList>();
    const DomainHandler& domainHandler = nodeList->getDomainHandler();
    connect(&domainHandler, &DomainHandler::resetting, this, &ConnectionMonitor::startTimer);
    connect(&domainHandler, &DomainHandler::disconnectedFromDomain, this, &ConnectionMonitor::startTimer);
    connect(&domainHandler, &DomainHandler::connectedToDomain, this, &ConnectionMonitor::stopTimer);
    connect(&domainHandler, &DomainHandler::domainConnectionRefused, this, &ConnectionMonitor::stopTimer);
    connect(&domainHandler, &DomainHandler::redirectToErrorDomainURL, this, &ConnectionMonitor::stopTimer);
    connect(this, &ConnectionMonitor::setRedirectErrorState, &domainHandler, &DomainHandler::setRedirectErrorState);

    _timer.setSingleShot(true);
    if (!domainHandler.isConnected()) {
        _timer.start(ON_INITIAL_LOAD_REDIRECT_AFTER_DISCONNECTED_FOR_X_MS);
    }

    connect(&_timer, &QTimer::timeout, this, [this]() {
        qDebug() << "ConnectionMonitor: Redirecting to 404 error domain";
        // set in a timeout error
        emit setRedirectErrorState(REDIRECT_HIFI_ADDRESS, 5);
    });
}

void ConnectionMonitor::startTimer() {
    _timer.start(REDIRECT_AFTER_DISCONNECTED_FOR_X_MS);
}

void ConnectionMonitor::stopTimer() {
    _timer.stop();
}
