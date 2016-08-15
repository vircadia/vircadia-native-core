//
//  PacketReceiver.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 1/23/2014.
//  Update by Ryan Huffman on 7/8/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PacketReceiver.h"

#include <QMutexLocker>

#include "DependencyManager.h"
#include "NetworkLogging.h"
#include "NodeList.h"
#include "SharedUtil.h"

PacketReceiver::PacketReceiver(QObject* parent) : QObject(parent) {
    qRegisterMetaType<QSharedPointer<NLPacket>>();
    qRegisterMetaType<QSharedPointer<NLPacketList>>();
    qRegisterMetaType<QSharedPointer<ReceivedMessage>>();
}

bool PacketReceiver::registerListenerForTypes(PacketTypeList types, QObject* listener, const char* slot) {
    Q_ASSERT_X(!types.empty(), "PacketReceiver::registerListenerForTypes", "No types to register");
    Q_ASSERT_X(listener, "PacketReceiver::registerListenerForTypes", "No object to register");
    Q_ASSERT_X(slot, "PacketReceiver::registerListenerForTypes", "No slot to register");
    
    // Partition types based on whether they are sourced or not (non sourced in front)
    auto middle = std::partition(std::begin(types), std::end(types), [](PacketType type) {
        return NON_SOURCED_PACKETS.contains(type);
    });
    
    QMetaMethod nonSourcedMethod, sourcedMethod;
    
    // Check we have a valid method for non sourced types if any
    if (middle != std::begin(types)) {
        nonSourcedMethod = matchingMethodForListener(*std::begin(types), listener, slot);
        if (!nonSourcedMethod.isValid()) {
            return false;
        }
    }
    
    // Check we have a valid method for sourced types if any
    if (middle != std::end(types)) {
        sourcedMethod = matchingMethodForListener(*middle, listener, slot);
        if (!sourcedMethod.isValid()) {
            return false;
        }
    }
    
    // Register non sourced types
    std::for_each(std::begin(types), middle, [this, &listener, &nonSourcedMethod](PacketType type) {
        registerVerifiedListener(type, listener, nonSourcedMethod);
    });
    
    // Register sourced types
    std::for_each(middle, std::end(types), [this, &listener, &sourcedMethod](PacketType type) {
        registerVerifiedListener(type, listener, sourcedMethod);
    });
    
    return true;
}

void PacketReceiver::registerDirectListener(PacketType type, QObject* listener, const char* slot) {
    Q_ASSERT_X(listener, "PacketReceiver::registerDirectListener", "No object to register");
    Q_ASSERT_X(slot, "PacketReceiver::registerDirectListener", "No slot to register");
    
    bool success = registerListener(type, listener, slot);
    if (success) {
        QMutexLocker locker(&_directConnectSetMutex);
        
        // if we successfully registered, add this object to the set of objects that are directly connected
        _directlyConnectedObjects.insert(listener);
    }
}

void PacketReceiver::registerDirectListenerForTypes(PacketTypeList types,
                                                    QObject* listener, const char* slot) {
    Q_ASSERT_X(listener, "PacketReceiver::registerDirectListenerForTypes", "No object to register");
    Q_ASSERT_X(slot, "PacketReceiver::registerDirectListenerForTypes", "No slot to register");
    
    // just call register listener for types to start
    bool success = registerListenerForTypes(std::move(types), listener, slot);
    if (success) {
        QMutexLocker locker(&_directConnectSetMutex);
        
        // if we successfully registered, add this object to the set of objects that are directly connected
        _directlyConnectedObjects.insert(listener);
    }
}

bool PacketReceiver::registerListener(PacketType type, QObject* listener, const char* slot,
                                             bool deliverPending) {
    Q_ASSERT_X(listener, "PacketReceiver::registerListener", "No object to register");
    Q_ASSERT_X(slot, "PacketReceiver::registerListener", "No slot to register");

    QMetaMethod matchingMethod = matchingMethodForListener(type, listener, slot);

    if (matchingMethod.isValid()) {
        qCDebug(networking) << "Registering a packet listener for packet list type" << type;
        registerVerifiedListener(type, listener, matchingMethod, deliverPending);
        return true;
    } else {
        qCWarning(networking) << "FAILED to Register a packet listener for packet list type" << type;
        return false;
    }
}

