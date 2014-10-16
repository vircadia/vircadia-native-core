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
    NodeList* nodeList = NodeList::getInstance();
    
    while (NodeList::getInstance()->getNodeSocket().hasPendingDatagrams()) {
        incomingPacket.resize(nodeList->getNodeSocket().pendingDatagramSize());
        nodeList->getNodeSocket().readDatagram(incomingPacket.data(), incomingPacket.size(),
                                               senderSockAddr.getAddressPointer(), senderSockAddr.getPortPointer());
        
        _packetCount++;
        _byteCount += incomingPacket.size();
        
        if (nodeList->packetVersionAndHashMatch(incomingPacket)) {
            
            PacketType incomingType = packetTypeForPacket(incomingPacket);
            // only process this packet if we have a match on the packet version
            switch (incomingType) {
                case PacketTypeMixedAudio:
                case PacketTypeSilentAudioFrame:
                case PacketTypeAudioStreamStats: {
                    if (incomingType != PacketTypeAudioStreamStats) {
                        QMetaObject::invokeMethod(&application->_audio, "addReceivedAudioToStream", Qt::QueuedConnection,
                                                  Q_ARG(QByteArray, incomingPacket));
                    } else {
                        QMetaObject::invokeMethod(&application->_audio, "parseAudioStreamStatsPacket", Qt::QueuedConnection,
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
                case PacketTypeParticleAddResponse:
                    // this will keep creatorTokenIDs to IDs mapped correctly
                    Particle::handleAddParticleResponse(incomingPacket);
                    application->getParticles()->getTree()->handleAddParticleResponse(incomingPacket);
                    break;
                case PacketTypeEntityAddResponse:
                    // this will keep creatorTokenIDs to IDs mapped correctly
                    EntityItemID::handleAddEntityResponse(incomingPacket);
                    application->getEntities()->getTree()->handleAddEntityResponse(incomingPacket);
                    break;
                case PacketTypeParticleData:
                case PacketTypeParticleErase:
                case PacketTypeEntityData:
                case PacketTypeEntityErase:
                case PacketTypeVoxelData:
                case PacketTypeVoxelErase:
                case PacketTypeOctreeStats:
                case PacketTypeEnvironmentData: {
                    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                                            "Application::networkReceive()... _octreeProcessor.queueReceivedPacket()");
                    bool wantExtraDebugging = application->getLogger()->extraDebugging();
                    if (wantExtraDebugging && packetTypeForPacket(incomingPacket) == PacketTypeVoxelData) {
                        int numBytesPacketHeader = numBytesForPacketHeader(incomingPacket);
                        unsigned char* dataAt = reinterpret_cast<unsigned char*>(incomingPacket.data()) + numBytesPacketHeader;
                        dataAt += sizeof(OCTREE_PACKET_FLAGS);
                        OCTREE_PACKET_SEQUENCE sequence = (*(OCTREE_PACKET_SEQUENCE*)dataAt);
                        dataAt += sizeof(OCTREE_PACKET_SEQUENCE);
                        OCTREE_PACKET_SENT_TIME sentAt = (*(OCTREE_PACKET_SENT_TIME*)dataAt);
                        dataAt += sizeof(OCTREE_PACKET_SENT_TIME);
                        OCTREE_PACKET_SENT_TIME arrivedAt = usecTimestampNow();
                        int flightTime = arrivedAt - sentAt;
                        
                        qDebug("got an Octree data or erase message, sequence:%d flightTime:%d", sequence, flightTime);
                    }
                    
                    SharedNodePointer matchedNode = NodeList::getInstance()->sendingNodeForPacket(incomingPacket);
                    
                    if (matchedNode) {
                        // add this packet to our list of voxel packets and process them on the voxel processing
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
                    AccountManager::getInstance().checkAndSignalForAccessToken();
                    break;
                }
                case PacketTypeMuteEnvironment: {
                    glm::vec3 position;
                    float radius;
                    
                    int headerSize = numBytesForPacketHeaderGivenPacketType(PacketTypeMuteEnvironment);
                    memcpy(&position, incomingPacket.constData() + headerSize, sizeof(glm::vec3));
                    memcpy(&radius, incomingPacket.constData() + headerSize + sizeof(glm::vec3), sizeof(float));
                    
                    if (glm::distance(Application::getInstance()->getAvatar()->getPosition(), position) < radius
                        && !Application::getInstance()->getAudio()->getMuted()) {
                        Application::getInstance()->getAudio()->toggleMute();
                    }
                    break;
                }
                case PacketTypeVoxelEditNack:
                    if (!Menu::getInstance()->isOptionChecked(MenuOption::DisableNackPackets)) {
                        application->_voxelEditSender.processNackPacket(incomingPacket);
                    }
                    break;
                case PacketTypeParticleEditNack:
                    if (!Menu::getInstance()->isOptionChecked(MenuOption::DisableNackPackets)) {
                        application->_particleEditSender.processNackPacket(incomingPacket);
                    }
                    break;
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
