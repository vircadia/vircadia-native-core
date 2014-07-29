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
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkDiskCache>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include <AudioRingBuffer.h>
#include <AvatarData.h>
#include <NetworkAccessManager.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <ResourceCache.h>
#include <UUID.h>
#include <VoxelConstants.h>

#include <ParticlesScriptingInterface.h> // TODO: consider moving to scriptengine.h
#include <ModelsScriptingInterface.h> // TODO: consider moving to scriptengine.h

#include "avatars/ScriptableAvatar.h"

#include "Agent.h"

Agent::Agent(const QByteArray& packet) :
    ThreadedAssignment(packet),
    _voxelEditSender(),
    _particleEditSender(),
    _modelEditSender(),
    _receivedAudioBuffer(NETWORK_BUFFER_LENGTH_SAMPLES_STEREO),
    _avatarHashMap()
{
    // be the parent of the script engine so it gets moved when we do
    _scriptEngine.setParent(this);
    
    _scriptEngine.getVoxelsScriptingInterface()->setPacketSender(&_voxelEditSender);
    _scriptEngine.getParticlesScriptingInterface()->setPacketSender(&_particleEditSender);
    _scriptEngine.getModelsScriptingInterface()->setPacketSender(&_modelEditSender);
}

void Agent::readPendingDatagrams() {
    QByteArray receivedPacket;
    HifiSockAddr senderSockAddr;
    NodeList* nodeList = NodeList::getInstance();
    
    while (readAvailableDatagram(receivedPacket, senderSockAddr)) {
        if (nodeList->packetVersionAndHashMatch(receivedPacket)) {
            PacketType datagramPacketType = packetTypeForPacket(receivedPacket);
            
            if (datagramPacketType == PacketTypeJurisdiction) {
                int headerBytes = numBytesForPacketHeader(receivedPacket);
                
                SharedNodePointer matchedNode = nodeList->sendingNodeForPacket(receivedPacket);
                
                if (matchedNode) {
                    // PacketType_JURISDICTION, first byte is the node type...
                    switch (receivedPacket[headerBytes]) {
                        case NodeType::VoxelServer:
                            _scriptEngine.getVoxelsScriptingInterface()->getJurisdictionListener()->
                                                                queueReceivedPacket(matchedNode,receivedPacket);
                            break;
                        case NodeType::ParticleServer:
                            _scriptEngine.getParticlesScriptingInterface()->getJurisdictionListener()->
                                                                queueReceivedPacket(matchedNode, receivedPacket);
                            break;
                        case NodeType::ModelServer:
                            _scriptEngine.getModelsScriptingInterface()->getJurisdictionListener()->
                                                                queueReceivedPacket(matchedNode, receivedPacket);
                            break;
                    }
                }
                
            } else if (datagramPacketType == PacketTypeParticleAddResponse) {
                // this will keep creatorTokenIDs to IDs mapped correctly
                Particle::handleAddParticleResponse(receivedPacket);
                
                // also give our local particle tree a chance to remap any internal locally created particles
                _particleViewer.getTree()->handleAddParticleResponse(receivedPacket);

                // Make sure our Node and NodeList knows we've heard from this node.
                SharedNodePointer sourceNode = nodeList->sendingNodeForPacket(receivedPacket);
                sourceNode->setLastHeardMicrostamp(usecTimestampNow());

            } else if (datagramPacketType == PacketTypeModelAddResponse) {
                // this will keep creatorTokenIDs to IDs mapped correctly
                ModelItem::handleAddModelResponse(receivedPacket);
                
                // also give our local particle tree a chance to remap any internal locally created particles
                _modelViewer.getTree()->handleAddModelResponse(receivedPacket);

                // Make sure our Node and NodeList knows we've heard from this node.
                SharedNodePointer sourceNode = nodeList->sendingNodeForPacket(receivedPacket);
                sourceNode->setLastHeardMicrostamp(usecTimestampNow());

            } else if (datagramPacketType == PacketTypeParticleData
                        || datagramPacketType == PacketTypeParticleErase
                        || datagramPacketType == PacketTypeOctreeStats
                        || datagramPacketType == PacketTypeVoxelData
                        || datagramPacketType == PacketTypeModelData
                        || datagramPacketType == PacketTypeModelErase
            ) {
                // Make sure our Node and NodeList knows we've heard from this node.
                SharedNodePointer sourceNode = nodeList->sendingNodeForPacket(receivedPacket);
                sourceNode->setLastHeardMicrostamp(usecTimestampNow());

                QByteArray mutablePacket = receivedPacket;
                ssize_t messageLength = mutablePacket.size();

                if (datagramPacketType == PacketTypeOctreeStats) {

                    int statsMessageLength = OctreeHeadlessViewer::parseOctreeStats(mutablePacket, sourceNode);
                    if (messageLength > statsMessageLength) {
                        mutablePacket = mutablePacket.mid(statsMessageLength);
                        
                        // TODO: this needs to be fixed, the goal is to test the packet version for the piggyback, but
                        //       this is testing the version and hash of the original packet
                        //       need to use numBytesArithmeticCodingFromBuffer()...
                        if (!NodeList::getInstance()->packetVersionAndHashMatch(receivedPacket)) {
                            return; // bail since piggyback data doesn't match our versioning
                        }
                    } else {
                        return; // bail since no piggyback data
                    }

                    datagramPacketType = packetTypeForPacket(mutablePacket);
                } // fall through to piggyback message

                if (datagramPacketType == PacketTypeParticleData || datagramPacketType == PacketTypeParticleErase) {
                    _particleViewer.processDatagram(mutablePacket, sourceNode);
                }

                if (datagramPacketType == PacketTypeModelData || datagramPacketType == PacketTypeModelErase) {
                    _modelViewer.processDatagram(mutablePacket, sourceNode);
                }
                
                if (datagramPacketType == PacketTypeVoxelData) {
                    _voxelViewer.processDatagram(mutablePacket, sourceNode);
                }

            } else if (datagramPacketType == PacketTypeMixedAudio) {

                QUuid senderUUID = uuidFromPacketHeader(receivedPacket);

                // parse sequence number for this packet
                int numBytesPacketHeader = numBytesForPacketHeader(receivedPacket);
                const char* sequenceAt = receivedPacket.constData() + numBytesPacketHeader;
                quint16 sequence = *(reinterpret_cast<const quint16*>(sequenceAt));
                _incomingMixedAudioSequenceNumberStats.sequenceNumberReceived(sequence, senderUUID);

                // parse the data and grab the average loudness
                _receivedAudioBuffer.parseData(receivedPacket);
                
                // pretend like we have read the samples from this buffer so it does not fill
                static int16_t garbageAudioBuffer[NETWORK_BUFFER_LENGTH_SAMPLES_STEREO];
                _receivedAudioBuffer.readSamples(garbageAudioBuffer, NETWORK_BUFFER_LENGTH_SAMPLES_STEREO);
                
                // let this continue through to the NodeList so it updates last heard timestamp
                // for the sending audio mixer
                NodeList::getInstance()->processNodeData(senderSockAddr, receivedPacket);
            } else if (datagramPacketType == PacketTypeBulkAvatarData
                       || datagramPacketType == PacketTypeAvatarIdentity
                       || datagramPacketType == PacketTypeAvatarBillboard
                       || datagramPacketType == PacketTypeKillAvatar) {
                // let the avatar hash map process it
                _avatarHashMap.processAvatarMixerDatagram(receivedPacket, nodeList->sendingNodeForPacket(receivedPacket));
                
                // let this continue through to the NodeList so it updates last heard timestamp
                // for the sending avatar-mixer
                NodeList::getInstance()->processNodeData(senderSockAddr, receivedPacket);
            } else {
                NodeList::getInstance()->processNodeData(senderSockAddr, receivedPacket);
            }
        }
    }
}

