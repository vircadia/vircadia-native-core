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

    _scriptEngine = std::unique_ptr<ScriptEngine>(new ScriptEngine(scriptContents, _payload));
    _scriptEngine->setParent(this); // be the parent of the script engine so it gets moved when we do

    // setup an Avatar for the script to use
    ScriptableAvatar scriptedAvatar(_scriptEngine.get());
    scriptedAvatar.setForceFaceTrackerConnected(true);

    // call model URL setters with empty URLs so our avatar, if user, will have the default models
    scriptedAvatar.setFaceModelURL(QUrl());
    scriptedAvatar.setSkeletonModelURL(QUrl());

    // give this AvatarData object to the script engine
    setAvatarData(&scriptedAvatar, "Avatar");
    
    auto avatarHashMap = DependencyManager::set<AvatarHashMap>();
    _scriptEngine->registerGlobalObject("AvatarList", avatarHashMap.data());

    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerListener(PacketType::BulkAvatarData, avatarHashMap.data(), "processAvatarDataPacket");
    packetReceiver.registerListener(PacketType::KillAvatar, avatarHashMap.data(), "processKillAvatar");
    packetReceiver.registerListener(PacketType::AvatarIdentity, avatarHashMap.data(), "processAvatarIdentityPacket");
    packetReceiver.registerListener(PacketType::AvatarBillboard, avatarHashMap.data(), "processAvatarBillboardPacket");

    // register ourselves to the script engine
    _scriptEngine->registerGlobalObject("Agent", this);

    // FIXME -we shouldn't be calling this directly, it's normally called by run(), not sure why 
    // viewers would need this called.
    //_scriptEngine->init(); // must be done before we set up the viewers

    _scriptEngine->registerGlobalObject("SoundCache", DependencyManager::get<SoundCache>().data());

    QScriptValue webSocketServerConstructorValue = _scriptEngine->newFunction(WebSocketServerClass::constructor);
    _scriptEngine->globalObject().setProperty("WebSocketServer", webSocketServerConstructorValue);

    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();

    _scriptEngine->registerGlobalObject("EntityViewer", &_entityViewer);
    
    // we need to make sure that init has been called for our EntityScriptingInterface
    // so that it actually has a jurisdiction listener when we ask it for it next
    entityScriptingInterface->init();
    _entityViewer.setJurisdictionListener(entityScriptingInterface->getJurisdictionListener());
    
    _entityViewer.init();
    
    entityScriptingInterface->setEntityTree(_entityViewer.getTree());

    // wire up our additional agent related processing to the update signal
    QObject::connect(_scriptEngine.get(), &ScriptEngine::update, this, &Agent::processAgentAvatarAndAudio);

    _scriptEngine->run();
    setFinished(true);
}

void Agent::setIsAvatar(bool isAvatar) {
    _isAvatar = isAvatar;

    if (_isAvatar && !_avatarIdentityTimer) {
        // set up the avatar timers
        _avatarIdentityTimer = new QTimer(this);
        _avatarBillboardTimer = new QTimer(this);

        // connect our slot
        connect(_avatarIdentityTimer, &QTimer::timeout, this, &Agent::sendAvatarIdentityPacket);
        connect(_avatarBillboardTimer, &QTimer::timeout, this, &Agent::sendAvatarBillboardPacket);

        // start the timers
        _avatarIdentityTimer->start(AVATAR_IDENTITY_PACKET_SEND_INTERVAL_MSECS);
        _avatarBillboardTimer->start(AVATAR_BILLBOARD_PACKET_SEND_INTERVAL_MSECS);
    }

    if (!_isAvatar) {
        if (_avatarIdentityTimer) {
            _avatarIdentityTimer->stop();
            delete _avatarIdentityTimer;
            _avatarIdentityTimer = nullptr;
        }
        
        if (_avatarBillboardTimer) {
            _avatarBillboardTimer->stop();
            delete _avatarBillboardTimer;
            _avatarBillboardTimer = nullptr;
        }
    }
}

void Agent::setAvatarData(AvatarData* avatarData, const QString& objectName) {
    _avatarData = avatarData;
    _scriptEngine->registerGlobalObject(objectName, avatarData);
}

void Agent::sendAvatarIdentityPacket() {
    if (_isAvatar && _avatarData) {
        _avatarData->sendIdentityPacket();
    }
}

void Agent::sendAvatarBillboardPacket() {
    if (_isAvatar && _avatarData) {
        _avatarData->sendBillboardPacket();
    }
}


