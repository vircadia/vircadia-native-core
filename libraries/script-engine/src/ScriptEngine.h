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
#include <AudioScriptingInterface.h>
#include <VoxelsScriptingInterface.h>

#include <AvatarData.h>

class ParticlesScriptingInterface;

#include "AbstractControllerScriptingInterface.h"
#include "DataServerScriptingInterface.h"
#include "Quat.h"

const QString NO_SCRIPT("");

class ScriptEngine : public QObject {
    Q_OBJECT
public:
    ScriptEngine(const QString& scriptContents = NO_SCRIPT, bool wantMenuItems = false,
		 const QString& scriptMenuName = QString(""), AbstractMenuInterface* menu = NULL,
                    AbstractControllerScriptingInterface* controllerScriptingInterface = NULL);

    ~ScriptEngine();

    /// Access the VoxelsScriptingInterface in order to initialize it with a custom packet sender and jurisdiction listener
    static VoxelsScriptingInterface* getVoxelsScriptingInterface() { return &_voxelsScriptingInterface; }

    /// Access the ParticlesScriptingInterface in order to initialize it with a custom packet sender and jurisdiction listener
    static ParticlesScriptingInterface* getParticlesScriptingInterface() { return &_particlesScriptingInterface; }
    
    /// Access the DataServerScriptingInterface for access to its underlying UUID
    const DataServerScriptingInterface& getDataServerScriptingInterface() { return _dataServerScriptingInterface; }

    /// sets the script contents, will return false if failed, will fail if script is already running
    bool setScriptContents(const QString& scriptContents);

    void setupMenuItems();
    void cleanMenuItems();

    void registerGlobalObject(const QString& name, QObject* object); /// registers a global object by name
    
    void setIsAvatar(bool isAvatar) { _isAvatar = isAvatar; }
    bool isAvatar() const { return _isAvatar; }
    
    void setAvatarData(AvatarData* avatarData, const QString& objectName);

public slots:
    void init();
    void run(); /// runs continuously until Agent.stop() is called
    void stop();
    void evaluate(); /// initializes the engine, and evaluates the script, but then returns control to caller

signals:
    void willSendAudioDataCallback();
    void willSendVisualDataCallback();
    void scriptEnding();
    void finished(const QString& fileNameString);

protected:
    
    QString _scriptContents;
    bool _isFinished;
    bool _isRunning;
    bool _isInitialized;
    QScriptEngine _engine;
    bool _isAvatar;

private:
    static VoxelsScriptingInterface _voxelsScriptingInterface;
    static ParticlesScriptingInterface _particlesScriptingInterface;
    AbstractControllerScriptingInterface* _controllerScriptingInterface;
    AudioScriptingInterface _audioScriptingInterface;
    DataServerScriptingInterface _dataServerScriptingInterface;
    AvatarData* _avatarData;
    bool _wantMenuItems;
    QString _scriptMenuName;
    QString _fileNameString;
    AbstractMenuInterface* _menu;
    static int _scriptNumber;
    Quat _quatLibrary;
};

#endif /* defined(__hifi__ScriptEngine__) */
