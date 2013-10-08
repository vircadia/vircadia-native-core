//
//  DataServerClient.h
//  hifi
//
//  Created by Stephen Birarda on 10/7/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__DataServerClient__
#define __hifi__DataServerClient__

#include <map>

#include <QtCore/QUuid>

#include "Application.h"

class DataServerClient {
public:
    static void putValueForKey(const char* key, const char* value);
    static void getValueForKeyAndUUID(const char* key, const QUuid& uuid);
    static void getValueForKeyAndUserString(const char* key, const QString& userString);
    static void getClientValueForKey(const char* key);
    static void processConfirmFromDataServer(unsigned char* packetData, int numPacketBytes);
    static void processSendFromDataServer(unsigned char* packetData, int numPacketBytes);
    static void processMessageFromDataServer(unsigned char* packetData, int numPacketBytes);
    static void removeMatchedPacketFromMap(unsigned char* packetData, int numPacketBytes);
    static void resendUnmatchedPackets();
private:
    static std::map<unsigned char*, int> _unmatchedPackets;
};

namespace DataServerKey {
    const char FaceMeshURL[] = "mesh";
    const char UUID[] = "uuid";
}

#endif /* defined(__hifi__DataServerClient__) */
