//
//  OctreeScriptingInterface.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/6/13
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "OctreeScriptingInterface.h"

OctreeScriptingInterface::OctreeScriptingInterface(OctreeEditPacketSender* packetSender, 
                JurisdictionListener* jurisdictionListener)
{
    setPacketSender(packetSender);
    setJurisdictionListener(jurisdictionListener);
}

OctreeScriptingInterface::~OctreeScriptingInterface() {
    if (_managedJuridiciontListerner) {
        delete _jurisdictionListener;
    }
    if (_managedPacketSender) {
        delete _packetSender;
    }
}

void OctreeScriptingInterface::setPacketSender(OctreeEditPacketSender* packetSender) {
    _packetSender = packetSender;
}

void OctreeScriptingInterface::setJurisdictionListener(JurisdictionListener* jurisdictionListener) {
    _jurisdictionListener = jurisdictionListener;
}

void OctreeScriptingInterface::init() {
    if (_jurisdictionListener) {
        _managedJuridiciontListerner = false;
    } else {
        _managedJuridiciontListerner = true;
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
}
