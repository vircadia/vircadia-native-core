//
//  PacketReceiver.cpp
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

#include "PacketReceiver.h"

#include <QtCore/QMetaObject>
#include <QtCore/QMutexLocker>

#include "DependencyManager.h"
#include "NetworkLogging.h"
#include "NodeList.h"
#include "SharedUtil.h"

PacketReceiver::PacketReceiver(QObject* parent) : QObject(parent) {
    qRegisterMetaType<QSharedPointer<NLPacket>>();
    qRegisterMetaType<QSharedPointer<NLPacketList>>();
    qRegisterMetaType<QSharedPointer<ReceivedMessage>>();
}

bool PacketReceiver::ListenerReference::invokeWithQt(const QSharedPointer<ReceivedMessage>& receivedMessagePointer, const QSharedPointer<Node>& sourceNode) {
    ListenerReferencePointer thisPointer = sharedFromThis();
    return QMetaObject::invokeMethod(getObject(), [=]() {
        thisPointer->invokeDirectly(receivedMessagePointer, sourceNode);
    });
}

bool PacketReceiver::registerListenerForTypes(PacketTypeList types, const ListenerReferencePointer& listener) {
    Q_ASSERT_X(!types.empty(), "PacketReceiver::registerListenerForTypes", "No types to register");
    Q_ASSERT_X(listener, "PacketReceiver::registerListenerForTypes", "No listener to register");
    
    std::for_each(std::begin(types), std::end(types), [this, &listener](PacketType type) {
        registerVerifiedListener(type, listener);
    });
    
    return true;
}

void PacketReceiver::registerDirectListener(PacketType type, const ListenerReferencePointer& listener) {
    Q_ASSERT_X(listener, "PacketReceiver::registerDirectListener", "No listener to register");
    
    bool success = registerListener(type, listener);
    if (success) {
        QMutexLocker locker(&_directConnectSetMutex);
        
        // if we successfully registered, add this object to the set of objects that are directly connected
        _directlyConnectedObjects.insert(listener->getObject());
    }
}

void PacketReceiver::registerDirectListenerForTypes(PacketTypeList types, const ListenerReferencePointer& listener) {
    Q_ASSERT_X(listener, "PacketReceiver::registerDirectListenerForTypes", "No listener to register");
    
    // just call register listener for types to start
    bool success = registerListenerForTypes(std::move(types), listener);
    if (success) {
        QMutexLocker locker(&_directConnectSetMutex);
        
        // if we successfully registered, add this object to the set of objects that are directly connected
        _directlyConnectedObjects.insert(listener->getObject());
    }
}

bool PacketReceiver::registerListener(PacketType type, const ListenerReferencePointer& listener,  bool deliverPending) {
    Q_ASSERT_X(listener, "PacketReceiver::registerListener", "No listener to register");

    bool matchingMethod = matchingMethodForListener(type, listener);

    if (matchingMethod) {
        qCDebug(networking) << "Registering a packet listener for packet list type" << type;
        registerVerifiedListener(type, listener, deliverPending);
        return true;
    } else {
        qCWarning(networking) << "FAILED to Register a packet listener for packet list type" << type;
        return false;
    }
}

bool PacketReceiver::matchingMethodForListener(PacketType type, const ListenerReferencePointer& listener) const {
    Q_ASSERT_X(listener, "PacketReceiver::matchingMethodForListener", "No listener to call");

    bool isSourced = listener->isSourced();
    bool isNonSourcedPacket = PacketTypeEnum::getNonSourcedPackets().contains(type);

    assert(!isSourced || !isNonSourcedPacket);
    if (isSourced && isNonSourcedPacket) {
        qCDebug(networking) << "PacketReceiver::registerListener cannot support a sourced listener for type" << type;
        return false;
    }

    return true;
}

void PacketReceiver::registerVerifiedListener(PacketType type, const ListenerReferencePointer& listener, bool deliverPending) {
    Q_ASSERT_X(listener, "PacketReceiver::registerVerifiedListener", "No listener to register");
    QMutexLocker locker(&_packetListenerLock);

    if (_messageListenerMap.contains(type)) {
        qCWarning(networking) << "Registering a packet listener for packet type" << type
            << "that will remove a previously registered listener";
    }
    
    // add the mapping
    _messageListenerMap[type] = { listener, deliverPending };
}

