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

#include <AssetClient.h>
#include <AvatarHashMap.h>
#include <AudioInjectorManager.h>
#include <AssetClient.h>
#include <MessagesClient.h>
#include <NetworkAccessManager.h>
#include <NodeList.h>
#include <udt/PacketHeaders.h>
#include <ResourceCache.h>
#include <ScriptCache.h>
#include <SoundCache.h>
#include <UUID.h>

#include <recording/Deck.h>
#include <recording/Recorder.h>
#include <recording/Frame.h>

#include <plugins/CodecPlugin.h>
#include <plugins/PluginManager.h>

#include <WebSocketServerClass.h>
#include <EntityScriptingInterface.h> // TODO: consider moving to scriptengine.h

#include "avatars/ScriptableAvatar.h"
#include "entities/AssignmentParentFinder.h"
#include "RecordingScriptingInterface.h"
#include "AbstractAudioInterface.h"

#include "Agent.h"

static const int RECEIVED_AUDIO_STREAM_CAPACITY_FRAMES = 10;

Agent::Agent(ReceivedMessage& message) :
    ThreadedAssignment(message),
    _entityEditSender(),
    _receivedAudioStream(RECEIVED_AUDIO_STREAM_CAPACITY_FRAMES, RECEIVED_AUDIO_STREAM_CAPACITY_FRAMES) {
    DependencyManager::get<EntityScriptingInterface>()->setPacketSender(&_entityEditSender);

    ResourceManager::init();

    DependencyManager::registerInheritance<SpatialParentFinder, AssignmentParentFinder>();

    DependencyManager::set<ResourceCacheSharedItems>();
    DependencyManager::set<SoundCache>();
    DependencyManager::set<AudioInjectorManager>();
    DependencyManager::set<recording::Deck>();
    DependencyManager::set<recording::Recorder>();
    DependencyManager::set<RecordingScriptingInterface>();

    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();

    packetReceiver.registerListenerForTypes(
        { PacketType::MixedAudio, PacketType::SilentAudioFrame },
        this, "handleAudioPacket");
    packetReceiver.registerListenerForTypes(
        { PacketType::OctreeStats, PacketType::EntityData, PacketType::EntityErase },
        this, "handleOctreePacket");
    packetReceiver.registerListener(PacketType::Jurisdiction, this, "handleJurisdictionPacket");
    packetReceiver.registerListener(PacketType::SelectedAudioFormat, this, "handleSelectedAudioFormat");
}

void Agent::handleOctreePacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
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

void Agent::handleJurisdictionPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    NodeType_t nodeType;
    message->peekPrimitive(&nodeType);

    // PacketType_JURISDICTION, first byte is the node type...
    if (nodeType == NodeType::EntityServer) {
        DependencyManager::get<EntityScriptingInterface>()->getJurisdictionListener()->
            queueReceivedPacket(message, senderNode);
    }
}

void Agent::handleAudioPacket(QSharedPointer<ReceivedMessage> message) {
    _receivedAudioStream.parseData(*message);

    _lastReceivedAudioLoudness = _receivedAudioStream.getNextOutputFrameLoudness();

    _receivedAudioStream.clearBuffer();
}

const QString AGENT_LOGGING_NAME = "agent";

void Agent::run() {

    // make sure we request our script once the agent connects to the domain
    auto nodeList = DependencyManager::get<NodeList>();

    connect(&nodeList->getDomainHandler(), &DomainHandler::connectedToDomain, this, &Agent::requestScript);

    ThreadedAssignment::commonInit(AGENT_LOGGING_NAME, NodeType::Agent);

    // Setup MessagesClient
    auto messagesClient = DependencyManager::set<MessagesClient>();
    QThread* messagesThread = new QThread;
    messagesThread->setObjectName("Messages Client Thread");
    messagesClient->moveToThread(messagesThread);
    connect(messagesThread, &QThread::started, messagesClient.data(), &MessagesClient::init);
    messagesThread->start();

    // make sure we hear about connected nodes so we can grab an ATP script if a request is pending
    connect(nodeList.data(), &LimitedNodeList::nodeActivated, this, &Agent::nodeActivated);

    nodeList->addSetOfNodeTypesToNodeInterestSet({
        NodeType::AudioMixer, NodeType::AvatarMixer, NodeType::EntityServer, NodeType::MessagesMixer, NodeType::AssetServer
    });
}

