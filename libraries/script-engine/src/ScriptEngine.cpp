//
//  ScriptEngine.cpp
//  libraries/script-engine/src
//
//  Created by Brad Hefta-Gaub on 12/14/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QScriptEngine>

#include <AudioInjector.h>
#include <AudioRingBuffer.h>
#include <AvatarData.h>
#include <Bitstream.h>
#include <CollisionInfo.h>
#include <EntityScriptingInterface.h>
#include <NetworkAccessManager.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <ParticlesScriptingInterface.h>
#include <Sound.h>
#include <UUID.h>
#include <VoxelConstants.h>
#include <VoxelDetail.h>

#include "AnimationObject.h"
#include "ArrayBufferViewClass.h"
#include "DataViewClass.h"
#include "MenuItemProperties.h"
#include "MIDIEvent.h"
#include "LocalVoxels.h"
#include "ScriptEngine.h"
#include "TypedArrays.h"
#include "XMLHttpRequestClass.h"

VoxelsScriptingInterface ScriptEngine::_voxelsScriptingInterface;
ParticlesScriptingInterface ScriptEngine::_particlesScriptingInterface;
EntityScriptingInterface ScriptEngine::_entityScriptingInterface;

static QScriptValue soundConstructor(QScriptContext* context, QScriptEngine* engine) {
    QUrl soundURL = QUrl(context->argument(0).toString());
    QScriptValue soundScriptValue = engine->newQObject(new Sound(soundURL), QScriptEngine::ScriptOwnership);

    return soundScriptValue;
}

static QScriptValue debugPrint(QScriptContext* context, QScriptEngine* engine){
    qDebug() << "script:print()<<" << context->argument(0).toString();
    QString message = context->argument(0).toString()
        .replace("\\", "\\\\")
        .replace("\n", "\\n")
        .replace("\r", "\\r")
        .replace("'", "\\'");
    engine->evaluate("Script.print('" + message + "')");
    return QScriptValue();
}

QScriptValue injectorToScriptValue(QScriptEngine *engine, AudioInjector* const &in) {
    return engine->newQObject(in);
}

void injectorFromScriptValue(const QScriptValue &object, AudioInjector* &out) {
    out = qobject_cast<AudioInjector*>(object.toQObject());
}

QScriptValue injectorToScriptValueInputController(QScriptEngine *engine, AbstractInputController* const &in) {
    return engine->newQObject(in);
}

void injectorFromScriptValueInputController(const QScriptValue &object, AbstractInputController* &out) {
    out = qobject_cast<AbstractInputController*>(object.toQObject());
}

ScriptEngine::ScriptEngine(const QString& scriptContents, const QString& fileNameString,
                           AbstractControllerScriptingInterface* controllerScriptingInterface) :

    _scriptContents(scriptContents),
    _isFinished(false),
    _isRunning(false),
    _isInitialized(false),
    _isAvatar(false),
    _avatarIdentityTimer(NULL),
    _avatarBillboardTimer(NULL),
    _timerFunctionMap(),
    _isListeningToAudioStream(false),
    _avatarSound(NULL),
    _numAvatarSoundSentBytes(0),
    _controllerScriptingInterface(controllerScriptingInterface),
    _avatarData(NULL),
    _scriptName(),
    _fileNameString(fileNameString),
    _quatLibrary(),
    _vec3Library(),
    _uuidLibrary(),
    _animationCache(this),
    _arrayBufferClass(new ArrayBufferClass(this))
{
}

