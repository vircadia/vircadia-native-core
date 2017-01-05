//
//  EntityScriptServer.cpp
//  assignment-client/src/scripts
//
//  Created by Clément Brisset on 1/5/17.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntityScriptServer.h"

#include <AudioInjectorManager.h>
#include <EntityScriptingInterface.h>
#include <MessagesClient.h>
#include <plugins/CodecPlugin.h>
#include <plugins/PluginManager.h>
#include <ResourceManager.h>
#include <ScriptCache.h>
#include <ScriptEngines.h>
#include <SoundCache.h>

#include "../entities/AssignmentParentFinder.h"

static const int RECEIVED_AUDIO_STREAM_CAPACITY_FRAMES = 10;

EntityScriptServer::EntityScriptServer(ReceivedMessage& message) :
    ThreadedAssignment(message),
    _receivedAudioStream(RECEIVED_AUDIO_STREAM_CAPACITY_FRAMES, RECEIVED_AUDIO_STREAM_CAPACITY_FRAMES)
{
    DependencyManager::get<EntityScriptingInterface>()->setPacketSender(&_entityEditSender);

    ResourceManager::init();

    DependencyManager::registerInheritance<SpatialParentFinder, AssignmentParentFinder>();

    DependencyManager::set<ResourceCacheSharedItems>();
    DependencyManager::set<SoundCache>();
    DependencyManager::set<AudioInjectorManager>();

    DependencyManager::set<ScriptCache>();
    DependencyManager::set<ScriptEngines>();


    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();

    packetReceiver.registerListenerForTypes({ PacketType::MixedAudio, PacketType::SilentAudioFrame },
                                            this, "handleAudioPacket");
    packetReceiver.registerListenerForTypes({ PacketType::OctreeStats, PacketType::EntityData, PacketType::EntityErase },
                                            this, "handleOctreePacket");
    packetReceiver.registerListener(PacketType::Jurisdiction, this, "handleJurisdictionPacket");
    packetReceiver.registerListener(PacketType::SelectedAudioFormat, this, "handleSelectedAudioFormat");

}

static const QString ENTITY_SCRIPT_SERVER_LOGGING_NAME = "entity-script-server";

void EntityScriptServer::run() {
    // make sure we request our script once the agent connects to the domain
    auto nodeList = DependencyManager::get<NodeList>();

    ThreadedAssignment::commonInit(ENTITY_SCRIPT_SERVER_LOGGING_NAME, NodeType::EntityScriptServer);

    // Setup MessagesClient
    auto messagesClient = DependencyManager::set<MessagesClient>();
    QThread* messagesThread = new QThread;
    messagesThread->setObjectName("Messages Client Thread");
    messagesClient->moveToThread(messagesThread);
    connect(messagesThread, &QThread::started, messagesClient.data(), &MessagesClient::init);
    messagesThread->start();

    // make sure we hear about connected nodes so we can grab an ATP script if a request is pending
    connect(nodeList.data(), &LimitedNodeList::nodeActivated, this, &EntityScriptServer::nodeActivated);
    connect(nodeList.data(), &LimitedNodeList::nodeKilled, this, &EntityScriptServer::nodeKilled);

    nodeList->addSetOfNodeTypesToNodeInterestSet({
        NodeType::AudioMixer, NodeType::AvatarMixer, NodeType::EntityServer, NodeType::MessagesMixer, NodeType::AssetServer
    });
}

void EntityScriptServer::nodeActivated(SharedNodePointer activatedNode) {
    if (activatedNode->getType() == NodeType::AudioMixer) {
        negotiateAudioFormat();
    }
}

void EntityScriptServer::negotiateAudioFormat() {
    auto nodeList = DependencyManager::get<NodeList>();
    auto negotiateFormatPacket = NLPacket::create(PacketType::NegotiateAudioFormat);
    auto codecPlugins = PluginManager::getInstance()->getCodecPlugins();
    quint8 numberOfCodecs = (quint8)codecPlugins.size();
    negotiateFormatPacket->writePrimitive(numberOfCodecs);
    for (auto& plugin : codecPlugins) {
        auto codecName = plugin->getName();
        negotiateFormatPacket->writeString(codecName);
    }

    // grab our audio mixer from the NodeList, if it exists
    SharedNodePointer audioMixer = nodeList->soloNodeOfType(NodeType::AudioMixer);

    if (audioMixer) {
        // send off this mute packet
        nodeList->sendPacket(std::move(negotiateFormatPacket), *audioMixer);
    }
}

void EntityScriptServer::handleSelectedAudioFormat(QSharedPointer<ReceivedMessage> message) {
    QString selectedCodecName = message->readString();
    selectAudioFormat(selectedCodecName);
}

void EntityScriptServer::selectAudioFormat(const QString& selectedCodecName) {
    _selectedCodecName = selectedCodecName;

    qDebug() << "Selected Codec:" << _selectedCodecName;

    // release any old codec encoder/decoder first...
    if (_codec && _encoder) {
        _codec->releaseEncoder(_encoder);
        _encoder = nullptr;
        _codec = nullptr;
    }
    _receivedAudioStream.cleanupCodec();

    auto codecPlugins = PluginManager::getInstance()->getCodecPlugins();
    for (auto& plugin : codecPlugins) {
        if (_selectedCodecName == plugin->getName()) {
            _codec = plugin;
            _receivedAudioStream.setupCodec(plugin, _selectedCodecName, AudioConstants::STEREO);
            _encoder = plugin->createEncoder(AudioConstants::SAMPLE_RATE, AudioConstants::MONO);
            qDebug() << "Selected Codec Plugin:" << _codec.get();
            break;
        }
    }
}

void EntityScriptServer::handleAudioPacket(QSharedPointer<ReceivedMessage> message) {
    _receivedAudioStream.parseData(*message);

    _lastReceivedAudioLoudness = _receivedAudioStream.getNextOutputFrameLoudness();
    _receivedAudioStream.clearBuffer();
}

void EntityScriptServer::nodeKilled(SharedNodePointer killedNode) {

}

void EntityScriptServer::sendStatsPacket() {

}

void EntityScriptServer::handleOctreePacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {

}

void EntityScriptServer::handleJurisdictionPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {

}

void EntityScriptServer::aboutToFinish() {
    // our entity tree is going to go away so tell that to the EntityScriptingInterface
    DependencyManager::get<EntityScriptingInterface>()->setEntityTree(nullptr);

    ResourceManager::cleanup();

    // cleanup the AudioInjectorManager (and any still running injectors)
    DependencyManager::destroy<AudioInjectorManager>();
    DependencyManager::destroy<ScriptEngines>();

    // cleanup codec & encoder
    if (_codec && _encoder) {
        _codec->releaseEncoder(_encoder);
        _encoder = nullptr;
    }
}