void Agent::requestScript() {
    auto nodeList = DependencyManager::get<NodeList>();
    disconnect(&nodeList->getDomainHandler(), &DomainHandler::connectedToDomain, this, &Agent::requestScript);

    // figure out the URL for the script for this agent assignment
    QUrl scriptURL;
    if (_payload.isEmpty())  {
        scriptURL = QUrl(QString("http://%1:%2/assignment/%3/")
                         .arg(nodeList->getDomainHandler().getIP().toString())
                         .arg(DOMAIN_SERVER_HTTP_PORT)
                         .arg(uuidStringWithoutCurlyBraces(nodeList->getSessionUUID())));
    } else {
        scriptURL = QUrl(_payload);
    }

    // make sure this is not a script request for the file scheme
    if (scriptURL.scheme() == URL_SCHEME_FILE) {
        qWarning() << "Cannot load script for Agent from local filesystem.";
        scriptRequestFinished();
        return;
    }

    auto request = ResourceManager::createResourceRequest(this, scriptURL);

    if (!request) {
        qWarning() << "Could not create ResourceRequest for Agent script at" << scriptURL.toString();
        scriptRequestFinished();
        return;
    }

    // setup a timeout for script request
    static const int SCRIPT_TIMEOUT_MS = 10000;
    _scriptRequestTimeout = new QTimer;
    connect(_scriptRequestTimeout, &QTimer::timeout, this, &Agent::scriptRequestFinished);
    _scriptRequestTimeout->start(SCRIPT_TIMEOUT_MS);

    connect(request, &ResourceRequest::finished, this, &Agent::scriptRequestFinished);

    if (scriptURL.scheme() == URL_SCHEME_ATP) {
        // we have an ATP URL for the script - if we're not currently connected to the AssetServer
        // then wait for the nodeConnected signal to fire off the request

        auto assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);

        if (!assetServer || !assetServer->getActiveSocket()) {
            qDebug() << "Waiting to connect to Asset Server for ATP script download.";
            _pendingScriptRequest = request;

            return;
        }
    }

    qInfo() << "Requesting script at URL" << qPrintable(request->getUrl().toString());

    request->send();
}

void Agent::nodeActivated(SharedNodePointer activatedNode) {
    if (_pendingScriptRequest && activatedNode->getType() == NodeType::AssetServer) {
        qInfo() << "Requesting script at URL" << qPrintable(_pendingScriptRequest->getUrl().toString());

        _pendingScriptRequest->send();

        _pendingScriptRequest = nullptr;
    }
    if (activatedNode->getType() == NodeType::AudioMixer) {
        negotiateAudioFormat();
    }

}

void Agent::negotiateAudioFormat() {
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
    qInfo() << "negotiateAudioFormat called";
}

void Agent::handleSelectedAudioFormat(QSharedPointer<ReceivedMessage> message) {
    QString selectedCodecName = message->readString();
    selectAudioFormat(selectedCodecName);
}