ScriptEngine::ScriptEngine(const QUrl& scriptURL,
                           AbstractControllerScriptingInterface* controllerScriptingInterface)  :
    _scriptContents(),
    _isFinished(false),
    _isRunning(false),
    _isInitialized(false),
    _isAvatar(false),
    _avatarIdentityTimer(NULL),
    _avatarBillboardTimer(NULL),
    _timerFunctionMap(),
    _isListeningToAudioStream(false),
    _avatarSound(NULL),
    _numAvatarSoundSentBytes(0),
    _controllerScriptingInterface(controllerScriptingInterface),
    _avatarData(NULL),
    _scriptName(),
    _fileNameString(),
    _quatLibrary(),
    _vec3Library(),
    _uuidLibrary(),
    _animationCache(this),
    _arrayBufferClass(new ArrayBufferClass(this))
{
    QString scriptURLString = scriptURL.toString();
    _fileNameString = scriptURLString;

    QUrl url(scriptURL);
    
    // if the scheme length is one or lower, maybe they typed in a file, let's try
    const int WINDOWS_DRIVE_LETTER_SIZE = 1;
    if (url.scheme().size() <= WINDOWS_DRIVE_LETTER_SIZE) {
        url = QUrl::fromLocalFile(scriptURLString);
    }

    // ok, let's see if it's valid... and if so, load it
    if (url.isValid()) {
        if (url.scheme() == "file") {
            QString fileName = url.toLocalFile();
            QFile scriptFile(fileName);
            if (scriptFile.open(QFile::ReadOnly | QFile::Text)) {
                qDebug() << "Loading file:" << fileName;
                QTextStream in(&scriptFile);
                _scriptContents = in.readAll();
            } else {
                qDebug() << "ERROR Loading file:" << fileName;
                emit errorMessage("ERROR Loading file:" + fileName);
            }
        } else {
            NetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
            QNetworkReply* reply = networkAccessManager.get(QNetworkRequest(url));
            qDebug() << "Downloading script at" << url;
            QEventLoop loop;
            QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
            loop.exec();
            if (reply->error() == QNetworkReply::NoError && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200) {
                _scriptContents = reply->readAll();
            } else {
                qDebug() << "ERROR Loading file:" << url.toString();
                emit errorMessage("ERROR Loading file:" + url.toString());
            }
        }
    }
}

void ScriptEngine::setIsAvatar(bool isAvatar) {
    _isAvatar = isAvatar;

    if (_isAvatar && !_avatarIdentityTimer) {
        // set up the avatar timers
        _avatarIdentityTimer = new QTimer(this);
        _avatarBillboardTimer = new QTimer(this);

        // connect our slot
        connect(_avatarIdentityTimer, &QTimer::timeout, this, &ScriptEngine::sendAvatarIdentityPacket);
        connect(_avatarBillboardTimer, &QTimer::timeout, this, &ScriptEngine::sendAvatarBillboardPacket);

        // start the timers
        _avatarIdentityTimer->start(AVATAR_IDENTITY_PACKET_SEND_INTERVAL_MSECS);
        _avatarBillboardTimer->start(AVATAR_BILLBOARD_PACKET_SEND_INTERVAL_MSECS);
    }
}

void ScriptEngine::setAvatarData(AvatarData* avatarData, const QString& objectName) {
    _avatarData = avatarData;

    // remove the old Avatar property, if it exists
    globalObject().setProperty(objectName, QScriptValue());

    // give the script engine the new Avatar script property
    registerGlobalObject(objectName, _avatarData);
}

void ScriptEngine::setAvatarHashMap(AvatarHashMap* avatarHashMap, const QString& objectName) {
    // remove the old Avatar property, if it exists
    globalObject().setProperty(objectName, QScriptValue());

    // give the script engine the new avatar hash map
    registerGlobalObject(objectName, avatarHashMap);
}

bool ScriptEngine::setScriptContents(const QString& scriptContents, const QString& fileNameString) {
    if (_isRunning) {
        return false;
    }
    _scriptContents = scriptContents;
    _fileNameString = fileNameString;
    return true;
}

Q_SCRIPT_DECLARE_QMETAOBJECT(AudioInjectorOptions, QObject*)
Q_SCRIPT_DECLARE_QMETAOBJECT(LocalVoxels, QString)

