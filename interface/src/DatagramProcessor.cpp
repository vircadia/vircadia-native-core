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
        
        if (packetVersionMatch(incomingPacket)) {
            // only process this packet if we have a match on the packet version
            switch (packetTypeForPacket(incomingPacket)) {
                case PacketTypeTransmitterData:
                    //  V2 = IOS transmitter app
                    application->_myTransmitter.processIncomingData(reinterpret_cast<unsigned char*>(incomingPacket.data()),
                                                                    incomingPacket.size());
                    
                    break;
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
                        dataAt += sizeof(VOXEL_PACKET_FLAGS);
                        VOXEL_PACKET_SEQUENCE sequence = (*(VOXEL_PACKET_SEQUENCE*)dataAt);
                        dataAt += sizeof(VOXEL_PACKET_SEQUENCE);
                        VOXEL_PACKET_SENT_TIME sentAt = (*(VOXEL_PACKET_SENT_TIME*)dataAt);
                        dataAt += sizeof(VOXEL_PACKET_SENT_TIME);
                        VOXEL_PACKET_SENT_TIME arrivedAt = usecTimestampNow();
                        int flightTime = arrivedAt - sentAt;
                        
                        printf("got PacketType_VOXEL_DATA, sequence:%d flightTime:%d\n", sequence, flightTime);
                    }
                    
                    // add this packet to our list of voxel packets and process them on the voxel processing
                    application->_voxelProcessor.queueReceivedPacket(senderSockAddr, incomingPacket);
                    break;
                }
                case PacketTypeMetavoxelData:
                    application->_metavoxels.processData(incomingPacket, senderSockAddr);
                    break;
                case PacketTypeBulkAvatarData:
                case PacketTypeKillAvatar: {
                    // update having heard from the avatar-mixer and record the bytes received
                    SharedNodePointer avatarMixer = NodeList::getInstance()->nodeWithAddress(senderSockAddr);
                    
                    if (avatarMixer) {
                        avatarMixer->setLastHeardMicrostamp(usecTimestampNow());
                        avatarMixer->recordBytesReceived(incomingPacket.size());
                        
                        if (packetTypeForPacket(incomingPacket) == PacketTypeBulkAvatarData) {
                            QMetaObject::invokeMethod(&application->getAvatarManager(), "processAvatarMixerDatagram",
                                                      Q_ARG(const QByteArray&, incomingPacket),
                                                      Q_ARG(const QWeakPointer<Node>&, avatarMixer));
                        } else {
                            // this is an avatar kill, pass it to the application AvatarManager
                            QMetaObject::invokeMethod(&application->getAvatarManager(), "processKillAvatar",
                                                      Q_ARG(const QByteArray&, incomingPacket));
                        }
                    }
                    
                    application->_bandwidthMeter.inputStream(BandwidthMeter::AVATARS).updateValue(incomingPacket.size());
                    break;
                }
                case PacketTypeDataServerGet:
                case PacketTypeDataServerPut:
                case PacketTypeDataServerSend:
                case PacketTypeDataServerConfirm:
                    DataServerClient::processMessageFromDataServer(incomingPacket);
                    break;
                default:
                    NodeList::getInstance()->processNodeData(senderSockAddr, incomingPacket);
                    break;
            }
        }
    }
}