void Agent::selectAudioFormat(const QString& selectedCodecName) {
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

void Agent::scriptRequestFinished() {
    auto request = qobject_cast<ResourceRequest*>(sender());

    // stop the script request timeout, if it's running
    if (_scriptRequestTimeout) {
        QMetaObject::invokeMethod(_scriptRequestTimeout, "stop");
        _scriptRequestTimeout->deleteLater();
    }

    if (request && request->getResult() == ResourceRequest::Success) {
        _scriptContents = request->getData();
        qInfo() << "Downloaded script:" << _scriptContents;

        // we could just call executeScript directly - we use a QueuedConnection to allow scriptRequestFinished
        // to return before calling executeScript
        QMetaObject::invokeMethod(this, "executeScript", Qt::QueuedConnection);
    } else {
        if (request) {
            qWarning() << "Failed to download script at" << request->getUrl().toString() << " - bailing on assignment.";
            qWarning() << "ResourceRequest error was" << request->getResult();
        } else {
            qWarning() << "Failed to download script - request timed out. Bailing on assignment.";
        }

        setFinished(true);
    }

    request->deleteLater();
}

void Agent::executeScript() {
    _scriptEngine = std::unique_ptr<ScriptEngine>(new ScriptEngine(_scriptContents, _payload));
    _scriptEngine->setParent(this); // be the parent of the script engine so it gets moved when we do

    // setup an Avatar for the script to use
    auto scriptedAvatar = DependencyManager::get<ScriptableAvatar>();
    connect(_scriptEngine.get(), SIGNAL(update(float)), scriptedAvatar.data(), SLOT(update(float)), Qt::ConnectionType::QueuedConnection);
    scriptedAvatar->setForceFaceTrackerConnected(true);

    // call model URL setters with empty URLs so our avatar, if user, will have the default models
    scriptedAvatar->setSkeletonModelURL(QUrl());

    // give this AvatarData object to the script engine
    _scriptEngine->registerGlobalObject("Avatar", scriptedAvatar.data());


    using namespace recording;
    static const FrameType AVATAR_FRAME_TYPE = Frame::registerFrameType(AvatarData::FRAME_NAME);
    // FIXME how to deal with driving multiple avatars locally?
    Frame::registerFrameHandler(AVATAR_FRAME_TYPE, [this, scriptedAvatar](Frame::ConstPointer frame) {
        AvatarData::fromFrame(frame->data, *scriptedAvatar);
    });

    using namespace recording;
    static const FrameType AUDIO_FRAME_TYPE = Frame::registerFrameType(AudioConstants::getAudioFrameName());
    Frame::registerFrameHandler(AUDIO_FRAME_TYPE, [this, &scriptedAvatar](Frame::ConstPointer frame) {
        const QByteArray& audio = frame->data;
        static quint16 audioSequenceNumber{ 0 };
        Transform audioTransform;
        audioTransform.setTranslation(scriptedAvatar->getPosition());
        audioTransform.setRotation(scriptedAvatar->getOrientation());
        AbstractAudioInterface::emitAudioPacket(audio.data(), audio.size(), audioSequenceNumber, audioTransform, PacketType::MicrophoneAudioNoEcho);
    });

    auto avatarHashMap = DependencyManager::set<AvatarHashMap>();
    _scriptEngine->registerGlobalObject("AvatarList", avatarHashMap.data());

    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerListener(PacketType::BulkAvatarData, avatarHashMap.data(), "processAvatarDataPacket");
    packetReceiver.registerListener(PacketType::KillAvatar, avatarHashMap.data(), "processKillAvatar");
    packetReceiver.registerListener(PacketType::AvatarIdentity, avatarHashMap.data(), "processAvatarIdentityPacket");

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

    DependencyManager::set<AssignmentParentFinder>(_entityViewer.getTree());

    _avatarAudioTimer = new QTimer(this);
    _avatarAudioTimer->setTimerType(Qt::PreciseTimer);
    connect(_avatarAudioTimer, SIGNAL(timeout()), this, SLOT(processAgentAvatarAndAudio()));
    _avatarAudioTimer->start(10);

    // wire up our additional agent related processing to the update signal
    //QObject::connect(_scriptEngine.get(), &ScriptEngine::update, this, &Agent::processAgentAvatarAndAudio);

    _scriptEngine->run();

    Frame::clearFrameHandler(AUDIO_FRAME_TYPE);
    Frame::clearFrameHandler(AVATAR_FRAME_TYPE);

    setFinished(true);
}

QUuid Agent::getSessionUUID() const {
    return DependencyManager::get<NodeList>()->getSessionUUID();
}


void Agent::setIsAvatar(bool isAvatar) {
    _isAvatar = isAvatar;

    if (_isAvatar && !_avatarIdentityTimer) {
        // set up the avatar timers
        _avatarIdentityTimer = new QTimer(this);

        // connect our slot
        connect(_avatarIdentityTimer, &QTimer::timeout, this, &Agent::sendAvatarIdentityPacket);

        // start the timers
        _avatarIdentityTimer->start(AVATAR_IDENTITY_PACKET_SEND_INTERVAL_MSECS);
    }

    if (!_isAvatar) {

        if (_avatarIdentityTimer) {
            _avatarIdentityTimer->stop();
            delete _avatarIdentityTimer;
            _avatarIdentityTimer = nullptr;
        }
    }
}

void Agent::sendAvatarIdentityPacket() {
    if (_isAvatar) {
        auto scriptedAvatar = DependencyManager::get<ScriptableAvatar>();
        scriptedAvatar->sendIdentityPacket();
    }
}

void Agent::processAgentAvatarAndAudio() {
    if (!_scriptEngine->isFinished() && _isAvatar) {
        auto scriptedAvatar = DependencyManager::get<ScriptableAvatar>();
        const int SCRIPT_AUDIO_BUFFER_SAMPLES = AudioConstants::SAMPLE_RATE / 100;//SCRIPT_FPS + 0.5;
        const int SCRIPT_AUDIO_BUFFER_BYTES = SCRIPT_AUDIO_BUFFER_SAMPLES * sizeof(int16_t);

        QByteArray avatarByteArray = scriptedAvatar->toByteArray(true, randFloat() < AVATAR_SEND_FULL_UPDATE_RATIO);
        scriptedAvatar->doneEncoding(true);

        static AvatarDataSequenceNumber sequenceNumber = 0;
        auto avatarPacket = NLPacket::create(PacketType::AvatarData, avatarByteArray.size() + sizeof(sequenceNumber));
        avatarPacket->writePrimitive(sequenceNumber++);

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
                numAvailableSamples = (int16_t)numAvailableBytes / sizeof(int16_t);


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
                    _avatarSound.clear();
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
                audioPacket->writePrimitive(scriptedAvatar->getPosition());
                glm::quat headOrientation = scriptedAvatar->getHeadOrientation();
                audioPacket->writePrimitive(headOrientation);

            } else if (nextSoundOutput) {
                // write the codec
                audioPacket->writeString(_selectedCodecName);
                
                // assume scripted avatar audio is mono and set channel flag to zero
                audioPacket->writePrimitive((quint8)0);

                // use the orientation and position of this avatar for the source of this audio
                audioPacket->writePrimitive(scriptedAvatar->getPosition());
                glm::quat headOrientation = scriptedAvatar->getHeadOrientation();
                audioPacket->writePrimitive(headOrientation);
               
                // encode it
                QByteArray decodedBuffer(reinterpret_cast<const char*>(nextSoundOutput), numAvailableSamples*sizeof(int16_t));
                QByteArray encodedBuffer;
                if (_encoder) {
                    _encoder->encode(decodedBuffer, encodedBuffer);
                } else {
                    audioPacket->write(decodedBuffer.data(), decodedBuffer.size());
                }

                // write the raw audio data
                audioPacket->write(encodedBuffer.data(), encodedBuffer.size());
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
    setIsAvatar(false);// will stop timers for sending identity packets

    if (_scriptEngine) {
        _scriptEngine->stop();
    }

    // our entity tree is going to go away so tell that to the EntityScriptingInterface
    DependencyManager::get<EntityScriptingInterface>()->setEntityTree(nullptr);

    ResourceManager::cleanup();
    
    // cleanup the AudioInjectorManager (and any still running injectors)
    DependencyManager::destroy<AudioInjectorManager>();

    // cleanup codec & encoder
    if (_codec && _encoder) {
        _codec->releaseEncoder(_encoder);
        _encoder = nullptr;
    }
}