void ScriptEngine::init() {
    if (_isInitialized) {
        return; // only initialize once
    }
    
    _isInitialized = true;

    _voxelsScriptingInterface.init();
    _particlesScriptingInterface.init();

    // register various meta-types
    registerMetaTypes(this);
    registerMIDIMetaTypes(this);
    registerVoxelMetaTypes(this);
    registerEventTypes(this);
    registerMenuItemProperties(this);
    registerAnimationTypes(this);
    registerAvatarTypes(this);
    Bitstream::registerTypes(this);

    qScriptRegisterMetaType(this, ParticlePropertiesToScriptValue, ParticlePropertiesFromScriptValue);
    qScriptRegisterMetaType(this, ParticleIDtoScriptValue, ParticleIDfromScriptValue);
    qScriptRegisterSequenceMetaType<QVector<ParticleID> >(this);

    /**
    qScriptRegisterMetaType(this, ModelItemPropertiesToScriptValue, ModelItemPropertiesFromScriptValue);
    qScriptRegisterMetaType(this, ModelItemIDtoScriptValue, ModelItemIDfromScriptValue);
    qScriptRegisterMetaType(this, RayToModelIntersectionResultToScriptValue, RayToModelIntersectionResultFromScriptValue);
    qScriptRegisterSequenceMetaType<QVector<ModelItemID> >(this);
    **/

    qScriptRegisterMetaType(this, EntityItemPropertiesToScriptValue, EntityItemPropertiesFromScriptValue);
    qScriptRegisterMetaType(this, EntityItemIDtoScriptValue, EntityItemIDfromScriptValue);
    qScriptRegisterMetaType(this, RayToEntityIntersectionResultToScriptValue, RayToEntityIntersectionResultFromScriptValue);
    qScriptRegisterSequenceMetaType<QVector<EntityItemID> >(this);

    qScriptRegisterSequenceMetaType<QVector<glm::vec2> >(this);
    qScriptRegisterSequenceMetaType<QVector<glm::quat> >(this);
    qScriptRegisterSequenceMetaType<QVector<QString> >(this);

    QScriptValue xmlHttpRequestConstructorValue = newFunction(XMLHttpRequestClass::constructor);
    globalObject().setProperty("XMLHttpRequest", xmlHttpRequestConstructorValue);

    QScriptValue printConstructorValue = newFunction(debugPrint);
    globalObject().setProperty("print", printConstructorValue);

    QScriptValue soundConstructorValue = newFunction(soundConstructor);
    QScriptValue soundMetaObject = newQMetaObject(&Sound::staticMetaObject, soundConstructorValue);
    globalObject().setProperty("Sound", soundMetaObject);

    QScriptValue injectionOptionValue = scriptValueFromQMetaObject<AudioInjectorOptions>();
    globalObject().setProperty("AudioInjectionOptions", injectionOptionValue);

    QScriptValue localVoxelsValue = scriptValueFromQMetaObject<LocalVoxels>();
    globalObject().setProperty("LocalVoxels", localVoxelsValue);
    
    qScriptRegisterMetaType(this, injectorToScriptValue, injectorFromScriptValue);
    qScriptRegisterMetaType( this, injectorToScriptValueInputController, injectorFromScriptValueInputController);

    qScriptRegisterMetaType(this, animationDetailsToScriptValue, animationDetailsFromScriptValue);

    registerGlobalObject("Script", this);
    registerGlobalObject("Audio", &_audioScriptingInterface);
    registerGlobalObject("Controller", _controllerScriptingInterface);
    registerGlobalObject("Entities", &_entityScriptingInterface);
    registerGlobalObject("Particles", &_particlesScriptingInterface);
    registerGlobalObject("Quat", &_quatLibrary);
    registerGlobalObject("Vec3", &_vec3Library);
    registerGlobalObject("Uuid", &_uuidLibrary);
    registerGlobalObject("AnimationCache", &_animationCache);

    registerGlobalObject("Voxels", &_voxelsScriptingInterface);

    // constants
    globalObject().setProperty("TREE_SCALE", newVariant(QVariant(TREE_SCALE)));
    globalObject().setProperty("COLLISION_GROUP_ENVIRONMENT", newVariant(QVariant(COLLISION_GROUP_ENVIRONMENT)));
    globalObject().setProperty("COLLISION_GROUP_AVATARS", newVariant(QVariant(COLLISION_GROUP_AVATARS)));
    globalObject().setProperty("COLLISION_GROUP_VOXELS", newVariant(QVariant(COLLISION_GROUP_VOXELS)));
    globalObject().setProperty("COLLISION_GROUP_PARTICLES", newVariant(QVariant(COLLISION_GROUP_PARTICLES)));

    globalObject().setProperty("AVATAR_MOTION_OBEY_LOCAL_GRAVITY", newVariant(QVariant(AVATAR_MOTION_OBEY_LOCAL_GRAVITY)));
    globalObject().setProperty("AVATAR_MOTION_OBEY_ENVIRONMENTAL_GRAVITY", newVariant(QVariant(AVATAR_MOTION_OBEY_ENVIRONMENTAL_GRAVITY)));

    // let the VoxelPacketSender know how frequently we plan to call it
    _voxelsScriptingInterface.getVoxelPacketSender()->setProcessCallIntervalHint(SCRIPT_DATA_CALLBACK_USECS);
    _particlesScriptingInterface.getParticlePacketSender()->setProcessCallIntervalHint(SCRIPT_DATA_CALLBACK_USECS);
}

