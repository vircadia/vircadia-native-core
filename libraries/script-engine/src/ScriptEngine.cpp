//
//  Agent.cpp
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

#include "ScriptEngine.h"

int ScriptEngine::_scriptNumber = 1;

ScriptEngine::ScriptEngine(const QString& scriptContents, bool wantMenuItems,
                                const char* scriptMenuName, AbstractMenuInterface* menu) {
    _scriptContents = scriptContents;
    _isFinished = false;
    _isRunning = false;
    
    // some clients will use these menu features
    _wantMenuItems = wantMenuItems;
    if (scriptMenuName) {
        _scriptMenuName = "Stop ";
        _scriptMenuName.append(scriptMenuName);
    } else {
        _scriptMenuName = "Stop Script ";
        _scriptNumber++;
        _scriptMenuName.append(_scriptNumber);
    }
    _menu = menu;
}

ScriptEngine::~ScriptEngine() {
    //printf("ScriptEngine::~ScriptEngine()...\n");
}


void ScriptEngine::setupMenuItems() {
    if (_menu && _wantMenuItems) {
        _menu->addActionToQMenuAndActionHash(_menu->getActiveScriptsMenu(), _scriptMenuName, 0, this, SLOT(stop()));
    }
}

void ScriptEngine::cleanMenuItems() {
    if (_menu && _wantMenuItems) {
        _menu->removeAction(_menu->getActiveScriptsMenu(), _scriptMenuName);
    }
}

bool ScriptEngine::setScriptContents(const QString& scriptContents) {
    if (_isRunning) {
        return false;
    }
    _scriptContents = scriptContents;
    return true;
}

void ScriptEngine::run() {
    _isRunning = true;
    QScriptEngine engine;
    
    _voxelScriptingInterface.init();
    _particleScriptingInterface.init();
    
    // register meta-type for glm::vec3 conversions
    registerMetaTypes(&engine);
    
    QScriptValue agentValue = engine.newQObject(this);
    engine.globalObject().setProperty("Agent", agentValue);
    
    QScriptValue voxelScripterValue =  engine.newQObject(&_voxelScriptingInterface);
    engine.globalObject().setProperty("Voxels", voxelScripterValue);

    QScriptValue particleScripterValue =  engine.newQObject(&_particleScriptingInterface);
    engine.globalObject().setProperty("Particles", particleScripterValue);
    
    QScriptValue treeScaleValue = engine.newVariant(QVariant(TREE_SCALE));
    engine.globalObject().setProperty("TREE_SCALE", treeScaleValue);
    
    const unsigned int VISUAL_DATA_CALLBACK_USECS = (1.0 / 60.0) * 1000 * 1000;
    
    // let the VoxelPacketSender know how frequently we plan to call it
    _voxelScriptingInterface.getVoxelPacketSender()->setProcessCallIntervalHint(VISUAL_DATA_CALLBACK_USECS);
    _particleScriptingInterface.getParticlePacketSender()->setProcessCallIntervalHint(VISUAL_DATA_CALLBACK_USECS);

    //qDebug() << "Script:\n" << _scriptContents << "\n";
    
    QScriptValue result = engine.evaluate(_scriptContents);
    qDebug() << "Evaluated script.\n";
    
    if (engine.hasUncaughtException()) {
        int line = engine.uncaughtExceptionLineNumber();
        qDebug() << "Uncaught exception at line" << line << ":" << result.toString() << "\n";
    }
    
    timeval startTime;
    gettimeofday(&startTime, NULL);
    
    int thisFrame = 0;

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
        
        bool willSendVisualDataCallBack = false;
        if (_voxelScriptingInterface.getVoxelPacketSender()->serversExist()) {            
            // allow the scripter's call back to setup visual data
            willSendVisualDataCallBack = true;
            
            // release the queue of edit voxel messages.
            _voxelScriptingInterface.getVoxelPacketSender()->releaseQueuedMessages();
            
            // since we're in non-threaded mode, call process so that the packets are sent
            if (!_voxelScriptingInterface.getVoxelPacketSender()->isThreaded()) {
                _voxelScriptingInterface.getVoxelPacketSender()->process();
            }
        }

        if (_particleScriptingInterface.getParticlePacketSender()->serversExist()) {
            // allow the scripter's call back to setup visual data
            willSendVisualDataCallBack = true;
            
            // release the queue of edit voxel messages.
            _particleScriptingInterface.getParticlePacketSender()->releaseQueuedMessages();
            
            // since we're in non-threaded mode, call process so that the packets are sent
            if (!_particleScriptingInterface.getParticlePacketSender()->isThreaded()) {
                _particleScriptingInterface.getParticlePacketSender()->process();
            }
        }
        
        if (willSendVisualDataCallBack) {
            emit willSendVisualDataCallback();
        }

        if (engine.hasUncaughtException()) {
            int line = engine.uncaughtExceptionLineNumber();
            qDebug() << "Uncaught exception at line" << line << ":" << engine.uncaughtException().toString() << "\n";
        }
    }
    cleanMenuItems();

    // If we were on a thread, then wait till it's done
    if (thread()) {
        thread()->quit();
    }

    emit finished();
    _isRunning = false;
}

void ScriptEngine::stop() { 
    _isFinished = true; 
}


