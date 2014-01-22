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

#include <QtCore/QMap>
#include <QtCore/QPointer>
#include <QtCore/QUuid>

#include "HifiSockAddr.h"

class DataServerCallbackObject {
public:
    virtual void processDataServerResponse(const QString& userString, const QStringList& keyList, const QStringList& valueList) = 0;
};

class DataServerClient {
public:
    static const HifiSockAddr& dataServerSockAddr();
    
    static void putValueForKeyAndUserString(const QString& key, const QString& value, const QString& userString);
    static void putValueForKeyAndUUID(const QString& key, const QString& value, const QUuid& uuid);
    
    static void getValueForKeyAndUserString(const QString& key, const QString& userString,
                                            DataServerCallbackObject* callbackObject);
    static void getValueForKeyAndUUID(const QString& key, const QUuid& uuid, DataServerCallbackObject* callbackObject);
    static void getValuesForKeysAndUUID(const QStringList& keys, const QUuid& uuid, DataServerCallbackObject* callbackObject);
    static void getValuesForKeysAndUserString(const QStringList& keys, const QString& userString,
                                                    DataServerCallbackObject* callbackObject);
    
    static void processMessageFromDataServer(unsigned char* packetData, int numPacketBytes);
    
    static void resendUnmatchedPackets();
private:
    static void processConfirmFromDataServer(unsigned char* packetData);
    static void processSendFromDataServer(unsigned char* packetData, int numPacketBytes);
    static void removeMatchedPacketFromMap(unsigned char* packetData);
    
    static QMap<quint8, QByteArray> _unmatchedPackets;
    static QMap<quint8, DataServerCallbackObject*> _callbackObjects;
    static quint8 _sequenceNumber;
};

namespace DataServerKey {
    const QString Domain = "domain";
    const QString FaceMeshURL = "mesh";
    const QString SkeletonURL = "skeleton";
    const QString Position = "position";
    const QString Orientation = "orientation";
    const QString UUID = "uuid";
}

#endif /* defined(__hifi__DataServerClient__) */
