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

#include <AudioConstants.h>
#include <AudioInjectorManager.h>
#include <EntityScriptingInterface.h>
#include <MessagesClient.h>
#include <plugins/CodecPlugin.h>
#include <plugins/PluginManager.h>
#include <ResourceManager.h>
#include <ScriptCache.h>
#include <ScriptEngines.h>
#include <SoundCache.h>
#include <WebSocketServerClass.h>

#include "../entities/AssignmentParentFinder.h"

const size_t UUID_LENGTH_BYTES = 16;

int EntityScriptServer::_entitiesScriptEngineCount = 0;

EntityScriptServer::EntityScriptServer(ReceivedMessage& message) : ThreadedAssignment(message) {
    DependencyManager::get<EntityScriptingInterface>()->setPacketSender(&_entityEditSender);

    ResourceManager::init();

    DependencyManager::registerInheritance<SpatialParentFinder, AssignmentParentFinder>();

    DependencyManager::set<ResourceCacheSharedItems>();
    DependencyManager::set<SoundCache>();
    DependencyManager::set<AudioInjectorManager>();

    DependencyManager::set<ScriptCache>();
    DependencyManager::set<ScriptEngines>();


    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerListenerForTypes({ PacketType::OctreeStats, PacketType::EntityData, PacketType::EntityErase },
                                            this, "handleOctreePacket");
    packetReceiver.registerListener(PacketType::Jurisdiction, this, "handleJurisdictionPacket");
    packetReceiver.registerListener(PacketType::SelectedAudioFormat, this, "handleSelectedAudioFormat");

    auto avatarHashMap = DependencyManager::set<AvatarHashMap>();
    packetReceiver.registerListener(PacketType::BulkAvatarData, avatarHashMap.data(), "processAvatarDataPacket");
    packetReceiver.registerListener(PacketType::KillAvatar, avatarHashMap.data(), "processKillAvatar");
    packetReceiver.registerListener(PacketType::AvatarIdentity, avatarHashMap.data(), "processAvatarIdentityPacket");

    packetReceiver.registerListener(PacketType::ReloadEntityServerScript, this, "handleReloadEntityServerScriptPacket");
    packetReceiver.registerListener(PacketType::EntityScriptGetStatus, this, "handleEntityScriptGetStatusPacket");
}

static const QString ENTITY_SCRIPT_SERVER_LOGGING_NAME = "entity-script-server";

void EntityScriptServer::handleReloadEntityServerScriptPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    auto entityID = QUuid::fromRfc4122(message->read(UUID_LENGTH_BYTES));

    if (_entityViewer.getTree() && !_shuttingDown) {
        qDebug() << "Reloading: " << entityID;
        _entitiesScriptEngine->unloadEntityScript(entityID);
        checkAndCallPreload(entityID, true);
    }
}

void EntityScriptServer::handleEntityScriptGetStatusPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    MessageID messageID;
    message->readPrimitive(&messageID);
    auto entityID = QUuid::fromRfc4122(message->read(UUID_LENGTH_BYTES));

    // TODO(Huffman) Get Status
    
    qDebug() << "Getting script status of: " << entityID;
    auto replyPacket = NLPacket::create(PacketType::EntityScriptGetStatusReply, -1, true);
    replyPacket->writePrimitive(messageID);
    replyPacket->writeString("running");

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->sendPacket(std::move(replyPacket), *senderNode);
}

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
        NodeType::Agent, NodeType::AudioMixer, NodeType::AvatarMixer,
        NodeType::EntityServer, NodeType::MessagesMixer, NodeType::AssetServer
    });

    // Setup Script Engine
    resetEntitiesScriptEngine();

    // we need to make sure that init has been called for our EntityScriptingInterface
    // so that it actually has a jurisdiction listener when we ask it for it next
    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
    entityScriptingInterface->init();
    _entityViewer.setJurisdictionListener(entityScriptingInterface->getJurisdictionListener());

    _entityViewer.init();

    entityScriptingInterface->setEntityTree(_entityViewer.getTree());

    DependencyManager::set<AssignmentParentFinder>(_entityViewer.getTree());


    auto tree = _entityViewer.getTree().get();
    connect(tree, &EntityTree::deletingEntity, this, &EntityScriptServer::deletingEntity, Qt::QueuedConnection);
    connect(tree, &EntityTree::addingEntity, this, &EntityScriptServer::addingEntity, Qt::QueuedConnection);
    connect(tree, &EntityTree::entityScriptChanging, this, &EntityScriptServer::entitySciptChanging, Qt::QueuedConnection);
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

    auto codecPlugins = PluginManager::getInstance()->getCodecPlugins();
    for (auto& plugin : codecPlugins) {
        if (_selectedCodecName == plugin->getName()) {
            _codec = plugin;
            _encoder = plugin->createEncoder(AudioConstants::SAMPLE_RATE, AudioConstants::MONO);
            qDebug() << "Selected Codec Plugin:" << _codec.get();
            break;
        }
    }
}

