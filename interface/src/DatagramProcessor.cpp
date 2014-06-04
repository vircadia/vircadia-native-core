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

#include <PerfStat.h>

#include "Application.h"
#include "Menu.h"
#include "ui/OAuthWebViewHandler.h"

#include "DatagramProcessor.h"

DatagramProcessor::DatagramProcessor(QObject* parent) :
    QObject(parent)
{
    
}


// DEBUG

int DatagramProcessor::skewsI[10000];
int DatagramProcessor::S = 0;

unsigned char DatagramProcessor::typesI[10000];
int DatagramProcessor::diffsI[10000];
int DatagramProcessor::I = 0;



quint64 DatagramProcessor::prevTime = 0;

unsigned char DatagramProcessor::typesA[100];
quint64 DatagramProcessor::timesA[100];
int DatagramProcessor::A = 1;

unsigned char DatagramProcessor::typesB[100];
quint64 DatagramProcessor::timesB[100];
int DatagramProcessor::B = 1;

unsigned char* DatagramProcessor::currTypes = typesA;
unsigned char* DatagramProcessor::prevTypes = typesB;
quint64* DatagramProcessor::currTimes = timesA;
quint64* DatagramProcessor::prevTimes = timesB;
int* DatagramProcessor::currN = &A;
int* DatagramProcessor::prevN = &B;



void DatagramProcessor::processDatagrams() {

    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                            "DatagramProcessor::processDatagrams()");
    
    HifiSockAddr senderSockAddr;
    
    static QByteArray incomingPacket;
    
    Application* application = Application::getInstance();
    NodeList* nodeList = NodeList::getInstance();



prevTime = prevTimes[*prevN-1];

// swap
unsigned char* temp = currTypes;
currTypes = prevTypes;
prevTypes = temp;
// swap
quint64* temp2 = currTimes;
currTimes = prevTimes;
prevTimes = temp2;
// swap
int* temp3 = currN;
currN = prevN;
prevN = temp3;

// reset
*currN = 0;

int skew = 0;

    
    while (NodeList::getInstance()->getNodeSocket().hasPendingDatagrams()) {
        incomingPacket.resize(nodeList->getNodeSocket().pendingDatagramSize());
        nodeList->getNodeSocket().readDatagram(incomingPacket.data(), incomingPacket.size(),
                                               senderSockAddr.getAddressPointer(), senderSockAddr.getPortPointer());
        
        _packetCount++;
        _byteCount += incomingPacket.size();
        
        if (nodeList->packetVersionAndHashMatch(incomingPacket)) {

PacketType type = packetTypeForPacket(incomingPacket);
currTimes[*currN] = usecTimestampNow();
currTypes[*currN] = (unsigned char)type;
(*currN)++;


            // only process this packet if we have a match on the packet version
            switch (type) { //packetTypeForPacket(incomingPacket)) {
                case PacketTypeMixedAudio:
                    QMetaObject::invokeMethod(&application->_audio, "addReceivedAudioToBuffer", Qt::QueuedConnection,
                                              Q_ARG(QByteArray, incomingPacket));
                    break;
                    
                case PacketTypeParticleAddResponse:
                    // this will keep creatorTokenIDs to IDs mapped correctly
                    Particle::handleAddParticleResponse(incomingPacket);
                    application->getParticles()->getTree()->handleAddParticleResponse(incomingPacket);
                    break;
                case PacketTypeModelAddResponse:
                    // this will keep creatorTokenIDs to IDs mapped correctly
                    ModelItem::handleAddModelResponse(incomingPacket);
                    application->getModels()->getTree()->handleAddModelResponse(incomingPacket);
                    break;
                case PacketTypeParticleData:
                case PacketTypeParticleErase:
                case PacketTypeModelData:
                case PacketTypeModelErase:
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
                        
                        qDebug("got PacketType_VOXEL_DATA, sequence:%d flightTime:%d", sequence, flightTime);
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
                case PacketTypeDomainOAuthRequest: {
                    QDataStream readStream(incomingPacket);
                    readStream.skipRawData(numBytesForPacketHeader(incomingPacket));
                    
                    QUrl authorizationURL;
                    readStream >> authorizationURL;
                    
                    QMetaObject::invokeMethod(&OAuthWebViewHandler::getInstance(), "displayWebviewForAuthorizationURL",
                                              Q_ARG(const QUrl&, authorizationURL));
                    
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
                default:
                    int s = nodeList->processNodeData(senderSockAddr, incomingPacket);
                    if (s!=1234567890 && abs(s) > abs(skew))
                        skew = s;
                    break;
            }
        }
    }


    if (abs(skew) > 3000) {

        printf("large skew! %d ----------------------------\n", skew);

        skewsI[S++] = skew;

        /*
        printf("prev:::::::::::::::::::::::::::::::::::::\n");

        printf("\t type: %d  time: %llu  diff: %llu\n", prevTypes[0], prevTimes[0] % 100000000, prevTimes[0] - prevTime);
        for (int i = 1; i < *prevN; i++) {
            printf("\t type: %d  time: %llu  diff: %llu\n", prevTypes[i], prevTimes[i] % 100000000, prevTimes[i] - prevTimes[i - 1]);
        }

        printf("curr:::::::::::::::::::::::::::::::::::::\n");

        printf("\t type: %d  time: %llu  diff: %llu\n", currTypes[0], currTimes[0] % 100000000, currTimes[0] - prevTimes[*prevN - 1]);
        for (int i = 1; i < *currN; i++) {
            printf("\t type: %d  time: %llu  diff: %llu\n", currTypes[i], currTimes[i] % 100000000, currTimes[i] - currTimes[i - 1]);
        }*/

        diffsI[I++] = -2;   // prev marker

        typesI[I] = prevTypes[0];
        diffsI[I++] = prevTimes[0] - prevTime;
        for (int i = 1; i < *prevN; i++) {
            typesI[I] = prevTypes[i];
            diffsI[I++] = prevTimes[i] - prevTimes[i - 1];
        }


        diffsI[I++] = -1;   // curr marker

        typesI[I] = currTypes[0];
        diffsI[I++] = currTimes[0] - prevTimes[*prevN - 1];
        for (int i = 1; i < *currN; i++) {
            typesI[I] = currTypes[i];
            diffsI[I++] = currTimes[i] - currTimes[i - 1];
        }
    }

    skew = 0;
}