QMetaMethod PacketReceiver::matchingMethodForListener(PacketType type, QObject* object, const char* slot) const {
    Q_ASSERT_X(object, "PacketReceiver::matchingMethodForListener", "No object to call");
    Q_ASSERT_X(slot, "PacketReceiver::matchingMethodForListener", "No slot to call");

    // normalize the slot with the expected parameters
    static const QString SIGNATURE_TEMPLATE("%1(%2)");
    static const QString NON_SOURCED_MESSAGE_LISTENER_PARAMETERS = "QSharedPointer<ReceivedMessage>";

    QSet<QString> possibleSignatures {
        SIGNATURE_TEMPLATE.arg(slot, NON_SOURCED_MESSAGE_LISTENER_PARAMETERS)
    };

    if (!NON_SOURCED_PACKETS.contains(type)) {
        static const QString SOURCED_MESSAGE_LISTENER_PARAMETERS = "QSharedPointer<ReceivedMessage>,QSharedPointer<Node>";
        static const QString TYPEDEF_SOURCED_MESSAGE_LISTENER_PARAMETERS = "QSharedPointer<ReceivedMessage>,SharedNodePointer";

        // a sourced packet must take the shared pointer to the ReceivedMessage but optionally could include
        // a shared pointer to the node
        possibleSignatures << SIGNATURE_TEMPLATE.arg(slot, TYPEDEF_SOURCED_MESSAGE_LISTENER_PARAMETERS);
        possibleSignatures << SIGNATURE_TEMPLATE.arg(slot, SOURCED_MESSAGE_LISTENER_PARAMETERS);
    }

    int methodIndex = -1;

    foreach(const QString& signature, possibleSignatures) {
        QByteArray normalizedSlot =
            QMetaObject::normalizedSignature(signature.toStdString().c_str());
        
        // does the constructed normalized method exist?
        methodIndex = object->metaObject()->indexOfSlot(normalizedSlot.toStdString().c_str());

        if (methodIndex >= 0) {
            break;
        }
    }

    if (methodIndex < 0) {
        qCDebug(networking) << "PacketReceiver::registerListener expected a slot with one of the following signatures:"
                 << possibleSignatures.toList() << "- but such a slot was not found."
                 << "Could not complete listener registration for type" << type;
    }

    Q_ASSERT(methodIndex >= 0);

    // return the converted QMetaMethod
    if (methodIndex >= 0) {
        return object->metaObject()->method(methodIndex);
    } else {
        // if somehow (scripting?) something bad gets in here at runtime that doesn't hit the asserts above
        // return a non-valid QMetaMethod
        return QMetaMethod();
    }
}

void PacketReceiver::registerVerifiedListener(PacketType type, QObject* object, const QMetaMethod& slot, bool deliverPending) {
    Q_ASSERT_X(object, "PacketReceiver::registerVerifiedListener", "No object to register");
    QMutexLocker locker(&_packetListenerLock);

    if (_messageListenerMap.contains(type)) {
        qCWarning(networking) << "Registering a packet listener for packet type" << type
            << "that will remove a previously registered listener";
    }
    
    // add the mapping
    _messageListenerMap[type] = { QPointer<QObject>(object), slot, deliverPending };
}

