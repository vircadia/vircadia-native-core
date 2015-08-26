//
//  PacketReceiver.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 1/23/2014.
//  Update by Ryan Huffman on 7/8/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PacketReceiver_h
#define hifi_PacketReceiver_h

#include <vector>

#include <QtCore/QMap>
#include <QtCore/QMetaMethod>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSet>

#include "NLPacket.h"
#include "NLPacketList.h"
#include "udt/PacketHeaders.h"

class EntityEditPacketSender;
class OctreePacketProcessor;

class PacketReceiver : public QObject {
    Q_OBJECT
public:
    using PacketTypeList = std::vector<PacketType>;
    
    PacketReceiver(QObject* parent = 0);
    PacketReceiver(const PacketReceiver&) = delete;

    PacketReceiver& operator=(const PacketReceiver&) = delete;
    
    int getInPacketCount() const { return _inPacketCount; }
    int getInByteCount() const { return _inByteCount; }

    void setShouldDropPackets(bool shouldDropPackets) { _shouldDropPackets = shouldDropPackets; }
    
    void resetCounters() { _inPacketCount = 0; _inByteCount = 0; }

    bool registerListenerForTypes(PacketTypeList types, QObject* listener, const char* slot);
    bool registerMessageListener(PacketType type, QObject* listener, const char* slot);
    bool registerListener(PacketType type, QObject* listener, const char* slot);
    void unregisterListener(QObject* listener);
    
    void handleVerifiedPacket(std::unique_ptr<udt::Packet> packet);
    void handleVerifiedPacketList(std::unique_ptr<udt::PacketList> packetList);

signals:
    void dataReceived(quint8 channelType, int bytes);
    
private:
    // these are brutal hacks for now - ideally GenericThread / ReceivedPacketProcessor
    // should be changed to have a true event loop and be able to handle our QMetaMethod::invoke
    void registerDirectListenerForTypes(PacketTypeList types, QObject* listener, const char* slot);
    void registerDirectListener(PacketType type, QObject* listener, const char* slot);

    QMetaMethod matchingMethodForListener(PacketType type, QObject* object, const char* slot) const;
    void registerVerifiedListener(PacketType type, QObject* listener, const QMetaMethod& slot);
    
    using ObjectMethodPair = std::pair<QPointer<QObject>, QMetaMethod>;

    QMutex _packetListenerLock;
    // TODO: replace the two following hashes with an std::vector once we switch Packet/PacketList to Message
    QHash<PacketType, ObjectMethodPair> _packetListenerMap;
    QHash<PacketType, ObjectMethodPair> _packetListListenerMap;
    int _inPacketCount = 0;
    int _inByteCount = 0;
    bool _shouldDropPackets = false;
    QMutex _directConnectSetMutex;
    QSet<QObject*> _directlyConnectedObjects;
    
    friend class EntityEditPacketSender;
    friend class OctreePacketProcessor;
};

#endif // hifi_PacketReceiver_h
