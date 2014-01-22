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
    //printf("OctreeScriptingInterface::~OctreeScriptingInterface()\n");
    if (_managedJurisdictionListener) {
        //printf("OctreeScriptingInterface::~OctreeScriptingInterface() _managedJurisdictionListener... _jurisdictionListener->terminate()\n");
        _jurisdictionListener->terminate();
        //printf("OctreeScriptingInterface::~OctreeScriptingInterface() _managedJurisdictionListener... deleting _jurisdictionListener\n");
        delete _jurisdictionListener;
    }
    if (_managedPacketSender) {
        //printf("OctreeScriptingInterface::~OctreeScriptingInterface() _managedJurisdictionListener... _packetSender->terminate()\n");
        _packetSender->terminate();
        //printf("OctreeScriptingInterface::~OctreeScriptingInterface() _managedPacketSender... deleting _packetSender\n");
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
    //printf("OctreeScriptingInterface::init()\n");
    if (_jurisdictionListener) {
        _managedJurisdictionListener = false;
    } else {
        _managedJurisdictionListener = true;
        _jurisdictionListener = new JurisdictionListener(getServerNodeType());
        qDebug("OctreeScriptingInterface::init() _managedJurisdictionListener=true, creating _jurisdictionListener=%p", _jurisdictionListener);
        _jurisdictionListener->initialize(true);
    }
    
    if (_packetSender) {
        _managedPacketSender = false;
    } else {
        _managedPacketSender = true;
        _packetSender = createPacketSender();
        printf("OctreeScriptingInterface::init() _managedPacketSender=true, creating _packetSender=%p", _packetSender);
        _packetSender->setServerJurisdictions(_jurisdictionListener->getJurisdictions());
    }
}