const QString AGENT_LOGGING_NAME = "agent";

void Agent::run() {
    ThreadedAssignment::commonInit(AGENT_LOGGING_NAME, NodeType::Agent);
    
    NodeList* nodeList = NodeList::getInstance();
    nodeList->addSetOfNodeTypesToNodeInterestSet(NodeSet()
                                                 << NodeType::AudioMixer
                                                 << NodeType::AvatarMixer
                                                 << NodeType::VoxelServer
                                                 << NodeType::ParticleServer
                                                 << NodeType::ModelServer
                                                );
    
    // figure out the URL for the script for this agent assignment
    QUrl scriptURL;
    if (_payload.isEmpty())  {
        scriptURL = QUrl(QString("http://%1:%2/assignment/%3")
            .arg(NodeList::getInstance()->getDomainHandler().getIP().toString())
            .arg(DOMAIN_SERVER_HTTP_PORT)
            .arg(uuidStringWithoutCurlyBraces(_uuid)));
    } else {
        scriptURL = QUrl(_payload);
    }
   
    NetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkReply *reply = networkAccessManager.get(QNetworkRequest(scriptURL));
    
    QNetworkDiskCache* cache = new QNetworkDiskCache();
    QString cachePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    cache->setCacheDirectory(!cachePath.isEmpty() ? cachePath : "agentCache");
    networkAccessManager.setCache(cache);
    
    qDebug() << "Downloading script at" << scriptURL.toString();
    
    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    
    loop.exec();
    
    QString scriptContents(reply->readAll());
    
    qDebug() << "Downloaded script:" << scriptContents;
    
    // setup an Avatar for the script to use
    ScriptableAvatar scriptedAvatar(&_scriptEngine);
    scriptedAvatar.setForceFaceshiftConnected(true);

    // call model URL setters with empty URLs so our avatar, if user, will have the default models
    scriptedAvatar.setFaceModelURL(QUrl());
    scriptedAvatar.setSkeletonModelURL(QUrl());
    
    // give this AvatarData object to the script engine
    _scriptEngine.setAvatarData(&scriptedAvatar, "Avatar");
    _scriptEngine.setAvatarHashMap(&_avatarHashMap, "AvatarList");
    
    // register ourselves to the script engine
    _scriptEngine.registerGlobalObject("Agent", this);

    _scriptEngine.init(); // must be done before we set up the viewers

    _scriptEngine.registerGlobalObject("VoxelViewer", &_voxelViewer);
    // connect the VoxelViewer and the VoxelScriptingInterface to each other
    JurisdictionListener* voxelJL = _scriptEngine.getVoxelsScriptingInterface()->getJurisdictionListener();
    _voxelViewer.setJurisdictionListener(voxelJL);
    _voxelViewer.init();
    _scriptEngine.getVoxelsScriptingInterface()->setVoxelTree(_voxelViewer.getTree());
    
    _scriptEngine.registerGlobalObject("ParticleViewer", &_particleViewer);
    JurisdictionListener* particleJL = _scriptEngine.getParticlesScriptingInterface()->getJurisdictionListener();
    _particleViewer.setJurisdictionListener(particleJL);
    _particleViewer.init();
    _scriptEngine.getParticlesScriptingInterface()->setParticleTree(_particleViewer.getTree());

    _scriptEngine.registerGlobalObject("ModelViewer", &_modelViewer);
    JurisdictionListener* modelJL = _scriptEngine.getModelsScriptingInterface()->getJurisdictionListener();
    _modelViewer.setJurisdictionListener(modelJL);
    _modelViewer.init();
    _scriptEngine.getModelsScriptingInterface()->setModelTree(_modelViewer.getTree());

    _scriptEngine.setScriptContents(scriptContents);
    _scriptEngine.run();
    setFinished(true);
}

void Agent::aboutToFinish() {
    _scriptEngine.stop();
    NetworkAccessManager::getInstance().clearAccessCache();
}
