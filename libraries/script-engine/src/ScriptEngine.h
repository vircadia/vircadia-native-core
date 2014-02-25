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

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtScript/QScriptEngine>

#include <AbstractMenuInterface.h>
#include <AudioScriptingInterface.h>
#include <VoxelsScriptingInterface.h>

#include <AvatarData.h>

class ParticlesScriptingInterface;

#include "AbstractControllerScriptingInterface.h"
#include "Quat.h"
#include "Vec3.h"

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

    /// sets the script contents, will return false if failed, will fail if script is already running
    bool setScriptContents(const QString& scriptContents);

    void setupMenuItems();
    void cleanMenuItems();

    void registerGlobalObject(const QString& name, QObject* object); /// registers a global object by name
    
    Q_INVOKABLE void setIsAvatar(bool isAvatar);
    bool isAvatar() const { return _isAvatar; }
    
    void setAvatarData(AvatarData* avatarData, const QString& objectName);
    
    void init();
    void run(); /// runs continuously until Agent.stop() is called
    void evaluate(); /// initializes the engine, and evaluates the script, but then returns control to caller
    
    void timerFired();

public slots:
    void stop();
    
    QObject* setInterval(const QScriptValue& function, int intervalMS);
    QObject* setTimeout(const QScriptValue& function, int timeoutMS);
    void clearInterval(QObject* timer) { stopTimer(reinterpret_cast<QTimer*>(timer)); }
    void clearTimeout(QObject* timer) { stopTimer(reinterpret_cast<QTimer*>(timer)); }
    
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
    QTimer* _avatarIdentityTimer;
    QTimer* _avatarBillboardTimer;
    QHash<QTimer*, QScriptValue> _timerFunctionMap;

private:
    void sendAvatarIdentityPacket();
    void sendAvatarBillboardPacket();
    
    QObject* setupTimerWithInterval(const QScriptValue& function, int intervalMS, bool isSingleShot);
    void stopTimer(QTimer* timer);
    
    static VoxelsScriptingInterface _voxelsScriptingInterface;
    static ParticlesScriptingInterface _particlesScriptingInterface;
    AbstractControllerScriptingInterface* _controllerScriptingInterface;
    AudioScriptingInterface _audioScriptingInterface;
    AvatarData* _avatarData;
    bool _wantMenuItems;
    QString _scriptMenuName;
    QString _fileNameString;
    AbstractMenuInterface* _menu;
    static int _scriptNumber;
    Quat _quatLibrary;
    Vec3 _vec3Library;
};

#endif /* defined(__hifi__ScriptEngine__) */
