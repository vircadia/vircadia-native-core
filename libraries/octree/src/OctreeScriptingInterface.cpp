//
//  OctreeScriptingInterface.cpp
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QCoreApplication>

#include "OctreeScriptingInterface.h"

OctreeScriptingInterface::OctreeScriptingInterface(OctreeEditPacketSender* packetSender) :
    _packetSender(packetSender),
    _managedPacketSender(false),
    _initialized(false)
{
}

OctreeScriptingInterface::~OctreeScriptingInterface() {
    cleanupManagedObjects();
}

void OctreeScriptingInterface::cleanupManagedObjects() {
    if (_managedPacketSender) {
        _packetSender->terminate();
        _packetSender->deleteLater();
        _managedPacketSender = false;
        _packetSender = NULL;
    }
}

void OctreeScriptingInterface::setPacketSender(OctreeEditPacketSender* packetSender) {
    _packetSender = packetSender;
}

void OctreeScriptingInterface::init() {
    if (_initialized) {
        return;
    }
    
    if (_packetSender) {
        _managedPacketSender = false;
    } else {
        _managedPacketSender = true;
        _packetSender = createPacketSender();
    }

    if (QCoreApplication::instance()) {
        connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(cleanupManagedObjects()));
    }
    _initialized = true;
}
