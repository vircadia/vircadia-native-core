//
//  PacketReceiver.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 1/23/2014.
//  Update by Ryan Huffman on 7/8/2015.
//  Copyright 2014 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PacketReceiver_h
#define hifi_PacketReceiver_h

#include <vector>
#include <unordered_map>

#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtCore/QSharedPointer>
#include <QtCore/QEnableSharedFromThis>

#include "NLPacket.h"
#include "NLPacketList.h"
#include "ReceivedMessage.h"
#include "udt/PacketHeaders.h"

class EntityEditPacketSender;
class Node;
class OctreePacketProcessor;

namespace std {
    template <>
    struct hash<std::pair<SockAddr, udt::Packet::MessageNumber>> {
        size_t operator()(const std::pair<SockAddr, udt::Packet::MessageNumber>& pair) const {
            return hash<SockAddr>()(pair.first) ^ hash<udt::Packet::MessageNumber>()(pair.second);
        }
    };
}

class PacketReceiver : public QObject {
    Q_OBJECT
public:
    class ListenerReference : public QEnableSharedFromThis<ListenerReference> {
    public:
        virtual bool invokeDirectly(const QSharedPointer<ReceivedMessage>& receivedMessagePointer, const QSharedPointer<Node>& sourceNode) = 0;
        bool invokeWithQt(const QSharedPointer<ReceivedMessage>& receivedMessagePointer, const QSharedPointer<Node>& sourceNode);
        virtual bool isSourced() const = 0;
        virtual QObject* getObject() const = 0;
    };
    typedef QSharedPointer<ListenerReference> ListenerReferencePointer;

    template<class T>
    static ListenerReferencePointer makeUnsourcedListenerReference(T* target, void (T::*slot)(QSharedPointer<ReceivedMessage>));

    template <class T>
    static ListenerReferencePointer makeSourcedListenerReference(T* target, void (T::*slot)(QSharedPointer<ReceivedMessage>, QSharedPointer<Node>));

public:
    using PacketTypeList = std::vector<PacketType>;
    
    PacketReceiver(QObject* parent = 0);
    PacketReceiver(const PacketReceiver&) = delete;

    PacketReceiver& operator=(const PacketReceiver&) = delete;

    void setShouldDropPackets(bool shouldDropPackets) { _shouldDropPackets = shouldDropPackets; }

    // If deliverPending is false, ReceivedMessage will only be delivered once all packets for the message have
    // been received. If deliverPending is true, ReceivedMessage will be delivered as soon as the first packet
    // for the message is received.
    bool registerListener(PacketType type, const ListenerReferencePointer& listener, bool deliverPending = false);
    bool registerListenerForTypes(PacketTypeList types, const ListenerReferencePointer& listener);
    void unregisterListener(QObject* listener);
    
    void handleVerifiedPacket(std::unique_ptr<udt::Packet> packet);
    void handleVerifiedMessagePacket(std::unique_ptr<udt::Packet> message);
    void handleMessageFailure(SockAddr from, udt::Packet::MessageNumber messageNumber);
    
private:
    template <class T>
    class UnsourcedListenerReference : public ListenerReference {
    public:
        inline UnsourcedListenerReference(T* target, void (T::*slot)(QSharedPointer<ReceivedMessage>));
        virtual bool invokeDirectly(const QSharedPointer<ReceivedMessage>& receivedMessagePointer, const QSharedPointer<Node>& sourceNode) override;
        virtual bool isSourced() const override { return false; }
        virtual QObject* getObject() const override { return _target; }

    private:
        QPointer<T> _target;
        void (T::*_slot)(QSharedPointer<ReceivedMessage>);
    };

    template <class T>
    class SourcedListenerReference : public ListenerReference {
    public:
        inline SourcedListenerReference(T* target, void (T::*slot)(QSharedPointer<ReceivedMessage>, QSharedPointer<Node>));
        virtual bool invokeDirectly(const QSharedPointer<ReceivedMessage>& receivedMessagePointer, const QSharedPointer<Node>& sourceNode) override;
        virtual bool isSourced() const override { return true; }
        virtual QObject* getObject() const override { return _target; }

    private:
        QPointer<T> _target;
        void (T::*_slot)(QSharedPointer<ReceivedMessage>, QSharedPointer<Node>);
    };

    struct Listener {
        ListenerReferencePointer listener;
        bool deliverPending;
    };

    void handleVerifiedMessage(QSharedPointer<ReceivedMessage> message, bool justReceived);

    // these are brutal hacks for now - ideally GenericThread / ReceivedPacketProcessor
    // should be changed to have a true event loop and be able to handle our QMetaMethod::invoke
    void registerDirectListenerForTypes(PacketTypeList types, const ListenerReferencePointer& listener);
    void registerDirectListener(PacketType type, const ListenerReferencePointer& listener);

    bool matchingMethodForListener(PacketType type, const ListenerReferencePointer& listener) const;
    void registerVerifiedListener(PacketType type, const ListenerReferencePointer& listener, bool deliverPending = false);

    QMutex _packetListenerLock;
    QHash<PacketType, Listener> _messageListenerMap;

    bool _shouldDropPackets = false;
    QMutex _directConnectSetMutex;
    QSet<QObject*> _directlyConnectedObjects;

    std::unordered_map<std::pair<SockAddr, udt::Packet::MessageNumber>, QSharedPointer<ReceivedMessage>> _pendingMessages;
    
    friend class EntityEditPacketSender;
    friend class OctreePacketProcessor;
};

template <class T>
PacketReceiver::ListenerReferencePointer PacketReceiver::makeUnsourcedListenerReference(T* target, void (T::* slot)(QSharedPointer<ReceivedMessage>)) {
    return QSharedPointer<UnsourcedListenerReference<T>>::create(target, slot);
}

template <class T>
PacketReceiver::ListenerReferencePointer PacketReceiver::makeSourcedListenerReference(T* target, void (T::* slot)(QSharedPointer<ReceivedMessage>, QSharedPointer<Node>)) {
    return QSharedPointer<SourcedListenerReference<T>>::create(target, slot);
}

template <class T>
PacketReceiver::UnsourcedListenerReference<T>::UnsourcedListenerReference(T* target, void (T::*slot)(QSharedPointer<ReceivedMessage>)) :
    _target(target),_slot(slot) {
}

template <class T>
bool PacketReceiver::UnsourcedListenerReference<T>::invokeDirectly(const QSharedPointer<ReceivedMessage>& receivedMessagePointer, const QSharedPointer<Node>&) {
    if (_target.isNull()) {
        return false;
    }
    (_target->*_slot)(receivedMessagePointer);
    return true;
}

template <class T>
PacketReceiver::SourcedListenerReference<T>::SourcedListenerReference(T* target, void (T::*slot)(QSharedPointer<ReceivedMessage>, QSharedPointer<Node>)) :
    _target(target),_slot(slot) {
}

template <class T>
bool PacketReceiver::SourcedListenerReference<T>::invokeDirectly(const QSharedPointer<ReceivedMessage>& receivedMessagePointer, const QSharedPointer<Node>& sourceNode) {
    if (_target.isNull()) {
        return false;
    }
    (_target->*_slot)(receivedMessagePointer, sourceNode);
    return true;
}

#endif // hifi_PacketReceiver_h
