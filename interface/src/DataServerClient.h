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
    static void getValueForKeyAndUserString(const char* key, QString& userString);
    static void getClientValueForKey(const char* key) { getValueForKeyAndUserString(key, _clientUsername); }
    static void processConfirmFromDataServer(unsigned char* packetData, int numPacketBytes);
    static void processSendFromDataServer(unsigned char* packetData, int numPacketBytes);
    static void processMessageFromDataServer(unsigned char* packetData, int numPacketBytes);
    
    static void setClientUsername(const QString& clientUsername) { _clientUsername = clientUsername; }
    static QString& setClientUsername() { return _clientUsername; }
private:
    static QString _clientUsername;
    static std::vector<unsigned char*> _unmatchedPackets;
};

namespace DataServerKey {
    const char FaceMeshURL[] = "mesh";
}

#endif /* defined(__hifi__DataServerClient__) */
