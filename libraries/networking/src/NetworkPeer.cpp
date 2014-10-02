//
//  NetworkPeer.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2014-10-02.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <UUID.h>

#include "NetworkPeer.h"

NetworkPeer::NetworkPeer(const QUuid& uuid, const HifiSockAddr& publicSocket, const HifiSockAddr& localSocket) :
    _uuid(uuid),
    _publicSocket(publicSocket),
    _localSocket(localSocket),
    _symmetricSocket(),
    _activeSocket(NULL)
{
    
}

void NetworkPeer::setPublicSocket(const HifiSockAddr& publicSocket) {
    if (_activeSocket == &_publicSocket) {
        // if the active socket was the public socket then reset it to NULL
        _activeSocket = NULL;
    }
    
    _publicSocket = publicSocket;
}

void NetworkPeer::setLocalSocket(const HifiSockAddr& localSocket) {
    if (_activeSocket == &_localSocket) {
        // if the active socket was the local socket then reset it to NULL
        _activeSocket = NULL;
    }
    
    _localSocket = localSocket;
}

void NetworkPeer::setSymmetricSocket(const HifiSockAddr& symmetricSocket) {
    if (_activeSocket == &_symmetricSocket) {
        // if the active socket was the symmetric socket then reset it to NULL
        _activeSocket = NULL;
    }
    
    _symmetricSocket = symmetricSocket;
}

void NetworkPeer::activateLocalSocket() {
    qDebug() << "Activating local socket for network peer with ID" << uuidStringWithoutCurlyBraces(_uuid);
    _activeSocket = &_localSocket;
}

void NetworkPeer::activatePublicSocket() {
    qDebug() << "Activating public socket for network peer with ID" << uuidStringWithoutCurlyBraces(_uuid);
    _activeSocket = &_publicSocket;
}

void NetworkPeer::activateSymmetricSocket() {
    qDebug() << "Activating symmetric socket for network peer with ID" << uuidStringWithoutCurlyBraces(_uuid);
    _activeSocket = &_symmetricSocket;
}

QDebug operator<<(QDebug debug, const NetworkPeer &peer) {
    debug << uuidStringWithoutCurlyBraces(peer.getUUID())
        << "- public:" << peer.getPublicSocket()
        << "- local:" << peer.getLocalSocket();
    return debug;
}