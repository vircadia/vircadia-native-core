//
//  ScriptEngine.h
//  libraries/script-engine/src
//
//  Created by Brad Hefta-Gaub on 12/14/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptEngine_h
#define hifi_ScriptEngine_h

#include <vector>

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtScript/QScriptEngine>

#include <AudioScriptingInterface.h>
#include <VoxelsScriptingInterface.h>

#include <AvatarData.h>

#include "AbstractControllerScriptingInterface.h"
#include "Quat.h"
#include "ScriptUUID.h"
#include "Vec3.h"

class ParticlesScriptingInterface;

const QString NO_SCRIPT("");

const unsigned int SCRIPT_DATA_CALLBACK_USECS = floor(((1.0 / 60.0f) * 1000 * 1000) + 0.5);

class ScriptEngine : public QObject {
    Q_OBJECT
public:
    ScriptEngine(const QUrl& scriptURL,
                 AbstractControllerScriptingInterface* controllerScriptingInterface = NULL);

    ScriptEngine(const QString& scriptContents = NO_SCRIPT,
                 const QString& fileNameString = QString(""),
                 AbstractControllerScriptingInterface* controllerScriptingInterface = NULL);

    /// Access the VoxelsScriptingInterface in order to initialize it with a custom packet sender and jurisdiction listener
    static VoxelsScriptingInterface* getVoxelsScriptingInterface() { return &_voxelsScriptingInterface; }

    /// Access the ParticlesScriptingInterface in order to initialize it with a custom packet sender and jurisdiction listener
    static ParticlesScriptingInterface* getParticlesScriptingInterface() { return &_particlesScriptingInterface; }

    /// sets the script contents, will return false if failed, will fail if script is already running
    bool setScriptContents(const QString& scriptContents, const QString& fileNameString = QString(""));

    const QString& getScriptName() const { return _scriptName; }
    void cleanupMenuItems();

    void registerGlobalObject(const QString& name, QObject* object); /// registers a global object by name

    Q_INVOKABLE void setIsAvatar(bool isAvatar);
    bool isAvatar() const { return _isAvatar; }

    void setAvatarData(AvatarData* avatarData, const QString& objectName);

    bool isListeningToAudioStream() const { return _isListeningToAudioStream; }
    void setIsListeningToAudioStream(bool isListeningToAudioStream) { _isListeningToAudioStream = isListeningToAudioStream; }

    void setAvatarSound(Sound* avatarSound) { _avatarSound = avatarSound; }
    bool isPlayingAvatarSound() const { return _avatarSound != NULL; }

    void init();
    void run(); /// runs continuously until Agent.stop() is called
    void evaluate(); /// initializes the engine, and evaluates the script, but then returns control to caller

    void timerFired();

    bool hasScript() const { return !_scriptContents.isEmpty(); }

public slots:
    void stop();

    QObject* setInterval(const QScriptValue& function, int intervalMS);
    QObject* setTimeout(const QScriptValue& function, int timeoutMS);
    void clearInterval(QObject* timer) { stopTimer(reinterpret_cast<QTimer*>(timer)); }
    void clearTimeout(QObject* timer) { stopTimer(reinterpret_cast<QTimer*>(timer)); }
    void include(const QString& includeFile);

signals:
    void update(float deltaTime);
    void scriptEnding();
    void finished(const QString& fileNameString);
    void cleanupMenuItem(const QString& menuItemString);

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
    bool _isListeningToAudioStream;
    Sound* _avatarSound;
    int _numAvatarSoundSentBytes;

private:
    QUrl resolveInclude(const QString& include) const;
    void sendAvatarIdentityPacket();
    void sendAvatarBillboardPacket();

    QObject* setupTimerWithInterval(const QScriptValue& function, int intervalMS, bool isSingleShot);
    void stopTimer(QTimer* timer);

    static VoxelsScriptingInterface _voxelsScriptingInterface;
    static ParticlesScriptingInterface _particlesScriptingInterface;
    static int _scriptNumber;

    AbstractControllerScriptingInterface* _controllerScriptingInterface;
    AudioScriptingInterface _audioScriptingInterface;
    AvatarData* _avatarData;
    QString _scriptName;
    QString _fileNameString;
    Quat _quatLibrary;
    Vec3 _vec3Library;
    ScriptUUID _uuidLibrary;
};

#endif // hifi_ScriptEngine_h
