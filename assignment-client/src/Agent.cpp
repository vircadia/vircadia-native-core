//
//  Agent.cpp
//  hifi
//
//  Created by Stephen Birarda on 7/1/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include <AudioRingBuffer.h>
#include <AvatarData.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <UUID.h>
#include <VoxelConstants.h>
#include <ParticlesScriptingInterface.h>

#include "Agent.h"

Agent::Agent(const QByteArray& packet) :
    ThreadedAssignment(packet),
    _voxelEditSender(),
    _particleEditSender(),
    _receivedAudioBuffer(NETWORK_BUFFER_LENGTH_SAMPLES_STEREO)
{
    // be the parent of the script engine so it gets moved when we do
    _scriptEngine.setParent(this);
    
    _scriptEngine.getVoxelsScriptingInterface()->setPacketSender(&_voxelEditSender);
    _scriptEngine.getParticlesScriptingInterface()->setPacketSender(&_particleEditSender);
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

            } else if (datagramPacketType == PacketTypeParticleData
                        || datagramPacketType == PacketTypeParticleErase
                        || datagramPacketType == PacketTypeOctreeStats
                        || datagramPacketType == PacketTypeVoxelData
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

                if (datagramPacketType == PacketTypeVoxelData) {
                    _voxelViewer.processDatagram(mutablePacket, sourceNode);
                }

            } else if (datagramPacketType == PacketTypeMixedAudio) {
                // parse the data and grab the average loudness
                _receivedAudioBuffer.parseData(receivedPacket);
                
                // pretend like we have read the samples from this buffer so it does not fill
                static int16_t garbageAudioBuffer[NETWORK_BUFFER_LENGTH_SAMPLES_STEREO];
                _receivedAudioBuffer.readSamples(garbageAudioBuffer, NETWORK_BUFFER_LENGTH_SAMPLES_STEREO);
                
                // let this continue through to the NodeList so it updates last heard timestamp
                // for the sending audio mixer
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
                                                 << NodeType::ParticleServer);
    
    // figure out the URL for the script for this agent assignment
    QString scriptURLString("http://%1:8080/assignment/%2");
    scriptURLString = scriptURLString.arg(NodeList::getInstance()->getDomainInfo().getIP().toString(),
                                          uuidStringWithoutCurlyBraces(_uuid));
    
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    QNetworkReply *reply = networkManager->get(QNetworkRequest(QUrl(scriptURLString)));
    
    qDebug() << "Downloading script at" << scriptURLString;
    
    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    
    loop.exec();
    
    // let the AvatarData class use our QNetworkAcessManager
    AvatarData::setNetworkAccessManager(networkManager);
    
    QString scriptContents(reply->readAll());
    
    qDebug() << "Downloaded script:" << scriptContents;
    
    // setup an Avatar for the script to use
    AvatarData scriptedAvatar;
    
    // call model URL setters with empty URLs so our avatar, if user, will have the default models
    scriptedAvatar.setFaceModelURL(QUrl());
    scriptedAvatar.setSkeletonModelURL(QUrl());
    
    // give this AvatarData object to the script engine
    _scriptEngine.setAvatarData(&scriptedAvatar, "Avatar");
    
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

    _scriptEngine.setScriptContents(scriptContents);
    _scriptEngine.run();
    setFinished(true);
}

void Agent::aboutToFinish() {
    _scriptEngine.stop();
}
