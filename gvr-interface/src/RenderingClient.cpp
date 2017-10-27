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

RenderingClient::RenderingClient(QObject *parent, const QString& launchURLString) :
    Client(parent)
{
    _instance = this;
    
    // connect to AddressManager and pass it the launch URL, if we have one
    auto addressManager = DependencyManager::get<AddressManager>();
    connect(addressManager.data(), &AddressManager::locationChangeRequired, this, &RenderingClient::goToLocation);
    addressManager->loadSettings(launchURLString);
    
    // tell the NodeList which node types all rendering clients will want to know about
    DependencyManager::get<NodeList>()->addSetOfNodeTypesToNodeInterestSet(NodeSet() << NodeType::AudioMixer << NodeType::AvatarMixer);

    DependencyManager::set<AvatarHashMap>();
    
    // get our audio client setup on its own thread
    auto audioClient = DependencyManager::set<AudioClient>();
    audioClient->setPositionGetter(getPositionForAudio);
    audioClient->setOrientationGetter(getOrientationForAudio);
    audioClient->startThread();
    
   
    connect(&_avatarTimer, &QTimer::timeout, this, &RenderingClient::sendAvatarPacket);
    _avatarTimer.setInterval(16); // 60 FPS
    _avatarTimer.start();
    _fakeAvatar.setDisplayName("GearVR");
    _fakeAvatar.setFaceModelURL(QUrl(DEFAULT_HEAD_MODEL_URL));
    _fakeAvatar.setSkeletonModelURL(QUrl(DEFAULT_BODY_MODEL_URL));
    _fakeAvatar.toByteArray(); // Creates HeadData
}

void RenderingClient::sendAvatarPacket() {
    _fakeAvatar.setPosition(_position);
    _fakeAvatar.setHeadOrientation(_orientation);

    QByteArray packet = byteArrayWithPopulatedHeader(PacketTypeAvatarData);
    packet.append(_fakeAvatar.toByteArray());
    DependencyManager::get<NodeList>()->broadcastToNodes(packet, NodeSet() << NodeType::AvatarMixer);
    _fakeAvatar.sendIdentityPacket();
}

void RenderingClient::cleanupBeforeQuit() {
    DependencyManager::get<AudioClient>()->cleanupBeforeQuit();
    // destroy the AudioClient so it and its thread will safely go down
    DependencyManager::destroy<AudioClient>();
}

void RenderingClient::processVerifiedPacket(const HifiSockAddr& senderSockAddr, const QByteArray& incomingPacket) {
    auto nodeList = DependencyManager::get<NodeList>();
    PacketType incomingType = packetTypeForPacket(incomingPacket);
    
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
