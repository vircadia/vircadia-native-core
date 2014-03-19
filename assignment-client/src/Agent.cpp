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
    _avatarAudioStream(NULL)
{
    // be the parent of the script engine so it gets moved when we do
    _scriptEngine.setParent(this);
    
    _scriptEngine.getVoxelsScriptingInterface()->setPacketSender(&_voxelEditSender);
    _scriptEngine.getParticlesScriptingInterface()->setPacketSender(&_particleEditSender);
}

Agent::~Agent() {
    delete _avatarAudioStream;
}

const int SCRIPT_AUDIO_BUFFER_SAMPLES = floor(((SCRIPT_DATA_CALLBACK_USECS * SAMPLE_RATE) / (1000 * 1000)) + 0.5);

void Agent::setSendAvatarAudioStream(bool sendAvatarAudioStream) {
    if (sendAvatarAudioStream) {
        // the agentAudioStream number of samples is related to the ScriptEngine callback rate
        _avatarAudioStream = new int16_t[SCRIPT_AUDIO_BUFFER_SAMPLES];
        
        // fill the _audioStream with zeroes to start
        memset(_avatarAudioStream, 0, SCRIPT_AUDIO_BUFFER_SAMPLES * sizeof(int16_t));
        
        _scriptEngine.setNumAvatarAudioBufferSamples(SCRIPT_AUDIO_BUFFER_SAMPLES);
        _scriptEngine.setAvatarAudioBuffer(_avatarAudioStream);
    } else {
        delete _avatarAudioStream;
        _avatarAudioStream = NULL;
        
        _scriptEngine.setAvatarAudioBuffer(NULL);
    }
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

            } else {
                NodeList::getInstance()->processNodeData(senderSockAddr, receivedPacket);
            }
        }
    }
}

void Agent::run() {
    NodeList* nodeList = NodeList::getInstance();
    nodeList->setOwnerType(NodeType::Agent);
    
    nodeList->addSetOfNodeTypesToNodeInterestSet(NodeSet() << NodeType::AudioMixer << NodeType::AvatarMixer);
    
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
    
    timeval startTime;
    gettimeofday(&startTime, NULL);
    
    QTimer* domainServerTimer = new QTimer(this);
    connect(domainServerTimer, SIGNAL(timeout()), this, SLOT(checkInWithDomainServerOrExit()));
    domainServerTimer->start(DOMAIN_SERVER_CHECK_IN_USECS / 1000);
    
    QTimer* silentNodeTimer = new QTimer(this);
    connect(silentNodeTimer, SIGNAL(timeout()), nodeList, SLOT(removeSilentNodes()));
    silentNodeTimer->start(NODE_SILENCE_THRESHOLD_USECS / 1000);
    
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
}
