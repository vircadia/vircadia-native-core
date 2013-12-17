//
//  ScriptEngine.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/14/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__ScriptEngine__
#define __hifi__ScriptEngine__

#include <vector>

#include <QtScript/QScriptEngine>
#include <QtCore/QObject>
#include <QtCore/QUrl>

#include <AbstractMenuInterface.h>
#include <ParticleScriptingInterface.h>
#include <VoxelScriptingInterface.h>

const QString NO_SCRIPT("");

class ScriptEngine : public QObject {
    Q_OBJECT
public:
    ScriptEngine(const QString& scriptContents = NO_SCRIPT, bool wantMenuItems = false, 
                    const char* scriptMenuName = NULL, AbstractMenuInterface* menu = NULL);

    ~ScriptEngine();
    
    /// Access the VoxelScriptingInterface in order to initialize it with a custom packet sender and jurisdiction listener
    VoxelScriptingInterface* getVoxelScriptingInterface() { return &_voxelScriptingInterface; }

    /// Access the ParticleScriptingInterface in order to initialize it with a custom packet sender and jurisdiction listener
    ParticleScriptingInterface* getParticleScriptingInterface() { return &_particleScriptingInterface; }

    /// sets the script contents, will return false if failed, will fail if script is already running
    bool setScriptContents(const QString& scriptContents);

    void setupMenuItems();
    void cleanMenuItems();
    
public slots:
    void run();
    void stop();
    
signals:
    void willSendAudioDataCallback();
    void willSendVisualDataCallback();
    void finished();
protected:
    QString _scriptContents;
    bool _isFinished;
    bool _isRunning;


private:
    VoxelScriptingInterface _voxelScriptingInterface;
    ParticleScriptingInterface _particleScriptingInterface;
    bool _wantMenuItems;
    QString _scriptMenuName;
    AbstractMenuInterface* _menu;
    static int _scriptNumber;
};

#endif /* defined(__hifi__ScriptEngine__) */
