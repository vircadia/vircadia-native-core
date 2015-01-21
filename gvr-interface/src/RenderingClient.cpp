//
//  RenderingClient.cpp
//  gvr-interface/src
//
//  Created by Stephen Birarda on 1/20/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QThread>
#include <QtWidgets/QInputDialog>

#include <AudioClient.h>
#include <NodeList.h>

#include "RenderingClient.h"

RenderingClient::RenderingClient(QObject *parent) :
    Client(parent)
{
    // tell the NodeList which node types all rendering clients will want to know about
    DependencyManager::get<NodeList>()->addSetOfNodeTypesToNodeInterestSet(NodeSet() << NodeType::AudioMixer);
    
    // get our audio client setup on its own thread
    QThread* audioThread = new QThread(this);
    auto audioClient = DependencyManager::set<AudioClient>();
    
    audioClient->moveToThread(audioThread);
    connect(audioThread, &QThread::started, audioClient.data(), &AudioClient::start);

    audioThread->start();
}

RenderingClient::~RenderingClient() {
    auto audioClient = DependencyManager::get<AudioClient>();

    // stop the audio client
    QMetaObject::invokeMethod(audioClient.data(), "stop", Qt::BlockingQueuedConnection);
    
    // ask the audio thread to quit and wait until it is done
    audioClient->thread()->quit();
    audioClient->thread()->wait();
}

void RenderingClient::processVerifiedPacket(const HifiSockAddr& senderSockAddr, const QByteArray& incomingPacket) {
    PacketType incomingType = packetTypeForPacket(incomingPacket);
    // only process this packet if we have a match on the packet version
    switch (incomingType) {
        case PacketTypeAudioEnvironment:
        case PacketTypeAudioStreamStats:
        case PacketTypeMixedAudio:
        case PacketTypeSilentAudioFrame: {
            auto nodeList = DependencyManager::get<NodeList>();
             
            if (incomingType == PacketTypeAudioStreamStats) {
                QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(), "parseAudioStreamStatsPacket",
                                          Qt::QueuedConnection,
                                          Q_ARG(QByteArray, incomingPacket));
            } else if (incomingType == PacketTypeAudioEnvironment) {
                QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(), "parseAudioEnvironmentData",
                                          Qt::QueuedConnection,
                                          Q_ARG(QByteArray, incomingPacket));
            } else {
                qDebug() << "Processing received audio of" << incomingPacket.size();
                QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(), "addReceivedAudioToStream",
                                          Qt::QueuedConnection,
                                          Q_ARG(QByteArray, incomingPacket));
            }
            
            // update having heard from the audio-mixer and record the bytes received
            SharedNodePointer audioMixer = nodeList->sendingNodeForPacket(incomingPacket);
            
            if (audioMixer) {
                audioMixer->setLastHeardMicrostamp(usecTimestampNow());
                audioMixer->recordBytesReceived(incomingPacket.size());
            }
            
            break;
        }
        default:
            Client::processVerifiedPacket(senderSockAddr, incomingPacket);
            break;
    }
}