void PacketReceiver::unregisterListener(QObject* listener) {
    Q_ASSERT_X(listener, "PacketReceiver::unregisterListener", "No listener to unregister");
    
    {
        QMutexLocker packetListenerLocker(&_packetListenerLock);
        
        // clear any registrations for this listener in _messageListenerMap
        auto it = _messageListenerMap.begin();
        
        while (it != _messageListenerMap.end()) {
            if (it.value().object == listener) {
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
    
    auto nodeList = DependencyManager::get<LimitedNodeList>();
    
    // setup an NLPacket from the packet we were passed
    auto nlPacket = NLPacket::fromBase(std::move(packet));
    auto receivedMessage = QSharedPointer<ReceivedMessage>::create(*nlPacket);

    _inPacketCount += 1;
    _inByteCount += nlPacket->size();

    handleVerifiedMessage(receivedMessage, true);
}

void PacketReceiver::handleVerifiedMessagePacket(std::unique_ptr<udt::Packet> packet) {
    auto nlPacket = NLPacket::fromBase(std::move(packet));

    _inPacketCount += 1;
    _inByteCount += nlPacket->size();

    auto key = std::pair<HifiSockAddr, udt::Packet::MessageNumber>(nlPacket->getSenderSockAddr(), nlPacket->getMessageNumber());
    auto it = _pendingMessages.find(key);
    QSharedPointer<ReceivedMessage> message;

    if (it == _pendingMessages.end()) {
        // Create message
        message = QSharedPointer<ReceivedMessage>::create(*nlPacket);
        if (!message->isComplete()) {
            _pendingMessages[key] = message;
        }
        handleVerifiedMessage(message, true);
    } else {
        message = it->second;
        message->appendPacket(*nlPacket);

        if (message->isComplete()) {
            _pendingMessages.erase(it);
            handleVerifiedMessage(message, false);
        }
    }
}

void PacketReceiver::handleMessageFailure(HifiSockAddr from, udt::Packet::MessageNumber messageNumber) {
    auto key = std::pair<HifiSockAddr, udt::Packet::MessageNumber>(from, messageNumber);
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
    
    if (!receivedMessage->getSourceID().isNull()) {
        matchingNode = nodeList->nodeWithUUID(receivedMessage->getSourceID());
    }
    
    QMutexLocker packetListenerLocker(&_packetListenerLock);
    
    bool listenerIsDead = false;
    
    auto it = _messageListenerMap.find(receivedMessage->getType());
            
    if (it != _messageListenerMap.end() && it->method.isValid()) {
         
        auto listener = it.value();

        if ((listener.deliverPending && !justReceived) || (!listener.deliverPending && !receivedMessage->isComplete())) {
            return;
        }
    
        if (listener.object) {
            
            bool success = false;

            Qt::ConnectionType connectionType;
            // check if this is a directly connected listener
            {
                QMutexLocker directConnectLocker(&_directConnectSetMutex);
 
                connectionType = _directlyConnectedObjects.contains(listener.object) ? Qt::DirectConnection : Qt::AutoConnection;
            }
            
            PacketType packetType = receivedMessage->getType();
            
            if (matchingNode) {
                matchingNode->recordBytesReceived(receivedMessage->getSize());

                QMetaMethod metaMethod = listener.method;
                
                static const QByteArray QSHAREDPOINTER_NODE_NORMALIZED = QMetaObject::normalizedType("QSharedPointer<Node>");
                static const QByteArray SHARED_NODE_NORMALIZED = QMetaObject::normalizedType("SharedNodePointer");
                
                // one final check on the QPointer before we go to invoke
                if (listener.object) {
                    if (metaMethod.parameterTypes().contains(SHARED_NODE_NORMALIZED)) {
                        success = metaMethod.invoke(listener.object,
                                                    connectionType,
                                                    Q_ARG(QSharedPointer<ReceivedMessage>, receivedMessage),
                                                    Q_ARG(SharedNodePointer, matchingNode));
                        
                    } else if (metaMethod.parameterTypes().contains(QSHAREDPOINTER_NODE_NORMALIZED)) {
                        success = metaMethod.invoke(listener.object,
                                                    connectionType,
                                                    Q_ARG(QSharedPointer<ReceivedMessage>, receivedMessage),
                                                    Q_ARG(QSharedPointer<Node>, matchingNode));
                        
                    } else {
                        success = metaMethod.invoke(listener.object,
                                                    connectionType,
                                                    Q_ARG(QSharedPointer<ReceivedMessage>, receivedMessage));
                    }
                } else {
                    listenerIsDead = true;
                }
            } else {
                // qDebug() << "Got verified unsourced packet list: " << QString(nlPacketList->getMessage());
                
                // one final check on the QPointer before we invoke
                if (listener.object) {
                    success = listener.method.invoke(listener.object,
                                                     Q_ARG(QSharedPointer<ReceivedMessage>, receivedMessage));
                } else {
                    listenerIsDead = true;
                }
                
            }
            
            if (!success) {
                qCDebug(networking).nospace() << "Error delivering packet " << packetType << " to listener "
                    << listener.object << "::" << qPrintable(listener.method.methodSignature());
            }
            
        } else {
            listenerIsDead = true;
        }
        
        if (listenerIsDead) {
            qCDebug(networking).nospace() << "Listener for packet " << receivedMessage->getType()
                << " has been destroyed. Removing from listener map.";
            it = _messageListenerMap.erase(it);
            
            // if it exists, remove the listener from _directlyConnectedObjects
            {
                QMutexLocker directConnectLocker(&_directConnectSetMutex);
                _directlyConnectedObjects.remove(listener.object);
            }
        }
    } else if (it == _messageListenerMap.end()) {
        qCWarning(networking) << "No listener found for packet type" << receivedMessage->getType();
        
        // insert a dummy listener so we don't print this again
        _messageListenerMap.insert(receivedMessage->getType(), { nullptr, QMetaMethod(), false });
    }
}
