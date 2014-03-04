//
//  ScriptEngine.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/14/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include <AvatarData.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <UUID.h>
#include <VoxelConstants.h>
#include <VoxelDetail.h>
#include <ParticlesScriptingInterface.h>

#include <Sound.h>

#include "MenuItemProperties.h"
#include "ScriptEngine.h"

const unsigned int VISUAL_DATA_CALLBACK_USECS = (1.0 / 60.0) * 1000 * 1000;

int ScriptEngine::_scriptNumber = 1;
VoxelsScriptingInterface ScriptEngine::_voxelsScriptingInterface;
ParticlesScriptingInterface ScriptEngine::_particlesScriptingInterface;

static QScriptValue soundConstructor(QScriptContext* context, QScriptEngine* engine) {
    QUrl soundURL = QUrl(context->argument(0).toString());
    QScriptValue soundScriptValue = engine->newQObject(new Sound(soundURL), QScriptEngine::ScriptOwnership);

    return soundScriptValue;
}


ScriptEngine::ScriptEngine(const QString& scriptContents, bool wantMenuItems, const QString& fileNameString,
                           AbstractControllerScriptingInterface* controllerScriptingInterface) :
    _isAvatar(false),
    _avatarIdentityTimer(NULL),
    _avatarBillboardTimer(NULL),
    _avatarData(NULL)
{
    _scriptContents = scriptContents;
    _isFinished = false;
    _isRunning = false;
    _isInitialized = false;
    _fileNameString = fileNameString;

    QByteArray fileNameAscii = fileNameString.toLocal8Bit();
    const char* scriptMenuName = fileNameAscii.data();

    // some clients will use these menu features
    _wantMenuItems = wantMenuItems;
    if (!fileNameString.isEmpty()) {
        _scriptMenuName = "Stop ";
        _scriptMenuName.append(scriptMenuName);
        _scriptMenuName.append(QString(" [%1]").arg(_scriptNumber));
    } else {
        _scriptMenuName = "Stop Script ";
        _scriptMenuName.append(_scriptNumber);
    }
    _scriptNumber++;
    _controllerScriptingInterface = controllerScriptingInterface;
}

ScriptEngine::~ScriptEngine() {
    //printf("ScriptEngine::~ScriptEngine()...\n");
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
    _engine.globalObject().setProperty(objectName, QScriptValue());
    
    // give the script engine the new Avatar script property
    registerGlobalObject(objectName, _avatarData);
}

void ScriptEngine::cleanupMenuItems() {
    if (_wantMenuItems) {
        emit cleanupMenuItem(_scriptMenuName);
    }
}

bool ScriptEngine::setScriptContents(const QString& scriptContents) {
    if (_isRunning) {
        return false;
    }
    _scriptContents = scriptContents;
    return true;
}

Q_SCRIPT_DECLARE_QMETAOBJECT(AudioInjectorOptions, QObject*)

void ScriptEngine::init() {
    if (_isInitialized) {
        return; // only initialize once
    }

    _isInitialized = true;

    _voxelsScriptingInterface.init();
    _particlesScriptingInterface.init();

    // register various meta-types
    registerMetaTypes(&_engine);
    registerVoxelMetaTypes(&_engine);
    registerEventTypes(&_engine);
    registerMenuItemProperties(&_engine);
    
    qScriptRegisterMetaType(&_engine, ParticlePropertiesToScriptValue, ParticlePropertiesFromScriptValue);
    qScriptRegisterMetaType(&_engine, ParticleIDtoScriptValue, ParticleIDfromScriptValue);
    qScriptRegisterSequenceMetaType<QVector<ParticleID> >(&_engine);
    qScriptRegisterSequenceMetaType<QVector<glm::vec2> >(&_engine);

    QScriptValue soundConstructorValue = _engine.newFunction(soundConstructor);
    QScriptValue soundMetaObject = _engine.newQMetaObject(&Sound::staticMetaObject, soundConstructorValue);
    _engine.globalObject().setProperty("Sound", soundMetaObject);

    QScriptValue injectionOptionValue = _engine.scriptValueFromQMetaObject<AudioInjectorOptions>();
    _engine.globalObject().setProperty("AudioInjectionOptions", injectionOptionValue);

    registerGlobalObject("Script", this);
    registerGlobalObject("Audio", &_audioScriptingInterface);
    registerGlobalObject("Controller", _controllerScriptingInterface);
    registerGlobalObject("Particles", &_particlesScriptingInterface);
    registerGlobalObject("Quat", &_quatLibrary);
    registerGlobalObject("Vec3", &_vec3Library);

    registerGlobalObject("Voxels", &_voxelsScriptingInterface);

    QScriptValue treeScaleValue = _engine.newVariant(QVariant(TREE_SCALE));
    _engine.globalObject().setProperty("TREE_SCALE", treeScaleValue);

    // let the VoxelPacketSender know how frequently we plan to call it
    _voxelsScriptingInterface.getVoxelPacketSender()->setProcessCallIntervalHint(VISUAL_DATA_CALLBACK_USECS);
    _particlesScriptingInterface.getParticlePacketSender()->setProcessCallIntervalHint(VISUAL_DATA_CALLBACK_USECS);

}

void ScriptEngine::registerGlobalObject(const QString& name, QObject* object) {
    if (object) {
        QScriptValue value = _engine.newQObject(object);
        _engine.globalObject().setProperty(name, value);
    }
}

void ScriptEngine::evaluate() {
    if (!_isInitialized) {
        init();
    }

    QScriptValue result = _engine.evaluate(_scriptContents);

    if (_engine.hasUncaughtException()) {
        int line = _engine.uncaughtExceptionLineNumber();
        qDebug() << "Uncaught exception at line" << line << ":" << result.toString();
    }
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

    QScriptValue result = _engine.evaluate(_scriptContents);
    if (_engine.hasUncaughtException()) {
        int line = _engine.uncaughtExceptionLineNumber();
        qDebug() << "Uncaught exception at line" << line << ":" << result.toString();
    }

    timeval startTime;
    gettimeofday(&startTime, NULL);

    int thisFrame = 0;
    
    NodeList* nodeList = NodeList::getInstance();
    
    qint64 lastUpdate = usecTimestampNow();

    while (!_isFinished) {
        int usecToSleep = usecTimestamp(&startTime) + (thisFrame++ * VISUAL_DATA_CALLBACK_USECS) - usecTimestampNow();
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
        
        if (_isAvatar && _avatarData) {
            QByteArray avatarPacket = byteArrayWithPopulatedHeader(PacketTypeAvatarData);
            avatarPacket.append(_avatarData->toByteArray());
            
            nodeList->broadcastToNodes(avatarPacket, NodeSet() << NodeType::AvatarMixer);
        }

        qint64 now = usecTimestampNow();
        float deltaTime = (float)(now - lastUpdate)/(float)USECS_PER_SECOND;
        emit update(deltaTime);
        lastUpdate = now;

        if (_engine.hasUncaughtException()) {
            int line = _engine.uncaughtExceptionLineNumber();
            qDebug() << "Uncaught exception at line" << line << ":" << _engine.uncaughtException().toString();
        }
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
    
    cleanupMenuItems();

    // If we were on a thread, then wait till it's done
    if (thread()) {
        thread()->quit();
    }

    emit finished(_fileNameString);
    
    _isRunning = false;
}

void ScriptEngine::stop() {
    _isFinished = true;
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
        qDebug() << "Deleting a single shot timer";
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
