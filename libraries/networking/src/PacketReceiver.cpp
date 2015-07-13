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

#include "DependencyManager.h"
#include "NetworkLogging.h"
#include "NodeList.h"
#include "SharedUtil.h"

PacketReceiver::PacketReceiver(QObject* parent) :
    QObject(parent),
    _packetListenerMap()
{

}

void PacketReceiver::registerPacketListeners(const QSet<PacketType::Value>& types, QObject* object, const char* slot) {
    QSet<PacketType::Value> nonSourcedTypes;
    QSet<PacketType::Value> sourcedTypes;

    foreach(PacketType::Value type, types) {
        if (NON_SOURCED_PACKETS.contains(type)) {
            nonSourcedTypes << type;
        } else {
            sourcedTypes << type;
        }
    }

    if (nonSourcedTypes.size() > 0) {
        QMetaMethod nonSourcedMethod = matchingMethodForListener(*nonSourcedTypes.begin(), object, slot);
        foreach(PacketType::Value type, nonSourcedTypes) {
            registerVerifiedListener(type, object, nonSourcedMethod);
        }
    }

    if (sourcedTypes.size() > 0) {
        QMetaMethod sourcedMethod = matchingMethodForListener(*sourcedTypes.begin(), object, slot);
        foreach(PacketType::Value type, sourcedTypes) {
            registerVerifiedListener(type, object, sourcedMethod);
        }
    }
}

void PacketReceiver::registerPacketListener(PacketType::Value type, QObject* object, const char* slot) {
    QMetaMethod matchingMethod = matchingMethodForListener(type, object, slot);
    if (matchingMethod.isValid()) {
        registerVerifiedListener(type, object, matchingMethod);
    }
}

QMetaMethod PacketReceiver::matchingMethodForListener(PacketType::Value type, QObject* object, const char* slot) const {
    Q_ASSERT(object);

    // normalize the slot with the expected parameters
    QString normalizedSlot;

    if (NON_SOURCED_PACKETS.contains(type)) {
        const QString NON_SOURCED_PACKET_LISTENER_PARAMETERS = "QSharedPointer<NLPacket>";

        QString nonNormalizedSignature = QString("%1(%2)").arg(slot).arg(NON_SOURCED_PACKET_LISTENER_PARAMETERS);
        normalizedSlot =
            QMetaObject::normalizedSignature(nonNormalizedSignature.toStdString().c_str());
    } else {
        const QString SOURCED_PACKET_LISTENER_PARAMETERS = "QSharedPointer<NLPacket>,QSharedPointer<Node>";
    
        QString nonNormalizedSignature = QString("%1(%2)").arg(slot).arg(SOURCED_PACKET_LISTENER_PARAMETERS);
        normalizedSlot =
            QMetaObject::normalizedSignature(nonNormalizedSignature.toStdString().c_str());
    }

    // does the constructed normalized method exist?
    int methodIndex = object->metaObject()->indexOfSlot(normalizedSlot.toStdString().c_str());
    
    if (methodIndex < 0) {
        qDebug() << "PacketReceiver::registerPacketListener expected a method with a normalized signature of"
                << normalizedSlot << "but such a method was not found.";
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

void PacketReceiver::registerVerifiedListener(PacketType::Value type, QObject* object, const QMetaMethod& slot) {
    _packetListenerLock.lock();

    if (_packetListenerMap.contains(type)) {
        qDebug() << "Warning: Registering a packet listener for packet type " << type
            << "that will remove a previously registered listener";
    }

    // add the mapping
    _packetListenerMap[type] = ObjectMethodPair(QPointer<QObject>(object), slot);
        
    _packetListenerLock.unlock();

}

bool PacketReceiver::packetVersionMatch(const NLPacket& packet) {

    if (packet.getVersion() != versionForPacketType(packet.getType())
        && packet.getType() != PacketType::StunResponse) {

        static QMultiMap<QUuid, PacketType::Value> versionDebugSuppressMap;

        const QUuid& senderID = packet.getSourceID();
        
        if (!versionDebugSuppressMap.contains(senderID, packet.getType())) {

            qCDebug(networking) << "Packet version mismatch on" << packet.getType() << "- Sender"
                << senderID << "sent" << qPrintable(QString::number(packet.getVersion())) << "but"
                << qPrintable(QString::number(versionForPacketType(packet.getType()))) << "expected.";

            emit packetVersionMismatch(packet.getType());

            versionDebugSuppressMap.insert(senderID, packet.getType());
        }

        return false;
    } else {
        return true;
    }
}

void PacketReceiver::processDatagrams() {
    //PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                            //"PacketReceiver::processDatagrams()");

    if (_isShuttingDown) {
        return; // bail early... we're shutting down.
    }

    auto nodeList = DependencyManager::get<NodeList>();

    while (DependencyManager::get<NodeList>()->getNodeSocket().hasPendingDatagrams()) {
        // setup a buffer to read the packet into
        int packetSizeWithHeader = nodeList->getNodeSocket().pendingDatagramSize();
        std::unique_ptr<char> buffer = std::unique_ptr<char>(new char[packetSizeWithHeader]);
        
        // setup a HifiSockAddr to read into
        HifiSockAddr senderSockAddr;

        // pull the datagram
        nodeList->getNodeSocket().readDatagram(buffer.get(), packetSizeWithHeader,
                                               senderSockAddr.getAddressPointer(), senderSockAddr.getPortPointer());

        // setup an NLPacket from the data we just read
        auto packet = NLPacket::fromReceivedPacket(std::move(buffer), packetSizeWithHeader, senderSockAddr);
        
        _inPacketCount++;
        _inByteCount += packetSizeWithHeader;

        if (packetVersionMatch(*packet)) {
            
            SharedNodePointer matchingNode;
            if (nodeList->packetSourceAndHashMatch(*packet, matchingNode)) {
                 
                if (matchingNode) {
                    // No matter if this packet is handled or not, we update the timestamp for the last time we heard
                    // from this sending node
                    matchingNode->setLastHeardMicrostamp(usecTimestampNow());
                }

                _packetListenerLock.lock();
                
                auto it = _packetListenerMap.find(packet->getType());

                if (it != _packetListenerMap.end()) {

                    auto listener = it.value();
                   
                    if (!listener.first.isNull()) {
                        
                        if (matchingNode) {
                            emit dataReceived(matchingNode->getType(), packet->getSizeWithHeader());
                        } else {
                            emit dataReceived(NodeType::Unassigned, packet->getSizeWithHeader());
                        }
                        
                        bool success = false;

                        if (matchingNode) {
                            success = listener.second.invoke(listener.first,
                                Q_ARG(QSharedPointer<NLPacket>, QSharedPointer<NLPacket>(packet.release())));
                        } else {
                            success = listener.second.invoke(listener.first,
                                Q_ARG(QSharedPointer<NLPacket>, QSharedPointer<NLPacket>(packet.release())),
                                Q_ARG(SharedNodePointer, matchingNode));
                        }
                                            
                        if (!success) {
                            qDebug() << "Error delivering packet " << nameForPacketType(packet->getType()) << " to listener: "
                                << listener.first->objectName() << "::" << listener.second.name();
                        }

                    } else {
                        // we have a dead listener - remove this mapping from the _packetListenerMap
                        qDebug() << "Listener for packet type" << nameForPacketType(packet->getType())
                            << "has been destroyed - removing mapping.";
                        _packetListenerMap.erase(it);
                    }

                     _packetListenerLock.unlock();

                } else {
                    _packetListenerLock.unlock();
                    qDebug() << "No listener found for packet type " << nameForPacketType(packet->getType());
                }
            }
        }
    }
}
