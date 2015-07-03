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
#include "avatar/AvatarManager.h"
#include "AudioClient.h"
#include "Menu.h"
#include "InterfaceLogging.h"

#include "DatagramProcessor.h"

DatagramProcessor::DatagramProcessor(QObject* parent) :
    QObject(parent)
{

}

void DatagramProcessor::processDatagrams() {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                            "DatagramProcessor::processDatagrams()");

    if (_isShuttingDown) {
        return; // bail early... we're shutting down.
    }

    HifiSockAddr senderSockAddr;

    static QByteArray incomingPacket;

    Application* application = Application::getInstance();
    auto nodeList = DependencyManager::get<NodeList>();

    while (DependencyManager::get<NodeList>()->getNodeSocket().hasPendingDatagrams()) {
        incomingPacket.resize(nodeList->getNodeSocket().pendingDatagramSize());
        nodeList->readDatagram(incomingPacket, senderSockAddr.getAddressPointer(), senderSockAddr.getPortPointer());

        _inPacketCount++;
        _inByteCount += incomingPacket.size();

        if (nodeList->packetVersionAndHashMatch(incomingPacket)) {

            PacketType::Value incomingType = packetTypeForPacket(incomingPacket);
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
                    }

                    break;
                }
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
                case PacketTypeBulkAvatarData:
                case PacketTypeKillAvatar:
                case PacketTypeAvatarIdentity:
                case PacketTypeAvatarBillboard: {
                    // update having heard from the avatar-mixer and record the bytes received
                    SharedNodePointer avatarMixer = nodeList->sendingNodeForPacket(incomingPacket);

                    if (avatarMixer) {
                        avatarMixer->setLastHeardMicrostamp(usecTimestampNow());

                        QMetaObject::invokeMethod(DependencyManager::get<AvatarManager>().data(), "processAvatarMixerDatagram",
                                                  Q_ARG(const QByteArray&, incomingPacket),
                                                  Q_ARG(const QWeakPointer<Node>&, avatarMixer));
                    }
                    break;
                }
                case PacketTypeDomainConnectionDenied: {
                    int headerSize = numBytesForPacketHeaderGivenPacketType(PacketTypeDomainConnectionDenied);
                    QDataStream packetStream(QByteArray(incomingPacket.constData() + headerSize,
                                                        incomingPacket.size() - headerSize));
                    QString reason;
                    packetStream >> reason;

                    // output to the log so the user knows they got a denied connection request
                    // and check and signal for an access token so that we can make sure they are logged in
                    qCDebug(interfaceapp) << "The domain-server denied a connection request: " << reason;
                    qCDebug(interfaceapp) << "You may need to re-log to generate a keypair so you can provide a username signature.";
                    application->domainConnectionDenied(reason);
                    AccountManager::getInstance().checkAndSignalForAccessToken();
                    break;
                }
                case PacketTypeNoisyMute:
                case PacketTypeMuteEnvironment: {
                    bool mute = !DependencyManager::get<AudioClient>()->isMuted();

                    if (incomingType == PacketTypeMuteEnvironment) {
                        glm::vec3 position;
                        float radius;

                        int headerSize = numBytesForPacketHeaderGivenPacketType(PacketTypeMuteEnvironment);
                        memcpy(&position, incomingPacket.constData() + headerSize, sizeof(glm::vec3));
                        memcpy(&radius, incomingPacket.constData() + headerSize + sizeof(glm::vec3), sizeof(float));
                        float distance = glm::distance(DependencyManager::get<AvatarManager>()->getMyAvatar()->getPosition(),
                                                 position);

                        mute = mute && (distance < radius);
                    }

                    if (mute) {
                        DependencyManager::get<AudioClient>()->toggleMute();
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
