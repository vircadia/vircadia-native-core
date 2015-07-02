//
//  Agent.cpp
//  assignment-client/src
//
//  Created by Stephen Birarda on 7/1/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>
#include <QtCore/QStandardPaths>
#include <QtNetwork/QNetworkDiskCache>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include <AvatarHashMap.h>
#include <NetworkAccessManager.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <ResourceCache.h>
#include <SoundCache.h>
#include <UUID.h>

#include <EntityScriptingInterface.h> // TODO: consider moving to scriptengine.h

#include "avatars/ScriptableAvatar.h"

#include "Agent.h"

static const int RECEIVED_AUDIO_STREAM_CAPACITY_FRAMES = 10;

Agent::Agent(const QByteArray& packet) :
    ThreadedAssignment(packet),
    _entityEditSender(),
    _receivedAudioStream(AudioConstants::NETWORK_FRAME_SAMPLES_STEREO, RECEIVED_AUDIO_STREAM_CAPACITY_FRAMES,
        InboundAudioStream::Settings(0, false, RECEIVED_AUDIO_STREAM_CAPACITY_FRAMES, false,
        DEFAULT_WINDOW_STARVE_THRESHOLD, DEFAULT_WINDOW_SECONDS_FOR_DESIRED_CALC_ON_TOO_MANY_STARVES,
        DEFAULT_WINDOW_SECONDS_FOR_DESIRED_REDUCTION, false))
{
    // be the parent of the script engine so it gets moved when we do
    _scriptEngine.setParent(this);
    
    DependencyManager::get<EntityScriptingInterface>()->setPacketSender(&_entityEditSender);

    DependencyManager::set<ResourceCacheSharedItems>();
    DependencyManager::set<SoundCache>();
}

void Agent::readPendingDatagrams() {
    QByteArray receivedPacket;
    HifiSockAddr senderSockAddr;
    auto nodeList = DependencyManager::get<NodeList>();
    
    while (readAvailableDatagram(receivedPacket, senderSockAddr)) {
        if (nodeList->packetVersionAndHashMatch(receivedPacket)) {
            PacketType::Value datagramPacketType = packetTypeForPacket(receivedPacket);
            
            if (datagramPacketType == PacketTypeJurisdiction) {
                int headerBytes = numBytesForPacketHeader(receivedPacket);
                
                SharedNodePointer matchedNode = nodeList->sendingNodeForPacket(receivedPacket);
                
                if (matchedNode) {
                    // PacketType_JURISDICTION, first byte is the node type...
                    switch (receivedPacket[headerBytes]) {
                        case NodeType::EntityServer:
                            DependencyManager::get<EntityScriptingInterface>()->getJurisdictionListener()->
                                queueReceivedPacket(matchedNode, receivedPacket);
                            break;
                    }
                }
                
            } else if (datagramPacketType == PacketTypeOctreeStats
                        || datagramPacketType == PacketTypeEntityData
                        || datagramPacketType == PacketTypeEntityErase
            ) {
                // Make sure our Node and NodeList knows we've heard from this node.
                SharedNodePointer sourceNode = nodeList->sendingNodeForPacket(receivedPacket);
                sourceNode->setLastHeardMicrostamp(usecTimestampNow());

                QByteArray mutablePacket = receivedPacket;
                int messageLength = mutablePacket.size();

                if (datagramPacketType == PacketTypeOctreeStats) {

                    int statsMessageLength = OctreeHeadlessViewer::parseOctreeStats(mutablePacket, sourceNode);
                    if (messageLength > statsMessageLength) {
                        mutablePacket = mutablePacket.mid(statsMessageLength);
                        
                        // TODO: this needs to be fixed, the goal is to test the packet version for the piggyback, but
                        //       this is testing the version and hash of the original packet
                        //       need to use numBytesArithmeticCodingFromBuffer()...
                        if (!DependencyManager::get<NodeList>()->packetVersionAndHashMatch(receivedPacket)) {
                            return; // bail since piggyback data doesn't match our versioning
                        }
                    } else {
                        return; // bail since no piggyback data
                    }

                    datagramPacketType = packetTypeForPacket(mutablePacket);
                } // fall through to piggyback message

                if (datagramPacketType == PacketTypeEntityData || datagramPacketType == PacketTypeEntityErase) {
                    _entityViewer.processDatagram(mutablePacket, sourceNode);
                }
                
            } else if (datagramPacketType == PacketTypeMixedAudio || datagramPacketType == PacketTypeSilentAudioFrame) {

                _receivedAudioStream.parseData(receivedPacket);

                _lastReceivedAudioLoudness = _receivedAudioStream.getNextOutputFrameLoudness();

                _receivedAudioStream.clearBuffer();
                
                // let this continue through to the NodeList so it updates last heard timestamp
                // for the sending audio mixer
                DependencyManager::get<NodeList>()->processNodeData(senderSockAddr, receivedPacket);
            } else if (datagramPacketType == PacketTypeBulkAvatarData
                       || datagramPacketType == PacketTypeAvatarIdentity
                       || datagramPacketType == PacketTypeAvatarBillboard
                       || datagramPacketType == PacketTypeKillAvatar) {
                // let the avatar hash map process it
                DependencyManager::get<AvatarHashMap>()->processAvatarMixerDatagram(receivedPacket, nodeList->sendingNodeForPacket(receivedPacket));
                
                // let this continue through to the NodeList so it updates last heard timestamp
                // for the sending avatar-mixer
                DependencyManager::get<NodeList>()->processNodeData(senderSockAddr, receivedPacket);
            } else {
                DependencyManager::get<NodeList>()->processNodeData(senderSockAddr, receivedPacket);
            }
        }
    }
}