QScriptValue ScriptEngine::registerGlobalObject(const QString& name, QObject* object) {
    if (object) {
        QScriptValue value = newQObject(object);
        globalObject().setProperty(name, value);
        return value;
    }
    return QScriptValue::NullValue;
}

void ScriptEngine::registerGetterSetter(const QString& name, QScriptEngine::FunctionSignature getter,
                                        QScriptEngine::FunctionSignature setter, QScriptValue object) {
    QScriptValue setterFunction = newFunction(setter, 1);
    QScriptValue getterFunction = newFunction(getter);

    if (!object.isNull()) {
        object.setProperty(name, setterFunction, QScriptValue::PropertySetter);
        object.setProperty(name, getterFunction, QScriptValue::PropertyGetter);
    } else {
        globalObject().setProperty(name, setterFunction, QScriptValue::PropertySetter);
        globalObject().setProperty(name, getterFunction, QScriptValue::PropertyGetter);
    }
}

void ScriptEngine::evaluate() {
    if (!_isInitialized) {
        init();
    }

    QScriptValue result = evaluate(_scriptContents);

    if (hasUncaughtException()) {
        int line = uncaughtExceptionLineNumber();
        qDebug() << "Uncaught exception at (" << _fileNameString << ") line" << line << ":" << result.toString();
        emit errorMessage("Uncaught exception at (" + _fileNameString + ") line" + QString::number(line) + ":" + result.toString());
        clearExceptions();
    }
}

QScriptValue ScriptEngine::evaluate(const QString& program, const QString& fileName, int lineNumber) {
    QScriptValue result = QScriptEngine::evaluate(program, fileName, lineNumber);
    if (hasUncaughtException()) {
        int line = uncaughtExceptionLineNumber();
        qDebug() << "Uncaught exception at (" << _fileNameString << ") line" << line << ": " << result.toString();
    }
    emit evaluationFinished(result, hasUncaughtException());
    clearExceptions();
    return result;
}

void ScriptEngine::sendAvatarIdentityPacket() {
    if (_isAvatar && _avatarData) {
        _avatarData->sendIdentityPacket();
    }
}

void ScriptEngine::sendAvatarBillboardPacket() {
    if (_isAvatar && _avatarData) {
        _avatarData->sendBillboardPacket();
    }
}

