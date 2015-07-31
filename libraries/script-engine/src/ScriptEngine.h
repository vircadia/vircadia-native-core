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
#include <QtCore/QWaitCondition>
#include <QtScript/QScriptEngine>

#include <AnimationCache.h>
#include <AvatarData.h>
#include <AvatarHashMap.h>
#include <LimitedNodeList.h>
#include <EntityItemID.h>

#include "AbstractControllerScriptingInterface.h"
#include "ArrayBufferClass.h"
#include "AudioScriptingInterface.h"
#include "Quat.h"
#include "ScriptCache.h"
#include "ScriptUUID.h"
#include "Vec3.h"

const QString NO_SCRIPT("");

const unsigned int SCRIPT_DATA_CALLBACK_USECS = floor(((1.0f / 60.0f) * 1000 * 1000) + 0.5f);

typedef QHash<QString, QScriptValueList> RegisteredEventHandlers;

class ScriptEngine : public QScriptEngine, public ScriptUser {
    Q_OBJECT
public:
    ScriptEngine(const QString& scriptContents = NO_SCRIPT,
                 const QString& fileNameString = QString(""),
                 AbstractControllerScriptingInterface* controllerScriptingInterface = NULL);

    ~ScriptEngine();

    ArrayBufferClass* getArrayBufferClass() { return _arrayBufferClass; }
    
    /// sets the script contents, will return false if failed, will fail if script is already running
    bool setScriptContents(const QString& scriptContents, const QString& fileNameString = QString(""));

    const QString& getScriptName() const { return _scriptName; }

    QScriptValue registerGlobalObject(const QString& name, QObject* object); /// registers a global object by name
    void registerGetterSetter(const QString& name, QScriptEngine::FunctionSignature getter,
                              QScriptEngine::FunctionSignature setter, QScriptValue object = QScriptValue::NullValue);
    void registerFunction(const QString& name, QScriptEngine::FunctionSignature fun, int numArguments = -1);
    void registerFunction(QScriptValue parent, const QString& name, QScriptEngine::FunctionSignature fun,
                          int numArguments = -1);

    Q_INVOKABLE void setIsAvatar(bool isAvatar);
    bool isAvatar() const { return _isAvatar; }

    void setAvatarData(AvatarData* avatarData, const QString& objectName);
    void setAvatarHashMap(AvatarHashMap* avatarHashMap, const QString& objectName);

    bool isListeningToAudioStream() const { return _isListeningToAudioStream; }
    void setIsListeningToAudioStream(bool isListeningToAudioStream) { _isListeningToAudioStream = isListeningToAudioStream; }

    void setAvatarSound(Sound* avatarSound) { _avatarSound = avatarSound; }
    bool isPlayingAvatarSound() const { return _avatarSound != NULL; }

    void init();
    void run(); /// runs continuously until Agent.stop() is called
    void evaluate(); /// initializes the engine, and evaluates the script, but then returns control to caller

    void timerFired();

    bool hasScript() const { return !_scriptContents.isEmpty(); }

    bool isFinished() const { return _isFinished; }
    bool isRunning() const { return _isRunning; }
    bool evaluatePending() const { return _evaluatesPending > 0; }

    void setUserLoaded(bool isUserLoaded) { _isUserLoaded = isUserLoaded;  }
    bool isUserLoaded() const { return _isUserLoaded; }
    
    void setIsAgent(bool isAgent) { _isAgent = isAgent; }

    void setParentURL(const QString& parentURL) { _parentURL = parentURL;  }
    
    QString getFilename() const;
    
    static void stopAllScripts(QObject* application);
    
    void waitTillDoneRunning();

    virtual void scriptContentsAvailable(const QUrl& url, const QString& scriptContents);
    virtual void errorInLoadingScript(const QUrl& url);

    Q_INVOKABLE void addEventHandler(const EntityItemID& entityID, const QString& eventName, QScriptValue handler);
    Q_INVOKABLE void removeEventHandler(const EntityItemID& entityID, const QString& eventName, QScriptValue handler);

public slots:
    void loadURL(const QUrl& scriptURL, bool reload);
    void stop();

    QScriptValue evaluate(const QString& program, const QString& fileName = QString(), int lineNumber = 1);
    QObject* setInterval(const QScriptValue& function, int intervalMS);
    QObject* setTimeout(const QScriptValue& function, int timeoutMS);
    void clearInterval(QObject* timer) { stopTimer(reinterpret_cast<QTimer*>(timer)); }
    void clearTimeout(QObject* timer) { stopTimer(reinterpret_cast<QTimer*>(timer)); }
    void include(const QStringList& includeFiles, QScriptValue callback = QScriptValue());
    void include(const QString& includeFile, QScriptValue callback = QScriptValue());
    void load(const QString& loadfile);
    void print(const QString& message);
    QUrl resolvePath(const QString& path) const;

    void nodeKilled(SharedNodePointer node);

signals:
    void scriptLoaded(const QString& scriptFilename);
    void errorLoadingScript(const QString& scriptFilename);
    void update(float deltaTime);
    void scriptEnding();
    void finished(const QString& fileNameString);
    void cleanupMenuItem(const QString& menuItemString);
    void printedMessage(const QString& message);
    void errorMessage(const QString& message);
    void runningStateChanged();
    void evaluationFinished(QScriptValue result, bool isException);
    void loadScript(const QString& scriptName, bool isUserLoaded);
    void reloadScript(const QString& scriptName, bool isUserLoaded);
    void doneRunning();

protected:
    QString _scriptContents;
    QString _parentURL;
    bool _isFinished;
    bool _isRunning;
    int _evaluatesPending = 0;
    bool _isInitialized;
    bool _isAvatar;
    QTimer* _avatarIdentityTimer;
    QTimer* _avatarBillboardTimer;
    QHash<QTimer*, QScriptValue> _timerFunctionMap;
    bool _isListeningToAudioStream;
    Sound* _avatarSound;
    int _numAvatarSoundSentBytes;
    bool _isAgent = false;
    
private:
    void stopAllTimers();
    void sendAvatarIdentityPacket();
    void sendAvatarBillboardPacket();

    QObject* setupTimerWithInterval(const QScriptValue& function, int intervalMS, bool isSingleShot);
    void stopTimer(QTimer* timer);

    AbstractControllerScriptingInterface* _controllerScriptingInterface;
    AvatarData* _avatarData;
    QString _scriptName;
    QString _fileNameString;
    Quat _quatLibrary;
    Vec3 _vec3Library;
    ScriptUUID _uuidLibrary;
    bool _isUserLoaded;
    bool _isReloading;

    ArrayBufferClass* _arrayBufferClass;

    QHash<QUuid, quint16> _outgoingScriptAudioSequenceNumbers;
    QHash<EntityItemID, RegisteredEventHandlers> _registeredHandlers;
    void generalHandler(const EntityItemID& entityID, const QString& eventName, std::function<QScriptValueList()> argGenerator);

private:
    static QSet<ScriptEngine*> _allKnownScriptEngines;
    static QMutex _allScriptsMutex;
    static bool _stoppingAllScripts;
    static bool _doneRunningThisScript;
};

#endif // hifi_ScriptEngine_h