const QString AGENT_LOGGING_NAME = "agent";

void Agent::run() {
    ThreadedAssignment::commonInit(AGENT_LOGGING_NAME, NodeType::Agent);
    
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->addSetOfNodeTypesToNodeInterestSet(NodeSet()
                                                 << NodeType::AudioMixer
                                                 << NodeType::AvatarMixer
                                                 << NodeType::EntityServer
                                                );
    
    // figure out the URL for the script for this agent assignment
    QUrl scriptURL;
    if (_payload.isEmpty())  {
        scriptURL = QUrl(QString("http://%1:%2/assignment/%3")
            .arg(DependencyManager::get<NodeList>()->getDomainHandler().getIP().toString())
            .arg(DOMAIN_SERVER_HTTP_PORT)
            .arg(uuidStringWithoutCurlyBraces(_uuid)));
    } else {
        scriptURL = QUrl(_payload);
    }
   
    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkRequest networkRequest = QNetworkRequest(scriptURL);
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
    QNetworkReply* reply = networkAccessManager.get(networkRequest);
    
    QNetworkDiskCache* cache = new QNetworkDiskCache();
    QString cachePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    cache->setCacheDirectory(!cachePath.isEmpty() ? cachePath : "agentCache");
    networkAccessManager.setCache(cache);
    
    qDebug() << "Downloading script at" << scriptURL.toString();
    
    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    
    loop.exec();
    
    QString scriptContents(reply->readAll());
    delete reply;
    
    qDebug() << "Downloaded script:" << scriptContents;
    
    // setup an Avatar for the script to use
    ScriptableAvatar scriptedAvatar(&_scriptEngine);
    scriptedAvatar.setForceFaceTrackerConnected(true);

    // call model URL setters with empty URLs so our avatar, if user, will have the default models
    scriptedAvatar.setFaceModelURL(QUrl());
    scriptedAvatar.setSkeletonModelURL(QUrl());
    
    // give this AvatarData object to the script engine
    _scriptEngine.setAvatarData(&scriptedAvatar, "Avatar");
    _scriptEngine.setAvatarHashMap(DependencyManager::get<AvatarHashMap>().data(), "AvatarList");
    
    // register ourselves to the script engine
    _scriptEngine.registerGlobalObject("Agent", this);

    _scriptEngine.init(); // must be done before we set up the viewers
    
    _scriptEngine.registerGlobalObject("SoundCache", DependencyManager::get<SoundCache>().data());

    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();

    _scriptEngine.registerGlobalObject("EntityViewer", &_entityViewer);
    _entityViewer.setJurisdictionListener(entityScriptingInterface->getJurisdictionListener());
    _entityViewer.init();
    entityScriptingInterface->setEntityTree(_entityViewer.getTree());

    _scriptEngine.setScriptContents(scriptContents);
    _scriptEngine.run();
    setFinished(true);
}

void Agent::aboutToFinish() {
    _scriptEngine.stop();
    
    // our entity tree is going to go away so tell that to the EntityScriptingInterface
    DependencyManager::get<EntityScriptingInterface>()->setEntityTree(NULL);
}
