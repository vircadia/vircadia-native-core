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

#include <QtCore/QWeakPointer>

#include <AccountManager.h>
#include <PerfStat.h>

#include "Application.h"
#include "avatar/AvatarManager.h"
#include "AudioClient.h"
#include "Menu.h"
#include "InterfaceLogging.h"

#include "PacketReceiver.h"

PacketReceiver::PacketReceiver(QObject* parent) :
    QObject(parent),
    _packetListenerMap()
{

}

void PacketReceiver::registerPacketListener(PacketType::Value type, QObject* object, QString methodName) {
    packetListenerLock.lock();
    if (packetListenerMap.contains(type)) {
        qDebug() << "Warning: Registering a packet listener for packet type " << type
            << " that will remove a previously registered listener";
    }
    packetListenerMap[type] = QPair<QObject*, QString>(object, methodName);
    packetListenerLock.unlock();
}

void PacketReceiver::processDatagrams() {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                            "PacketReceiver::processDatagrams()");

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

            packetListenerLock.lock();
            auto& listener = packetListenerMap[incomingType];
            packetListenerLock.unlock();

            if (packetListenerMap.contains(incomingType)) {
                auto& listener = packetListenerMap[incomingType];
                NLPacket packet;
                bool success = QMetaObject::invokeMethod(listener.first, listener.second,
                        Q_ARG(std::unique_ptr<NLPacket>, packet),
                        Q_ARG(HifiSockAddr, senderSockAddr));
                if (!success) {
                    qDebug() << "Error sending packet " << incomingType << " to listener: " << listener.first.name() << "::" << listener.second;
                }
            } else {
                QDebug() << "No listener found for packet type: " << incomingType;
            }

        }
    }
}