void EntityScriptServer::resetEntitiesScriptEngine() {
    auto engineName = QString("Entities %1").arg(++_entitiesScriptEngineCount);
    auto newEngine = QSharedPointer<ScriptEngine>(new ScriptEngine(NO_SCRIPT, engineName));

    newEngine->registerGlobalObject("EntityViewer", &_entityViewer);

    auto webSocketServerConstructorValue = newEngine->newFunction(WebSocketServerClass::constructor);
    newEngine->globalObject().setProperty("WebSocketServer", webSocketServerConstructorValue);


//    newEngine->setEmitScriptUpdatesFunction([this]() {
//        SharedNodePointer entityServerNode = DependencyManager::get<NodeList>()->soloNodeOfType(NodeType::EntityServer);
//        return !entityServerNode || isPhysicsEnabled();
//    });

    newEngine->registerGlobalObject("AvatarList", DependencyManager::get<AvatarHashMap>().data());
    newEngine->registerGlobalObject("SoundCache", DependencyManager::get<SoundCache>().data());

    // connect this script engines printedMessage signal to the global ScriptEngines these various messages
    auto scriptEngines = DependencyManager::get<ScriptEngines>().data();
    connect(newEngine.data(), &ScriptEngine::printedMessage, scriptEngines, &ScriptEngines::onPrintedMessage);
    connect(newEngine.data(), &ScriptEngine::errorMessage, scriptEngines, &ScriptEngines::onErrorMessage);
    connect(newEngine.data(), &ScriptEngine::warningMessage, scriptEngines, &ScriptEngines::onWarningMessage);
    connect(newEngine.data(), &ScriptEngine::infoMessage, scriptEngines, &ScriptEngines::onInfoMessage);


    newEngine->runInThread();
    DependencyManager::get<EntityScriptingInterface>()->setEntitiesScriptEngine(newEngine.data());

    _entitiesScriptEngine.swap(newEngine);
}


void EntityScriptServer::clear() {
    // unload and stop the engine
    if (_entitiesScriptEngine) {
        // do this here (instead of in deleter) to avoid marshalling unload signals back to this thread
        _entitiesScriptEngine->unloadAllEntityScripts();
        _entitiesScriptEngine->stop();
    }

    // reset the engine
    if (!_shuttingDown) {
        resetEntitiesScriptEngine();
    }

    _entityViewer.clear();
}

void EntityScriptServer::shutdown() {
    if (_entitiesScriptEngine) {
        _entitiesScriptEngine->disconnectNonEssentialSignals(); // disconnect all slots/signals from the script engine, except essential
    }
    _shuttingDown = true;

    clear(); // always clear() on shutdown
}

void EntityScriptServer::addingEntity(const EntityItemID& entityID) {
    checkAndCallPreload(entityID);
}

void EntityScriptServer::deletingEntity(const EntityItemID& entityID) {
    if (_entityViewer.getTree() && !_shuttingDown && _entitiesScriptEngine) {
        _entitiesScriptEngine->unloadEntityScript(entityID);
    }
}

void EntityScriptServer::entitySciptChanging(const EntityItemID& entityID, const bool reload) {
    if (_entityViewer.getTree() && !_shuttingDown) {
        _entitiesScriptEngine->unloadEntityScript(entityID);
        checkAndCallPreload(entityID, reload);
    }
}

void EntityScriptServer::checkAndCallPreload(const EntityItemID& entityID, const bool reload) {
    if (_entityViewer.getTree() && !_shuttingDown) {
        EntityItemPointer entity = _entityViewer.getTree()->findEntityByEntityItemID(entityID);
        if (entity && entity->shouldPreloadScript() && _entitiesScriptEngine) {
            QString scriptUrl = entity->getScript();
            scriptUrl = ResourceManager::normalizeURL(scriptUrl);
            ScriptEngine::loadEntityScript(_entitiesScriptEngine, entityID, scriptUrl, reload);
            entity->scriptHasPreloaded();
        }
    }
}

void EntityScriptServer::nodeKilled(SharedNodePointer killedNode) {

}

void EntityScriptServer::sendStatsPacket() {

}

void EntityScriptServer::handleOctreePacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    auto packetType = message->getType();

    if (packetType == PacketType::OctreeStats) {

        int statsMessageLength = OctreeHeadlessViewer::parseOctreeStats(message, senderNode);
        if (message->getSize() > statsMessageLength) {
            // pull out the piggybacked packet and create a new QSharedPointer<NLPacket> for it
            int piggyBackedSizeWithHeader = message->getSize() - statsMessageLength;

            auto buffer = std::unique_ptr<char[]>(new char[piggyBackedSizeWithHeader]);
            memcpy(buffer.get(), message->getRawMessage() + statsMessageLength, piggyBackedSizeWithHeader);

            auto newPacket = NLPacket::fromReceivedPacket(std::move(buffer), piggyBackedSizeWithHeader, message->getSenderSockAddr());
            message = QSharedPointer<ReceivedMessage>::create(*newPacket);
        } else {
            return; // bail since no piggyback data
        }

        packetType = message->getType();
    } // fall through to piggyback message

    if (packetType == PacketType::EntityData) {
        _entityViewer.processDatagram(*message, senderNode);
    } else if (packetType == PacketType::EntityErase) {
        _entityViewer.processEraseMessage(*message, senderNode);
    }
}

void EntityScriptServer::handleJurisdictionPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    NodeType_t nodeType;
    message->peekPrimitive(&nodeType);

    // PacketType_JURISDICTION, first byte is the node type...
    if (nodeType == NodeType::EntityServer) {
        DependencyManager::get<EntityScriptingInterface>()->getJurisdictionListener()->
        queueReceivedPacket(message, senderNode);
    }
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
