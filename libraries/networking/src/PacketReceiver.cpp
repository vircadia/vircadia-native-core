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
#include "NLPacket.h"
#include "NodeList.h"

PacketReceiver::PacketReceiver(QObject* parent) :
    QObject(parent),
    _packetListenerMap()
{

}

void PacketReceiver::registerPacketListener(PacketType::Value type, QObject* object, QString methodName) {
    _packetListenerLock.lock();
    if (_packetListenerMap.contains(type)) {
        qDebug() << "Warning: Registering a packet listener for packet type " << type
            << " that will remove a previously registered listener";
    }
    _packetListenerMap[type] = QPair<QObject*, QString>(object, methodName);
    _packetListenerLock.unlock();
}

void PacketReceiver::processDatagrams() {
    //PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                            //"PacketReceiver::processDatagrams()");

    if (_isShuttingDown) {
        return; // bail early... we're shutting down.
    }

    static QByteArray incomingPacket;

    auto nodeList = DependencyManager::get<NodeList>();

    while (DependencyManager::get<NodeList>()->getNodeSocket().hasPendingDatagrams()) {
        incomingPacket.resize(nodeList->getNodeSocket().pendingDatagramSize());
        HifiSockAddr senderSockAddr;
        nodeList->readDatagram(incomingPacket, senderSockAddr.getAddressPointer(), senderSockAddr.getPortPointer());

        _inPacketCount++;
        _inByteCount += incomingPacket.size();

        if (nodeList->packetVersionAndHashMatch(incomingPacket)) {
            PacketType::Value incomingType = packetTypeForPacket(incomingPacket);

            // TODO What do we do about this?
            //nodeList->processNodeData(senderSockAddr, incomingPacket);

            _packetListenerLock.lock();
            auto& listener = _packetListenerMap[incomingType];
            _packetListenerLock.unlock();

            if (_packetListenerMap.contains(incomingType)) {
                auto& listener = _packetListenerMap[incomingType];
                //TODO Update packet
                QSharedPointer<NLPacket> packet;
                bool success = true; //QMetaObject::invokeMethod(listener.first, listener.second,
                //         Q_ARG(QSharedPointer<NLPacket>, packet),
                //         Q_ARG(HifiSockAddr, senderSockAddr));
                if (!success) {
                    qDebug() << "Error sending packet " << incomingType << " to listener: " << listener.first->objectName() << "::" << listener.second;
                }
            } else {
                qDebug() << "No listener found for packet type: " << incomingType;
            }

        }
    }
}
