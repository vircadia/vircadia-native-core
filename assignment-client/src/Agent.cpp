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
#include <udt/PacketHeaders.h>
#include <ResourceCache.h>
#include <SoundCache.h>
#include <UUID.h>

#include <WebSocketServerClass.h>
#include <EntityScriptingInterface.h> // TODO: consider moving to scriptengine.h

#include "avatars/ScriptableAvatar.h"

#include "Agent.h"

static const int RECEIVED_AUDIO_STREAM_CAPACITY_FRAMES = 10;

Agent::Agent(NLPacket& packet) :
    ThreadedAssignment(packet),
    _entityEditSender(),
    _receivedAudioStream(AudioConstants::NETWORK_FRAME_SAMPLES_STEREO, RECEIVED_AUDIO_STREAM_CAPACITY_FRAMES,
        InboundAudioStream::Settings(0, false, RECEIVED_AUDIO_STREAM_CAPACITY_FRAMES, false,
        DEFAULT_WINDOW_STARVE_THRESHOLD, DEFAULT_WINDOW_SECONDS_FOR_DESIRED_CALC_ON_TOO_MANY_STARVES,
        DEFAULT_WINDOW_SECONDS_FOR_DESIRED_REDUCTION, false))
{
    // be the parent of the script engine so it gets moved when we do
    _scriptEngine.setParent(this);
    _scriptEngine.setIsAgent(true);
    
    DependencyManager::get<EntityScriptingInterface>()->setPacketSender(&_entityEditSender);

    DependencyManager::set<ResourceCacheSharedItems>();
    DependencyManager::set<SoundCache>();

    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    
    packetReceiver.registerListenerForTypes(
        { PacketType::MixedAudio, PacketType::SilentAudioFrame },
        this, "handleAudioPacket");
    packetReceiver.registerListenerForTypes(
        { PacketType::OctreeStats, PacketType::EntityData, PacketType::EntityErase },
        this, "handleOctreePacket");
    packetReceiver.registerListener(PacketType::Jurisdiction, this, "handleJurisdictionPacket");
}

void Agent::handleOctreePacket(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    auto packetType = packet->getType();

    if (packetType == PacketType::OctreeStats) {

        int statsMessageLength = OctreeHeadlessViewer::parseOctreeStats(packet, senderNode);
        if (packet->getPayloadSize() > statsMessageLength) {
            // pull out the piggybacked packet and create a new QSharedPointer<NLPacket> for it
            int piggyBackedSizeWithHeader = packet->getPayloadSize() - statsMessageLength;
            
            auto buffer = std::unique_ptr<char[]>(new char[piggyBackedSizeWithHeader]);
            memcpy(buffer.get(), packet->getPayload() + statsMessageLength, piggyBackedSizeWithHeader);

            auto newPacket = NLPacket::fromReceivedPacket(std::move(buffer), piggyBackedSizeWithHeader, packet->getSenderSockAddr());
            packet = QSharedPointer<NLPacket>(newPacket.release());
        } else {
            return; // bail since no piggyback data
        }

        packetType = packet->getType();
    } // fall through to piggyback message

    if (packetType == PacketType::EntityData || packetType == PacketType::EntityErase) {
        _entityViewer.processDatagram(*packet, senderNode);
    }
}

void Agent::handleJurisdictionPacket(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    NodeType_t nodeType;
    packet->peekPrimitive(&nodeType);

    // PacketType_JURISDICTION, first byte is the node type...
    if (nodeType == NodeType::EntityServer) {
        DependencyManager::get<EntityScriptingInterface>()->getJurisdictionListener()->
            queueReceivedPacket(packet, senderNode);
    }
} 

void Agent::handleAudioPacket(QSharedPointer<NLPacket> packet) {
    _receivedAudioStream.parseData(*packet);

    _lastReceivedAudioLoudness = _receivedAudioStream.getNextOutputFrameLoudness();

    _receivedAudioStream.clearBuffer();
}

const QString AGENT_LOGGING_NAME = "agent";
const int PING_INTERVAL = 1000;

void Agent::run() {
    ThreadedAssignment::commonInit(AGENT_LOGGING_NAME, NodeType::Agent);

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->addSetOfNodeTypesToNodeInterestSet(NodeSet()
                                                 << NodeType::AudioMixer
                                                 << NodeType::AvatarMixer
                                                 << NodeType::EntityServer
                                                );

    _pingTimer = new QTimer(this);
    connect(_pingTimer, SIGNAL(timeout()), SLOT(sendPingRequests()));
    _pingTimer->start(PING_INTERVAL);

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
    
    auto avatarHashMap = DependencyManager::set<AvatarHashMap>();
    _scriptEngine.registerGlobalObject("AvatarList", avatarHashMap.data());

    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerListener(PacketType::BulkAvatarData, avatarHashMap.data(), "processAvatarDataPacket");
    packetReceiver.registerListener(PacketType::KillAvatar, avatarHashMap.data(), "processKillAvatar");
    packetReceiver.registerListener(PacketType::AvatarIdentity, avatarHashMap.data(), "processAvatarIdentityPacket");
    packetReceiver.registerListener(PacketType::AvatarBillboard, avatarHashMap.data(), "processAvatarBillboardPacket");

    // register ourselves to the script engine
    _scriptEngine.registerGlobalObject("Agent", this);

    if (!_payload.isEmpty()) {
        _scriptEngine.setParentURL(_payload);
    }

    // FIXME -we shouldn't be calling this directly, it's normally called by run(), not sure why 
    // viewers would need this called.
    _scriptEngine.init(); // must be done before we set up the viewers

    _scriptEngine.registerGlobalObject("SoundCache", DependencyManager::get<SoundCache>().data());

    QScriptValue webSocketServerConstructorValue = _scriptEngine.newFunction(WebSocketServerClass::constructor);
    _scriptEngine.globalObject().setProperty("WebSocketServer", webSocketServerConstructorValue);

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

    _pingTimer->stop();
    delete _pingTimer;

    // our entity tree is going to go away so tell that to the EntityScriptingInterface
    DependencyManager::get<EntityScriptingInterface>()->setEntityTree(NULL);
}

void Agent::sendPingRequests() {
    auto nodeList = DependencyManager::get<NodeList>();

    nodeList->eachMatchingNode([](const SharedNodePointer& node)->bool {
        switch (node->getType()) {
        case NodeType::AvatarMixer:
        case NodeType::AudioMixer:
        case NodeType::EntityServer:
            return true;
        default:
            return false;
        }
    }, [nodeList](const SharedNodePointer& node) {
        nodeList->sendPacket(nodeList->constructPingPacket(), *node);
    });
}
