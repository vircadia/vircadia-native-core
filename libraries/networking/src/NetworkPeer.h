//
//  NetworkPeer.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2014-10-02.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_NetworkPeer_h
#define hifi_NetworkPeer_h

#include <qobject.h>
#include <quuid.h>

#include "HifiSockAddr.h"

class NetworkPeer : public QObject {
public:
    NetworkPeer(const QUuid& uuid, const HifiSockAddr& publicSocket, const HifiSockAddr& localSocket);
    
    const HifiSockAddr& getPublicSocket() const { return _publicSocket; }
    void setPublicSocket(const HifiSockAddr& publicSocket);
    const HifiSockAddr& getLocalSocket() const { return _localSocket; }
    void setLocalSocket(const HifiSockAddr& localSocket);
    const HifiSockAddr& getSymmetricSocket() const { return _symmetricSocket; }
    void setSymmetricSocket(const HifiSockAddr& symmetricSocket);
    
    const HifiSockAddr* getActiveSocket() const { return _activeSocket; }
    
    void activatePublicSocket();
    void activateLocalSocket();
    void activateSymmetricSocket();
    
protected:
    QUuid _uuid;
    
    HifiSockAddr _publicSocket;
    HifiSockAddr _localSocket;
    HifiSockAddr _symmetricSocket;
    HifiSockAddr* _activeSocket;
};

#endif // hifi_NetworkPeer_h