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

void PacketReceiver::registerPacketListener(PacketType::Value type, QObject* object, const char* slot) {
    Q_ASSERT(object);
    
    _packetListenerLock.lock();

    if (_packetListenerMap.contains(type)) {
        qDebug() << "Warning: Registering a packet listener for packet type " << type
            << "that will remove a previously registered listener";
    }
    
    // convert the const char* slot to a QMetaMethod
    int methodIndex = object->metaObject()->indexOfSlot(slot);
    Q_ASSERT(methodIndex >= 0);

    QMetaMethod slotMethod = object->metaObject()->method(methodIndex);
    Q_ASSERT(method.isValid());

    // compare the parameters we expect and the parameters the QMetaMethod has
    bool parametersMatch = false;

    if (NON_SOURCED_PACKETS.contains(type)) {
        const QList<QByteArray> NON_SOURCED_PACKET_LISTENER_PARAMETERS = QList<QByteArray>()
            << QMetaObject::normalizedType("QSharedPointer<NLPacket>");

        parametersMatch = slotMethod.parameterTypes() == NON_SOURCED_PACKET_LISTENER_PARAMETERS;

        qDebug() << "PacketReceiver::registerPacketListener expected a method that takes"
                << NON_SOURCED_PACKET_LISTENER_PARAMETERS
                << "but parameter method takes" << slotMethod.parameterTypes();
    } else {
        const QList<QByteArray> SOURCED_PACKET_LISTENER_PARAMETERS = QList<QByteArray>()
            << QMetaObject::normalizedType("QSharedPointer<NLPacket>")
            << QMetaObject::normalizedType("QSharedPointer<Node>");

        parametersMatch = slotMethod.parameterTypes() == SOURCED_PACKET_LISTENER_PARAMETERS;

        if (!parametersMatch) {
            qDebug() << "PacketReceiver::registerPacketListener expected a method that takes"
                << SOURCED_PACKET_LISTENER_PARAMETERS
                << "but parameter method takes" << slotMethod.parameterTypes();
        }
    }
    
    // make sure the parameters match
    assert(parametersMatch);

    // add the mapping
    _packetListenerMap[type] = ObjectMethodPair(QPointer<QObject>(object), slotMethod);
        
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