void ScriptEngine::run() {
    if (!_isInitialized) {
        init();
    }
    _isRunning = true;
    _isFinished = false;
    emit runningStateChanged();

    QScriptValue result = evaluate(_scriptContents);
    if (hasUncaughtException()) {
        int line = uncaughtExceptionLineNumber();
        qDebug() << "Uncaught exception at (" << _fileNameString << ") line" << line << ":" << result.toString();
        emit errorMessage("Uncaught exception at (" + _fileNameString + ") line" + QString::number(line) + ":" + result.toString());
        clearExceptions();
    }

    QElapsedTimer startTime;
    startTime.start();

    int thisFrame = 0;

    NodeList* nodeList = NodeList::getInstance();

    qint64 lastUpdate = usecTimestampNow();

    while (!_isFinished) {
        int usecToSleep = (thisFrame++ * SCRIPT_DATA_CALLBACK_USECS) - startTime.nsecsElapsed() / 1000; // nsec to usec
        if (usecToSleep > 0) {
            usleep(usecToSleep);
        }

        if (_isFinished) {
            break;
        }

        QCoreApplication::processEvents();

        if (_isFinished) {
            break;
        }

        if (_voxelsScriptingInterface.getVoxelPacketSender()->serversExist()) {
            // release the queue of edit voxel messages.
            _voxelsScriptingInterface.getVoxelPacketSender()->releaseQueuedMessages();

            // since we're in non-threaded mode, call process so that the packets are sent
            if (!_voxelsScriptingInterface.getVoxelPacketSender()->isThreaded()) {
                _voxelsScriptingInterface.getVoxelPacketSender()->process();
            }
        }

        if (_particlesScriptingInterface.getParticlePacketSender()->serversExist()) {
            // release the queue of edit voxel messages.
            _particlesScriptingInterface.getParticlePacketSender()->releaseQueuedMessages();

            // since we're in non-threaded mode, call process so that the packets are sent
            if (!_particlesScriptingInterface.getParticlePacketSender()->isThreaded()) {
                _particlesScriptingInterface.getParticlePacketSender()->process();
            }
        }

        if (_entityScriptingInterface.getEntityPacketSender()->serversExist()) {
            // release the queue of edit voxel messages.
            _entityScriptingInterface.getEntityPacketSender()->releaseQueuedMessages();

            // since we're in non-threaded mode, call process so that the packets are sent
            if (!_entityScriptingInterface.getEntityPacketSender()->isThreaded()) {
                _entityScriptingInterface.getEntityPacketSender()->process();
            }
        } else {
            qDebug() << "Script engine wanted to release packets but no server existed!!!! **************";
        }

        if (_isAvatar && _avatarData) {

            const int SCRIPT_AUDIO_BUFFER_SAMPLES = floor(((SCRIPT_DATA_CALLBACK_USECS * SAMPLE_RATE) / (1000 * 1000)) + 0.5);
            const int SCRIPT_AUDIO_BUFFER_BYTES = SCRIPT_AUDIO_BUFFER_SAMPLES * sizeof(int16_t);

            QByteArray avatarPacket = byteArrayWithPopulatedHeader(PacketTypeAvatarData);
            avatarPacket.append(_avatarData->toByteArray());

            nodeList->broadcastToNodes(avatarPacket, NodeSet() << NodeType::AvatarMixer);

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
                
                QByteArray audioPacket = byteArrayWithPopulatedHeader(silentFrame
                                                                      ? PacketTypeSilentAudioFrame
                                                                      : PacketTypeMicrophoneAudioNoEcho);

                QDataStream packetStream(&audioPacket, QIODevice::Append);

                // pack a placeholder value for sequence number for now, will be packed when destination node is known
                int numPreSequenceNumberBytes = audioPacket.size();
                packetStream << (quint16) 0;
                
                // assume scripted avatar audio is mono and set channel flag to zero
                packetStream << (quint8) 0;

                // use the orientation and position of this avatar for the source of this audio
                packetStream.writeRawData(reinterpret_cast<const char*>(&_avatarData->getPosition()), sizeof(glm::vec3));
                glm::quat headOrientation = _avatarData->getHeadOrientation();
                packetStream.writeRawData(reinterpret_cast<const char*>(&headOrientation), sizeof(glm::quat));

                if (silentFrame) {
                    if (!_isListeningToAudioStream) {
                        // if we have a silent frame and we're not listening then just send nothing and break out of here
                        break;
                    }

                    // write the number of silent samples so the audio-mixer can uphold timing
                    packetStream.writeRawData(reinterpret_cast<const char*>(&SCRIPT_AUDIO_BUFFER_SAMPLES), sizeof(int16_t));
                } else if (nextSoundOutput) {
                    // write the raw audio data
                    packetStream.writeRawData(reinterpret_cast<const char*>(nextSoundOutput),
                                              numAvailableSamples * sizeof(int16_t));
                }

                // write audio packet to AudioMixer nodes
                NodeList* nodeList = NodeList::getInstance();
                foreach(const SharedNodePointer& node, nodeList->getNodeHash()) {
                    // only send to nodes of type AudioMixer
                    if (node->getType() == NodeType::AudioMixer) {
                        // pack sequence number
                        quint16 sequence = _outgoingScriptAudioSequenceNumbers[node->getUUID()]++;
                        memcpy(audioPacket.data() + numPreSequenceNumberBytes, &sequence, sizeof(quint16));

                        // send audio packet
                        nodeList->writeDatagram(audioPacket, node);
                    }
                }
            }
        }

        qint64 now = usecTimestampNow();
        float deltaTime = (float) (now - lastUpdate) / (float) USECS_PER_SECOND;

        if (hasUncaughtException()) {
            int line = uncaughtExceptionLineNumber();
            qDebug() << "Uncaught exception at (" << _fileNameString << ") line" << line << ":" << uncaughtException().toString();
            emit errorMessage("Uncaught exception at (" + _fileNameString + ") line" + QString::number(line) + ":" + uncaughtException().toString());
            clearExceptions();
        }

        emit update(deltaTime);
        lastUpdate = now;
    }
    emit scriptEnding();

    // kill the avatar identity timer
    delete _avatarIdentityTimer;

    if (_voxelsScriptingInterface.getVoxelPacketSender()->serversExist()) {
        // release the queue of edit voxel messages.
        _voxelsScriptingInterface.getVoxelPacketSender()->releaseQueuedMessages();

        // since we're in non-threaded mode, call process so that the packets are sent
        if (!_voxelsScriptingInterface.getVoxelPacketSender()->isThreaded()) {
            _voxelsScriptingInterface.getVoxelPacketSender()->process();
        }
    }

    if (_particlesScriptingInterface.getParticlePacketSender()->serversExist()) {
        // release the queue of edit voxel messages.
        _particlesScriptingInterface.getParticlePacketSender()->releaseQueuedMessages();

        // since we're in non-threaded mode, call process so that the packets are sent
        if (!_particlesScriptingInterface.getParticlePacketSender()->isThreaded()) {
            _particlesScriptingInterface.getParticlePacketSender()->process();
        }
    }

    if (_entityScriptingInterface.getEntityPacketSender()->serversExist()) {
        // release the queue of edit entity messages.
        _entityScriptingInterface.getEntityPacketSender()->releaseQueuedMessages();

        // since we're in non-threaded mode, call process so that the packets are sent
        if (!_entityScriptingInterface.getEntityPacketSender()->isThreaded()) {
            _entityScriptingInterface.getEntityPacketSender()->process();
        }
    }

    // If we were on a thread, then wait till it's done
    if (thread()) {
        thread()->quit();
    }

    emit finished(_fileNameString);

    _isRunning = false;
    emit runningStateChanged();
}

