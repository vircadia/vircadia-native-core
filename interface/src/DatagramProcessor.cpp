//
//  DatagramProcessor.cpp
//  interface/src
//
//  Created by Stephen Birarda on 1/23/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QWeakPointer>

#include <AccountManager.h>
#include <PerfStat.h>

#include "Application.h"
#include "Audio.h"
#include "Menu.h"

#include "DatagramProcessor.h"

DatagramProcessor::DatagramProcessor(QObject* parent) :
    QObject(parent)
{
    
}

void DatagramProcessor::processDatagrams() {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                            "DatagramProcessor::processDatagrams()");
    
    HifiSockAddr senderSockAddr;
    
    static QByteArray incomingPacket;
    
    Application* application = Application::getInstance();
    auto nodeList = DependencyManager::get<NodeList>();
    
    while (DependencyManager::get<NodeList>()->getNodeSocket().hasPendingDatagrams()) {
        incomingPacket.resize(nodeList->getNodeSocket().pendingDatagramSize());
        nodeList->getNodeSocket().readDatagram(incomingPacket.data(), incomingPacket.size(),
                                               senderSockAddr.getAddressPointer(), senderSockAddr.getPortPointer());
        
        _packetCount++;
        _byteCount += incomingPacket.size();
        
        if (nodeList->packetVersionAndHashMatch(incomingPacket)) {
            
            PacketType incomingType = packetTypeForPacket(incomingPacket);
            // only process this packet if we have a match on the packet version
            switch (incomingType) {
                case PacketTypeAudioEnvironment:
                case PacketTypeAudioStreamStats:
                case PacketTypeMixedAudio:
                case PacketTypeSilentAudioFrame: {
                    if (incomingType == PacketTypeAudioStreamStats) {
                        QMetaObject::invokeMethod(DependencyManager::get<Audio>().data(), "parseAudioStreamStatsPacket",
                                                  Qt::QueuedConnection,
                                                  Q_ARG(QByteArray, incomingPacket));
                    } else if (incomingType == PacketTypeAudioEnvironment) {
                        QMetaObject::invokeMethod(DependencyManager::get<Audio>().data(), "parseAudioEnvironmentData",
                                                  Qt::QueuedConnection,
                                                  Q_ARG(QByteArray, incomingPacket));
                    } else {
                        QMetaObject::invokeMethod(DependencyManager::get<Audio>().data(), "addReceivedAudioToStream",
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
                case PacketTypeEntityAddResponse:
                    // this will keep creatorTokenIDs to IDs mapped correctly
                    EntityItemID::handleAddEntityResponse(incomingPacket);
                    application->getEntities()->getTree()->handleAddEntityResponse(incomingPacket);
                    break;
                case PacketTypeEntityData:
                case PacketTypeEntityErase:
                case PacketTypeOctreeStats:
                case PacketTypeEnvironmentData: {
                    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                                            "Application::networkReceive()... _octreeProcessor.queueReceivedPacket()");
                    SharedNodePointer matchedNode = DependencyManager::get<NodeList>()->sendingNodeForPacket(incomingPacket);
                    
                    if (matchedNode) {
                        // add this packet to our list of octree packets and process them on the octree data processing
                        application->_octreeProcessor.queueReceivedPacket(matchedNode, incomingPacket);
                    }
                    
                    break;
                }
                case PacketTypeMetavoxelData:
                    nodeList->findNodeAndUpdateWithDataFromPacket(incomingPacket);
                    break;
                case PacketTypeBulkAvatarData:
                case PacketTypeKillAvatar:
                case PacketTypeAvatarIdentity:
                case PacketTypeAvatarBillboard: {
                    // update having heard from the avatar-mixer and record the bytes received
                    SharedNodePointer avatarMixer = nodeList->sendingNodeForPacket(incomingPacket);
                    
                    if (avatarMixer) {
                        avatarMixer->setLastHeardMicrostamp(usecTimestampNow());
                        avatarMixer->recordBytesReceived(incomingPacket.size());
                        
                        QMetaObject::invokeMethod(&application->getAvatarManager(), "processAvatarMixerDatagram",
                                                  Q_ARG(const QByteArray&, incomingPacket),
                                                  Q_ARG(const QWeakPointer<Node>&, avatarMixer));
                    }
                    
                    application->_bandwidthMeter.inputStream(BandwidthMeter::AVATARS).updateValue(incomingPacket.size());
                    break;
                }
                case PacketTypeDomainConnectionDenied: {
                    // output to the log so the user knows they got a denied connection request
                    // and check and signal for an access token so that we can make sure they are logged in
                    qDebug() << "The domain-server denied a connection request.";
                    qDebug() << "You may need to re-log to generate a keypair so you can provide a username signature.";
                    AccountManager::getInstance().checkAndSignalForAccessToken();
                    break;
                }
                case PacketTypeNoisyMute:
                case PacketTypeMuteEnvironment: {
                    bool mute = !DependencyManager::get<Audio>()->isMuted();
                    
                    if (incomingType == PacketTypeMuteEnvironment) {
                        glm::vec3 position;
                        float radius, distance;
                        
                        int headerSize = numBytesForPacketHeaderGivenPacketType(PacketTypeMuteEnvironment);
                        memcpy(&position, incomingPacket.constData() + headerSize, sizeof(glm::vec3));
                        memcpy(&radius, incomingPacket.constData() + headerSize + sizeof(glm::vec3), sizeof(float));
                        distance = glm::distance(Application::getInstance()->getAvatar()->getPosition(), position);
                        
                        mute = mute && (distance < radius);
                    }
                    
                    if (mute) {
                        DependencyManager::get<Audio>()->toggleMute();
                        if (incomingType == PacketTypeMuteEnvironment) {
                            AudioScriptingInterface::getInstance().environmentMuted();
                        } else {
                            AudioScriptingInterface::getInstance().mutedByMixer();
                        }
                    }
                    break;
                }
                case PacketTypeEntityEditNack:
                    if (!Menu::getInstance()->isOptionChecked(MenuOption::DisableNackPackets)) {
                        application->_entityEditSender.processNackPacket(incomingPacket);
                    }
                    break;
                default:
                    nodeList->processNodeData(senderSockAddr, incomingPacket);
                    break;
            }
        }
    }
}