void Agent::processAgentAvatarAndAudio(float deltaTime) {
    if (!_scriptEngine->isFinished() && _isAvatar && _avatarData) {

        const int SCRIPT_AUDIO_BUFFER_SAMPLES = floor(((SCRIPT_DATA_CALLBACK_USECS * AudioConstants::SAMPLE_RATE)
            / (1000 * 1000)) + 0.5);
        const int SCRIPT_AUDIO_BUFFER_BYTES = SCRIPT_AUDIO_BUFFER_SAMPLES * sizeof(int16_t);

        QByteArray avatarByteArray = _avatarData->toByteArray(true, randFloat() < AVATAR_SEND_FULL_UPDATE_RATIO);
        _avatarData->doneEncoding(true);
        auto avatarPacket = NLPacket::create(PacketType::AvatarData, avatarByteArray.size());

        avatarPacket->write(avatarByteArray);

        auto nodeList = DependencyManager::get<NodeList>();

        nodeList->broadcastToNodes(std::move(avatarPacket), NodeSet() << NodeType::AvatarMixer);

        if (_isListeningToAudioStream || _avatarSound) {
            // if we have an avatar audio stream then send it out to our audio-mixer
            bool silentFrame = true;

            int16_t numAvailableSamples = SCRIPT_AUDIO_BUFFER_SAMPLES;
            const int16_t* nextSoundOutput = NULL;

            if (_avatarSound) {

                const QByteArray& soundByteArray = _avatarSound->getByteArray();
                nextSoundOutput = reinterpret_cast<const int16_t*>(soundByteArray.data()
                    + _numAvatarSoundSentBytes);

                int numAvailableBytes = (soundByteArray.size() - _numAvatarSoundSentBytes) > SCRIPT_AUDIO_BUFFER_BYTES
                    ? SCRIPT_AUDIO_BUFFER_BYTES
                    : soundByteArray.size() - _numAvatarSoundSentBytes;
                numAvailableSamples = numAvailableBytes / sizeof(int16_t);


                // check if the all of the _numAvatarAudioBufferSamples to be sent are silence
                for (int i = 0; i < numAvailableSamples; ++i) {
                    if (nextSoundOutput[i] != 0) {
                        silentFrame = false;
                        break;
                    }
                }

                _numAvatarSoundSentBytes += numAvailableBytes;
                if (_numAvatarSoundSentBytes == soundByteArray.size()) {
                    // we're done with this sound object - so set our pointer back to NULL
                    // and our sent bytes back to zero
                    _avatarSound = NULL;
                    _numAvatarSoundSentBytes = 0;
                }
            }

            auto audioPacket = NLPacket::create(silentFrame
                ? PacketType::SilentAudioFrame
                : PacketType::MicrophoneAudioNoEcho);

            // seek past the sequence number, will be packed when destination node is known
            audioPacket->seek(sizeof(quint16));

            if (silentFrame) {
                if (!_isListeningToAudioStream) {
                    // if we have a silent frame and we're not listening then just send nothing and break out of here
                    return;
                }

                // write the number of silent samples so the audio-mixer can uphold timing
                audioPacket->writePrimitive(SCRIPT_AUDIO_BUFFER_SAMPLES);

                // use the orientation and position of this avatar for the source of this audio
                audioPacket->writePrimitive(_avatarData->getPosition());
                glm::quat headOrientation = _avatarData->getHeadOrientation();
                audioPacket->writePrimitive(headOrientation);

            }else if (nextSoundOutput) {
                // assume scripted avatar audio is mono and set channel flag to zero
                audioPacket->writePrimitive((quint8)0);

                // use the orientation and position of this avatar for the source of this audio
                audioPacket->writePrimitive(_avatarData->getPosition());
                glm::quat headOrientation = _avatarData->getHeadOrientation();
                audioPacket->writePrimitive(headOrientation);

                // write the raw audio data
                audioPacket->write(reinterpret_cast<const char*>(nextSoundOutput), numAvailableSamples * sizeof(int16_t));
            }

            // write audio packet to AudioMixer nodes
            auto nodeList = DependencyManager::get<NodeList>();
            nodeList->eachNode([this, &nodeList, &audioPacket](const SharedNodePointer& node){
                // only send to nodes of type AudioMixer
                if (node->getType() == NodeType::AudioMixer) {
                    // pack sequence number
                    quint16 sequence = _outgoingScriptAudioSequenceNumbers[node->getUUID()]++;
                    audioPacket->seek(0);
                    audioPacket->writePrimitive(sequence);

                    // send audio packet
                    nodeList->sendUnreliablePacket(*audioPacket, *node);
                }
            });
        }
    }
}

void Agent::aboutToFinish() {
    setIsAvatar(false);// will stop timers for sending billboards and identity packets
    if (_scriptEngine) {
        _scriptEngine->stop();
    }

    if (_pingTimer) {
        _pingTimer->stop();
        delete _pingTimer;
    }

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
