//
//  DataServerClient.h
//  hifi
//
//  Created by Stephen Birarda on 10/7/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__DataServerClient__
#define __hifi__DataServerClient__

#include <vector>

#include <QtCore/QUuid>

class DataServerClient {
public:
    static void putValueForKey(const char* key, const char* value);
    static void getValueForKeyAndUUID(const char* key, QUuid& uuid);
    static void getClientValueForKey(const char* key) { getValueForKeyAndUUID(key, _clientUUID); }
    static void processConfirmFromDataServer(unsigned char* packetData, int numPacketBytes);
    static void processGetFromDataServer(unsigned char* packetData, int numPacketBytes);
    
    static void setClientUUID(QUuid& clientUUID) { _clientUUID = clientUUID; }
    static QUuid& getClientUUID() { return _clientUUID; }
private:
    static QUuid _clientUUID;
    static std::vector<unsigned char*> _unconfirmedPackets;
};

#endif /* defined(__hifi__DataServerClient__) */