void ScriptEngine::stop() {
    _isFinished = true;
    emit runningStateChanged();
}

void ScriptEngine::timerFired() {
    QTimer* callingTimer = reinterpret_cast<QTimer*>(sender());

    // call the associated JS function, if it exists
    QScriptValue timerFunction = _timerFunctionMap.value(callingTimer);
    if (timerFunction.isValid()) {
        timerFunction.call();
    }

    if (!callingTimer->isActive()) {
        // this timer is done, we can kill it
        delete callingTimer;
    }
}

QObject* ScriptEngine::setupTimerWithInterval(const QScriptValue& function, int intervalMS, bool isSingleShot) {
    // create the timer, add it to the map, and start it
    QTimer* newTimer = new QTimer(this);
    newTimer->setSingleShot(isSingleShot);

    connect(newTimer, &QTimer::timeout, this, &ScriptEngine::timerFired);

    // make sure the timer stops when the script does
    connect(this, &ScriptEngine::scriptEnding, newTimer, &QTimer::stop);

    _timerFunctionMap.insert(newTimer, function);

    newTimer->start(intervalMS);
    return newTimer;
}

QObject* ScriptEngine::setInterval(const QScriptValue& function, int intervalMS) {
    return setupTimerWithInterval(function, intervalMS, false);
}

