//
//  ScriptEngine.h
//  hifi
//
//  Created by Stephen Birarda on 7/1/13.
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

class ScriptEngine : public QObject {
    Q_OBJECT
public:
    ScriptEngine(QString scriptContents, bool wantMenuItems = false, 
                    const char* scriptMenuName = NULL, AbstractMenuInterface* menu = NULL);

    ~ScriptEngine();
    
    /// Access the VoxelScriptingInterface in order to initialize it with a custom packet sender and jurisdiction listener
    VoxelScriptingInterface* getVoxelScriptingInterface() { return &_voxelScriptingInterface; }

    /// Access the ParticleScriptingInterface in order to initialize it with a custom packet sender and jurisdiction listener
    ParticleScriptingInterface* getParticleScriptingInterface() { return &_particleScriptingInterface; }

    void setupMenuItems();
    void cleanMenuItems();
    
public slots:
    void run();
    void stop();
    void applicationAboutToQuit();
    
signals:
    void willSendAudioDataCallback();
    void willSendVisualDataCallback();
    void finished();
protected:
    QString _scriptContents;
    bool _isFinished;


private:
    VoxelScriptingInterface _voxelScriptingInterface;
    ParticleScriptingInterface _particleScriptingInterface;
    bool _wantMenuItems;
    QString _scriptMenuName;
    AbstractMenuInterface* _menu;
    static int _scriptNumber;
};

#endif /* defined(__hifi__ScriptEngine__) */
