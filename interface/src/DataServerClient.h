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
    static const HifiSockAddr& dataServerSockAddr();
    static void putValueForKey(const QString& key, const char* value);
    static void getClientValueForKey(const QString& key);
    
    static void getValueForKeyAndUUID(const QString& key, const QUuid& uuid);
    static void getValuesForKeysAndUUID(const QStringList& keys, const QUuid& uuid);
    static void getValuesForKeysAndUserString(const QStringList& keys, const QString& userString);
    
    static void processConfirmFromDataServer(unsigned char* packetData, int numPacketBytes);
    static void processSendFromDataServer(unsigned char* packetData, int numPacketBytes);
    static void processMessageFromDataServer(unsigned char* packetData, int numPacketBytes);
    static void removeMatchedPacketFromMap(unsigned char* packetData, int numPacketBytes);
    static void resendUnmatchedPackets();
private:
    
    
    static std::map<unsigned char*, int> _unmatchedPackets;
};

namespace DataServerKey {
    const QString Domain = "domain";
    const QString FaceMeshURL = "mesh";
    const QString SkeletonURL = "skeleton";
    const QString Position = "position";
    const QString UUID = "uuid";
}

#endif /* defined(__hifi__DataServerClient__) */