QObject* ScriptEngine::setTimeout(const QScriptValue& function, int timeoutMS) {
    return setupTimerWithInterval(function, timeoutMS, true);
}

void ScriptEngine::stopTimer(QTimer *timer) {
    if (_timerFunctionMap.contains(timer)) {
        timer->stop();
        _timerFunctionMap.remove(timer);
        delete timer;
    }
}

QUrl ScriptEngine::resolveInclude(const QString& include) const {
    // first lets check to see if it's already a full URL
    QUrl url(include);
    if (!url.scheme().isEmpty()) {
        return url;
    }

    // we apparently weren't a fully qualified url, so, let's assume we're relative
    // to the original URL of our script
    QUrl parentURL(_fileNameString);

    // if the parent URL's scheme is empty, then this is probably a local file...
    if (parentURL.scheme().isEmpty()) {
        parentURL = QUrl::fromLocalFile(_fileNameString);
    }

    // at this point we should have a legitimate fully qualified URL for our parent
    url = parentURL.resolved(url);
    return url;
}

void ScriptEngine::print(const QString& message) {
    emit printedMessage(message);
}

void ScriptEngine::include(const QString& includeFile) {
    QUrl url = resolveInclude(includeFile);
    QString includeContents;

    if (url.scheme() == "http" || url.scheme() == "ftp") {
        NetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
        QNetworkReply* reply = networkAccessManager.get(QNetworkRequest(url));
        qDebug() << "Downloading included script at" << includeFile;
        QEventLoop loop;
        QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();
        includeContents = reply->readAll();
    } else {
#ifdef _WIN32
        QString fileName = url.toString();
#else
        QString fileName = url.toLocalFile();
#endif
        QFile scriptFile(fileName);
        if (scriptFile.open(QFile::ReadOnly | QFile::Text)) {
            qDebug() << "Including file:" << fileName;
            QTextStream in(&scriptFile);
            includeContents = in.readAll();
        } else {
            qDebug() << "ERROR Including file:" << fileName;
            emit errorMessage("ERROR Including file:" + fileName);
        }
    }

    QScriptValue result = evaluate(includeContents);
    if (hasUncaughtException()) {
        int line = uncaughtExceptionLineNumber();
        qDebug() << "Uncaught exception at (" << includeFile << ") line" << line << ":" << result.toString();
        emit errorMessage("Uncaught exception at (" + includeFile + ") line" + QString::number(line) + ":" + result.toString());
        clearExceptions();
    }
}

void ScriptEngine::load(const QString& loadFile) {
    QUrl url = resolveInclude(loadFile);
    emit loadScript(url.toString());
}

void ScriptEngine::nodeKilled(SharedNodePointer node) {
    _outgoingScriptAudioSequenceNumbers.remove(node->getUUID());
}
