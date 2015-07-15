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
    qRegisterMetaType<QSharedPointer<NLPacket>>();
}

void PacketReceiver::registerListenerForTypes(const QSet<PacketType::Value>& types, PacketListener* listener, const char* slot) {
    QSet<PacketType::Value> nonSourcedTypes;
    QSet<PacketType::Value> sourcedTypes;

    foreach(PacketType::Value type, types) {
        if (NON_SOURCED_PACKETS.contains(type)) {
            nonSourcedTypes << type;
        } else {
            sourcedTypes << type;
        }
    }

    QObject* object = dynamic_cast<QObject*>(listener);
    Q_ASSERT(object);

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

void PacketReceiver::registerListener(PacketType::Value type, PacketListener* listener, const char* slot) {
    QObject* object = dynamic_cast<QObject*>(listener);
    Q_ASSERT(object);

    QMetaMethod matchingMethod = matchingMethodForListener(type, object, slot);

    if (matchingMethod.isValid()) {
        registerVerifiedListener(type, object, matchingMethod);
    }
}

QMetaMethod PacketReceiver::matchingMethodForListener(PacketType::Value type, QObject* object, const char* slot) const {
    Q_ASSERT(object);

    // normalize the slot with the expected parameters

    const QString NON_SOURCED_PACKET_LISTENER_PARAMETERS = "QSharedPointer<NLPacket>";

    QSet<QString> possibleSignatures { QString("%1(%2)").arg(slot).arg(NON_SOURCED_PACKET_LISTENER_PARAMETERS) };

    if (!NON_SOURCED_PACKETS.contains(type)) {
        static const QString SOURCED_PACKET_LISTENER_PARAMETERS = "QSharedPointer<NLPacket>,QSharedPointer<Node>";
        static const QString TYPEDEF_SOURCED_PACKET_LISTENER_PARAMETERS = "QSharedPointer<NLPacket>,SharedNodePointer";

        // a sourced packet must take the shared pointer to the packet but optionally could include
        // a shared pointer to the node

        possibleSignatures << QString("%1(%2)").arg(slot).arg(TYPEDEF_SOURCED_PACKET_LISTENER_PARAMETERS);
        possibleSignatures << QString("%1(%2)").arg(slot).arg(SOURCED_PACKET_LISTENER_PARAMETERS);
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
        qDebug() << "PacketReceiver::registerListener expected a slot with one of the following signatures:"
                 << possibleSignatures.toList() << "- but such a slot was not found."
                 << "Could not complete listener registration for type"
                 << type << "-" << nameForPacketType(type);
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
        qDebug() << "Warning: Registering a packet listener for packet type" << type
            << "that will remove a previously registered listener";
    }

    // add the mapping
    _packetListenerMap[type] = ObjectMethodPair(object, slot);

    _packetListenerLock.unlock();

}

void PacketReceiver::unregisterListener(PacketListener* listener) {
    _packetListenerLock.lock();

    auto it = _packetListenerMap.begin();

    while (it != _packetListenerMap.end()) {
        if (it.value().first == dynamic_cast<QObject*>(listener)) {
            // this listener matches - erase it
            it = _packetListenerMap.erase(it);
        } else {
            ++it;
        }
    }

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

    auto nodeList = DependencyManager::get<LimitedNodeList>();

    while (nodeList->getNodeSocket().hasPendingDatagrams()) {
        // setup a buffer to read the packet into
        int packetSizeWithHeader = nodeList->getNodeSocket().pendingDatagramSize();
        std::unique_ptr<char> buffer = std::unique_ptr<char>(new char[packetSizeWithHeader]);

        // if we're supposed to drop this packet then break out here
        if (_shouldDropPackets) {
            break;
        }

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

                    if (listener.first) {

                        bool success = false;
                        PacketType::Value packetType = packet->getType();

                        if (matchingNode) {
                            // if this was a sequence numbered packet we should store the last seq number for
                            // a packet of this type for this node
                            if (SEQUENCE_NUMBERED_PACKETS.contains(packet->getType())) {
                                matchingNode->setLastSequenceNumberForPacketType(packet->readSequenceNumber(), packet->getType());
                            }

                            emit dataReceived(matchingNode->getType(), packet->getDataSize());
                            QMetaMethod metaMethod = listener.second;

                            static const QByteArray QSHAREDPOINTER_NODE_NORMALIZED = QMetaObject::normalizedType("QSharedPointer<Node>");
                            static const QByteArray SHARED_NODE_NORMALIZED = QMetaObject::normalizedType("SharedNodePointer");

                            if (metaMethod.parameterTypes().contains(SHARED_NODE_NORMALIZED)) {
                                success = metaMethod.invoke(listener.first,
                                        Q_ARG(QSharedPointer<NLPacket>, QSharedPointer<NLPacket>(packet.release())),
                                        Q_ARG(SharedNodePointer, matchingNode));

                            } else if (metaMethod.parameterTypes().contains(QSHAREDPOINTER_NODE_NORMALIZED)) {
                                success = metaMethod.invoke(listener.first,
                                        Q_ARG(QSharedPointer<NLPacket>, QSharedPointer<NLPacket>(packet.release())),
                                        Q_ARG(QSharedPointer<Node>, matchingNode));

                            } else {
                                success = metaMethod.invoke(listener.first,
                                    Q_ARG(QSharedPointer<NLPacket>, QSharedPointer<NLPacket>(packet.release())));
                            }
                        } else {
                            emit dataReceived(NodeType::Unassigned, packet->getDataSize());

                            success = listener.second.invoke(listener.first,
                                Q_ARG(QSharedPointer<NLPacket>, QSharedPointer<NLPacket>(packet.release())));
                        }

                        if (!success) {
                            qDebug().nospace() << "Error delivering packet " << packetType
                                << " (" << qPrintable(nameForPacketType(packetType)) << ") to listener "
                                << listener.first << "::" << qPrintable(listener.second.methodSignature());
                        }

                    }
                } else {
                    qWarning() << "No listener found for packet type " << nameForPacketType(packet->getType());
                    
                    // insert a dummy listener so we don't print this again
                    _packetListenerMap.insert(packet->getType(), { nullptr, QMetaMethod() });
                }

                _packetListenerLock.unlock();
            }
        }
    }
}
