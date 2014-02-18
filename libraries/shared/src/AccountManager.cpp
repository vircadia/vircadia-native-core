//
//  AccountManager.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QDataStream>

#include "PacketHeaders.h"

#include "AccountManager.h"

QString AccountManager::_username = "";

void AccountManager::processDomainServerAuthRequest(const QByteArray& packet) {
    QDataStream authPacketStream(packet);
    authPacketStream.skipRawData(numBytesForPacketHeader(packet));
    
    // grab the hostname this domain-server wants us to authenticate with
    QString authenticationHostname;
    authPacketStream >> authenticationHostname;
    
    // check if we already have an access token associated with that hostname
    
}