//
//  DatagramProcessor.cpp
//  hifi
//
//  Created by Stephen Birarda on 1/23/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

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
    ssize_t bytesReceived;
    
    static unsigned char incomingPacket[MAX_PACKET_SIZE];
    
    Application* application = Application::getInstance();
    
    while (NodeList::getInstance()->getNodeSocket().hasPendingDatagrams() &&
           (bytesReceived = NodeList::getInstance()->getNodeSocket().readDatagram((char*) incomingPacket,
                                                                                  MAX_PACKET_SIZE,
                                                                                  senderSockAddr.getAddressPointer(),
                                                                                  senderSockAddr.getPortPointer()))) {
        
        _packetCount++;
        _byteCount += bytesReceived;
        
        if (packetVersionMatch(incomingPacket)) {
            // only process this packet if we have a match on the packet version
            switch (incomingPacket[0]) {
                case PACKET_TYPE_TRANSMITTER_DATA_V2:
                    //  V2 = IOS transmitter app
                    application->_myTransmitter.processIncomingData(incomingPacket, bytesReceived);
                    
                    break;
                case PACKET_TYPE_MIXED_AUDIO:
                    QMetaObject::invokeMethod(&application->_audio, "addReceivedAudioToBuffer", Qt::QueuedConnection,
                                              Q_ARG(QByteArray, QByteArray((char*) incomingPacket, bytesReceived)));
                    break;
                    
                case PACKET_TYPE_PARTICLE_ADD_RESPONSE:
                    // this will keep creatorTokenIDs to IDs mapped correctly
                    Particle::handleAddParticleResponse(incomingPacket, bytesReceived);
                    break;
                    
                case PACKET_TYPE_PARTICLE_DATA:
                case PACKET_TYPE_PARTICLE_ERASE:
                case PACKET_TYPE_VOXEL_DATA:
                case PACKET_TYPE_VOXEL_ERASE:
                case PACKET_TYPE_OCTREE_STATS:
                case PACKET_TYPE_ENVIRONMENT_DATA: {
                    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                                            "Application::networkReceive()... _voxelProcessor.queueReceivedPacket()");
                    
                    bool wantExtraDebugging = application->getLogger()->extraDebugging();
                    if (wantExtraDebugging && incomingPacket[0] == PACKET_TYPE_VOXEL_DATA) {
                        int numBytesPacketHeader = numBytesForPacketHeader(incomingPacket);
                        unsigned char* dataAt = incomingPacket + numBytesPacketHeader;
                        dataAt += sizeof(VOXEL_PACKET_FLAGS);
                        VOXEL_PACKET_SEQUENCE sequence = (*(VOXEL_PACKET_SEQUENCE*)dataAt);
                        dataAt += sizeof(VOXEL_PACKET_SEQUENCE);
                        VOXEL_PACKET_SENT_TIME sentAt = (*(VOXEL_PACKET_SENT_TIME*)dataAt);
                        dataAt += sizeof(VOXEL_PACKET_SENT_TIME);
                        VOXEL_PACKET_SENT_TIME arrivedAt = usecTimestampNow();
                        int flightTime = arrivedAt - sentAt;
                        
                        printf("got PACKET_TYPE_VOXEL_DATA, sequence:%d flightTime:%d\n", sequence, flightTime);
                    }
                    
                    // add this packet to our list of voxel packets and process them on the voxel processing
                    application->_voxelProcessor.queueReceivedPacket(senderSockAddr, incomingPacket, bytesReceived);
                    break;
                }
                case PACKET_TYPE_METAVOXEL_DATA:
                    application->_metavoxels.processData(QByteArray((const char*) incomingPacket, bytesReceived),
                                                         senderSockAddr);
                    break;
                case PACKET_TYPE_BULK_AVATAR_DATA: {
                    // update having heard from the avatar-mixer and record the bytes received
                    SharedNodePointer avatarMixer = NodeList::getInstance()->nodeWithAddress(senderSockAddr);
                    
                    if (avatarMixer) {
                        avatarMixer->setLastHeardMicrostamp(usecTimestampNow());
                        avatarMixer->recordBytesReceived(bytesReceived);
                        
                        
                        QMetaObject::invokeMethod(&application->getAvatarManager(), "processAvatarMixerDatagram",
                                                  Q_ARG(const QByteArray&,
                                                        QByteArray(reinterpret_cast<char*>(incomingPacket), bytesReceived)));
                    }
                    
                    application->_bandwidthMeter.inputStream(BandwidthMeter::AVATARS).updateValue(bytesReceived);
                    break;
                }
                case PACKET_TYPE_DATA_SERVER_GET:
                case PACKET_TYPE_DATA_SERVER_PUT:
                case PACKET_TYPE_DATA_SERVER_SEND:
                case PACKET_TYPE_DATA_SERVER_CONFIRM:
                    DataServerClient::processMessageFromDataServer(incomingPacket, bytesReceived);
                    break;
                default:
                    NodeList::getInstance()->processNodeData(senderSockAddr, incomingPacket, bytesReceived);
                    break;
            }
        }
    }
}