void PacketReceiver::unregisterListener(QObject* listener) {
    Q_ASSERT_X(listener, "PacketReceiver::unregisterListener", "No listener to unregister");
    
    {
        QMutexLocker packetListenerLocker(&_packetListenerLock);
        
        // clear any registrations for this listener in _messageListenerMap
        auto it = _messageListenerMap.begin();
        
        while (it != _messageListenerMap.end()) {
            if (it.value().listener->getObject() == listener) {
                it = _messageListenerMap.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    QMutexLocker directConnectSetLocker(&_directConnectSetMutex);
    _directlyConnectedObjects.remove(listener);
}

void PacketReceiver::handleVerifiedPacket(std::unique_ptr<udt::Packet> packet) {
    // if we're supposed to drop this packet then break out here
    if (_shouldDropPackets) {
        return;
    }

    // setup an NLPacket from the packet we were passed
    auto nlPacket = NLPacket::fromBase(std::move(packet));
    auto receivedMessage = QSharedPointer<ReceivedMessage>::create(*nlPacket);

    handleVerifiedMessage(receivedMessage, true);
}

void PacketReceiver::handleVerifiedMessagePacket(std::unique_ptr<udt::Packet> packet) {
    auto nlPacket = NLPacket::fromBase(std::move(packet));

    auto key = std::pair<SockAddr, udt::Packet::MessageNumber>(nlPacket->getSenderSockAddr(), nlPacket->getMessageNumber());
    auto it = _pendingMessages.find(key);
    QSharedPointer<ReceivedMessage> message;

    if (it == _pendingMessages.end()) {
        // Create message
        message = QSharedPointer<ReceivedMessage>::create(*nlPacket);
        if (!message->isComplete()) {
            _pendingMessages[key] = message;
        }
        handleVerifiedMessage(message, true);  // Handler may handle first message packet immediately when it arrives.
    } else {
        message = it->second;
        message->appendPacket(*nlPacket);

        if (message->isComplete()) {
            _pendingMessages.erase(it);
            handleVerifiedMessage(message, false);
        }
    }
}

void PacketReceiver::handleMessageFailure(SockAddr from, udt::Packet::MessageNumber messageNumber) {
    auto key = std::pair<SockAddr, udt::Packet::MessageNumber>(from, messageNumber);
    auto it = _pendingMessages.find(key);
    if (it != _pendingMessages.end()) {
        auto message = it->second;
        message->setFailed();
        _pendingMessages.erase(it);
    }
}

void PacketReceiver::handleVerifiedMessage(QSharedPointer<ReceivedMessage> receivedMessage, bool justReceived) {
    auto nodeList = DependencyManager::get<LimitedNodeList>();
    
    SharedNodePointer matchingNode;
    
    if (receivedMessage->getSourceID() != Node::NULL_LOCAL_ID) {
        matchingNode = nodeList->nodeWithLocalID(receivedMessage->getSourceID());
    }
    QMutexLocker packetListenerLocker(&_packetListenerLock);
    
    auto it = _messageListenerMap.find(receivedMessage->getType());
    if (it != _messageListenerMap.end() && !it->listener.isNull()) {
         
        auto listener = it.value();

        if ((listener.deliverPending && !justReceived) || (!listener.deliverPending && !receivedMessage->isComplete())) {
            return;
        }
            
        bool success = false;

        bool isDirectConnect = false;
        // check if this is a directly connected listener
        {
            QMutexLocker directConnectLocker(&_directConnectSetMutex);
            isDirectConnect = _directlyConnectedObjects.contains(listener.listener->getObject());
        }

        // one final check on the QPointer before we go to invoke
        if (listener.listener->getObject()) {
            if (isDirectConnect) {
                success = listener.listener->invokeDirectly(receivedMessage, matchingNode);
            } else {
                success = listener.listener->invokeWithQt(receivedMessage, matchingNode);
            }
        } else {
            qCDebug(networking).nospace() << "Listener for packet " << receivedMessage->getType()
                << " has been destroyed. Removing from listener map.";
            it = _messageListenerMap.erase(it);

            // if it exists, remove the listener from _directlyConnectedObjects
            {
                QMutexLocker directConnectLocker(&_directConnectSetMutex);
                _directlyConnectedObjects.remove(listener.listener->getObject());
            }
        }

        if (!success) {
            qCDebug(networking).nospace() << "Error delivering packet " << receivedMessage->getType() << " to listener "
                << listener.listener->getObject();
        }

    } else if (it == _messageListenerMap.end()) {
        qCWarning(networking) << "No listener found for packet type" << receivedMessage->getType();
        
        // insert a dummy listener so we don't print this again
        _messageListenerMap.insert(receivedMessage->getType(), { ListenerReferencePointer(), false });
    }
}
