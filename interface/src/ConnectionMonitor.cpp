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

#include <NodeList.h>
#include <DependencyManager.h>
#include <DomainHandler.h>
#include <AddressManager.h>

static const int DISPLAY_AFTER_DISCONNECTED_FOR_X_MS = 5000;

void ConnectionMonitor::init() {
    // Connect to domain disconnected message
    auto nodeList = DependencyManager::get<NodeList>();
    const DomainHandler& domainHandler = nodeList->getDomainHandler();
    connect(&domainHandler, &DomainHandler::disconnectedFromDomain, this, &ConnectionMonitor::disconnectedFromDomain);
    connect(&domainHandler, &DomainHandler::connectedToDomain, this, &ConnectionMonitor::connectedToDomain);

    // Connect to AddressManager::hostChanged
    auto addressManager = DependencyManager::get<AddressManager>();
    connect(addressManager.data(), &AddressManager::hostChanged, this, &ConnectionMonitor::hostChanged);

    _timer.setSingleShot(true);
    _timer.setInterval(DISPLAY_AFTER_DISCONNECTED_FOR_X_MS);
    _timer.start();

    auto dialogsManager = DependencyManager::get<DialogsManager>();
    connect(&_timer, &QTimer::timeout, dialogsManager.data(), &DialogsManager::showAddressBar);
}

void ConnectionMonitor::disconnectedFromDomain() {
    _timer.start();
}

void ConnectionMonitor::connectedToDomain(const QString& name) {
    _timer.stop();
}

void ConnectionMonitor::hostChanged(const QString& name) {
    _timer.start();
}
