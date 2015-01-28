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

#include <AddressManager.h>
#include <AudioClient.h>
#include <AvatarHashMap.h>
#include <NodeList.h>

#include "RenderingClient.h"

RenderingClient* RenderingClient::_instance = NULL;

RenderingClient::RenderingClient(QObject *parent) :
    Client(parent)
{
    _instance = this;
    
    // tell the NodeList which node types all rendering clients will want to know about
    DependencyManager::get<NodeList>()->addSetOfNodeTypesToNodeInterestSet(NodeSet() << NodeType::AudioMixer << NodeType::AvatarMixer);

    DependencyManager::set<AvatarHashMap>();
    
    // get our audio client setup on its own thread
    QThread* audioThread = new QThread(this);
    auto audioClient = DependencyManager::set<AudioClient>();
    
    audioClient->setPositionGetter(getPositionForAudio);
    audioClient->setOrientationGetter(getOrientationForAudio);
    
    audioClient->moveToThread(audioThread);
    connect(audioThread, &QThread::started, audioClient.data(), &AudioClient::start);

    audioThread->start();
    
    connect(DependencyManager::get<AddressManager>().data(), &AddressManager::locationChangeRequired, 
            this, &RenderingClient::goToLocation);
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
    auto nodeList = DependencyManager::get<NodeList>();
    PacketType incomingType = packetTypeForPacket(incomingPacket);
    // only process this packet if we have a match on the packet version
    switch (incomingType) {
        case PacketTypeAudioEnvironment:
        case PacketTypeAudioStreamStats:
        case PacketTypeMixedAudio:
        case PacketTypeSilentAudioFrame: {
            
            if (incomingType == PacketTypeAudioStreamStats) {
                QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(), "parseAudioStreamStatsPacket",
                                          Qt::QueuedConnection,
                                          Q_ARG(QByteArray, incomingPacket));
            } else if (incomingType == PacketTypeAudioEnvironment) {
                QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(), "parseAudioEnvironmentData",
                                          Qt::QueuedConnection,
                                          Q_ARG(QByteArray, incomingPacket));
            } else {
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
        case PacketTypeBulkAvatarData:
        case PacketTypeKillAvatar:
        case PacketTypeAvatarIdentity:
        case PacketTypeAvatarBillboard: {
            // update having heard from the avatar-mixer and record the bytes received
            SharedNodePointer avatarMixer = nodeList->sendingNodeForPacket(incomingPacket);
            
            if (avatarMixer) {
                avatarMixer->setLastHeardMicrostamp(usecTimestampNow());
                avatarMixer->recordBytesReceived(incomingPacket.size());
                
                QMetaObject::invokeMethod(DependencyManager::get<AvatarHashMap>().data(),
                                          "processAvatarMixerDatagram",
                                          Q_ARG(const QByteArray&, incomingPacket),
                                          Q_ARG(const QWeakPointer<Node>&, avatarMixer));
            }
            break;
        }
        default:
            Client::processVerifiedPacket(senderSockAddr, incomingPacket);
            break;
    }
}

void RenderingClient::goToLocation(const glm::vec3& newPosition,
                                   bool hasOrientationChange, const glm::quat& newOrientation,
                                   bool shouldFaceLocation) {
    qDebug().nospace() << "RenderingClient goToLocation - moving to " << newPosition.x << ", "
       << newPosition.y << ", " << newPosition.z;

    glm::vec3 shiftedPosition = newPosition;

    if (hasOrientationChange) {
       qDebug().nospace() << "RenderingClient goToLocation - new orientation is "
           << newOrientation.x << ", " << newOrientation.y << ", " << newOrientation.z << ", " << newOrientation.w;

       // orient the user to face the target
       glm::quat quatOrientation = newOrientation;

       if (shouldFaceLocation) {

           quatOrientation = newOrientation * glm::angleAxis(PI, glm::vec3(0.0f, 1.0f, 0.0f));

           // move the user a couple units away
           const float DISTANCE_TO_USER = 2.0f;
           shiftedPosition = newPosition - quatOrientation * glm::vec3( 0.0f, 0.0f,-1.0f) * DISTANCE_TO_USER;
       }

       _orientation = quatOrientation;
    }

    _position = shiftedPosition;
    
}
