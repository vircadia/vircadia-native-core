//
//  OctreeScriptingInterface.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/6/13
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <QCoreApplication>

#include "OctreeScriptingInterface.h"

OctreeScriptingInterface::OctreeScriptingInterface(OctreeEditPacketSender* packetSender, 
                JurisdictionListener* jurisdictionListener) :
    _packetSender(packetSender),
    _jurisdictionListener(jurisdictionListener),
    _managedPacketSender(false), 
    _managedJurisdictionListener(false),
    _initialized(false)
{
}

OctreeScriptingInterface::~OctreeScriptingInterface() {
    cleanupManagedObjects();
}

void OctreeScriptingInterface::cleanupManagedObjects() {
    if (_managedJurisdictionListener) {
        _jurisdictionListener->terminate();
        _jurisdictionListener->deleteLater();
        _managedJurisdictionListener = false;
        _jurisdictionListener = NULL;
    }
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

void OctreeScriptingInterface::setJurisdictionListener(JurisdictionListener* jurisdictionListener) {
    _jurisdictionListener = jurisdictionListener;
}

void OctreeScriptingInterface::init() {
    if (_initialized) {
        return;
    }
    if (_jurisdictionListener) {
        _managedJurisdictionListener = false;
    } else {
        _managedJurisdictionListener = true;
        _jurisdictionListener = new JurisdictionListener(getServerNodeType());
        _jurisdictionListener->initialize(true);
    }
    
    if (_packetSender) {
        _managedPacketSender = false;
    } else {
        _managedPacketSender = true;
        _packetSender = createPacketSender();
        _packetSender->setServerJurisdictions(_jurisdictionListener->getJurisdictions());
    }

    if (QCoreApplication::instance()) {
        connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(cleanupManagedObjects()));
    }
    _initialized = true;
}
