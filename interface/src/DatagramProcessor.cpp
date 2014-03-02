//
//  DatagramProcessor.cpp
//  hifi
//
//  Created by Stephen Birarda on 1/23/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QWeakPointer>

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
            // only process this packet if we have a match on the packet version
            switch (packetTypeForPacket(incomingPacket)) {
                case PacketTypeMixedAudio:
                    QMetaObject::invokeMethod(&application->_audio, "addReceivedAudioToBuffer", Qt::QueuedConnection,
                                              Q_ARG(QByteArray, incomingPacket));
                    break;
                    
                case PacketTypeParticleAddResponse:
                    // this will keep creatorTokenIDs to IDs mapped correctly
                    Particle::handleAddParticleResponse(incomingPacket);
                    application->getParticles()->getTree()->handleAddParticleResponse(incomingPacket);
                    break;
                    
                case PacketTypeParticleData:
                case PacketTypeParticleErase:
                case PacketTypeVoxelData:
                case PacketTypeVoxelErase:
                case PacketTypeOctreeStats:
                case PacketTypeEnvironmentData: {
                    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                                            "Application::networkReceive()... _voxelProcessor.queueReceivedPacket()");
                    
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
                        
                        printf("got PacketType_VOXEL_DATA, sequence:%d flightTime:%d\n", sequence, flightTime);
                    }
                    
                    SharedNodePointer matchedNode = NodeList::getInstance()->sendingNodeForPacket(incomingPacket);
                    
                    if (matchedNode) {
                        // add this packet to our list of voxel packets and process them on the voxel processing
                        application->_voxelProcessor.queueReceivedPacket(matchedNode, incomingPacket);
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
                default:
                    nodeList->processNodeData(senderSockAddr, incomingPacket);
                    break;
            }
        }
    }
}
