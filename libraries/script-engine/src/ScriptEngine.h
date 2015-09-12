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
#include <QtCore/QSet>
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
                 AbstractControllerScriptingInterface* controllerScriptingInterface = NULL,
                 bool wantSignals = true);

    ~ScriptEngine();

    /// Launch the script running in a dedicated thread. This will have the side effect of evalulating
    /// the current script contents and calling run(). Callers will likely want to register the script with external
    /// services before calling this.
    void runInThread();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOTE - these are NOT intended to be public interfaces available to scripts, the are only Q_INVOKABLE so we can
    //        properly ensure they are only called on the correct thread

    /// registers a global object by name
    Q_INVOKABLE void registerGlobalObject(const QString& name, QObject* object);

    /// registers a global getter/setter
    Q_INVOKABLE void registerGetterSetter(const QString& name, QScriptEngine::FunctionSignature getter,
                            QScriptEngine::FunctionSignature setter, const QString& parent = QString(""));
    
    /// register a global function
    Q_INVOKABLE void registerFunction(const QString& name, QScriptEngine::FunctionSignature fun, int numArguments = -1);

    /// register a function as a method on a previously registered global object
    Q_INVOKABLE void registerFunction(const QString& parent, const QString& name, QScriptEngine::FunctionSignature fun,
                          int numArguments = -1);

    /// evaluate some code in the context of the ScriptEngine and return the result
    Q_INVOKABLE QScriptValue evaluate(const QString& program, const QString& fileName = QString(), int lineNumber = 1); // this is also used by the script tool widget

    /// if the script engine is not already running, this will download the URL and start the process of seting it up
    /// to run... NOTE - this is used by Application currently to load the url. We don't really want it to be exposed 
    /// to scripts. we may not need this to be invokable
    void loadURL(const QUrl& scriptURL, bool reload);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOTE - these are intended to be public interfaces available to scripts 
    Q_INVOKABLE void addEventHandler(const EntityItemID& entityID, const QString& eventName, QScriptValue handler);
    Q_INVOKABLE void removeEventHandler(const EntityItemID& entityID, const QString& eventName, QScriptValue handler);

    Q_INVOKABLE void load(const QString& loadfile);
    Q_INVOKABLE void include(const QStringList& includeFiles, QScriptValue callback = QScriptValue());
    Q_INVOKABLE void include(const QString& includeFile, QScriptValue callback = QScriptValue());

    Q_INVOKABLE QObject* setInterval(const QScriptValue& function, int intervalMS);
    Q_INVOKABLE QObject* setTimeout(const QScriptValue& function, int timeoutMS);
    Q_INVOKABLE void clearInterval(QObject* timer) { stopTimer(reinterpret_cast<QTimer*>(timer)); }
    Q_INVOKABLE void clearTimeout(QObject* timer) { stopTimer(reinterpret_cast<QTimer*>(timer)); }
    Q_INVOKABLE void print(const QString& message);
    Q_INVOKABLE QUrl resolvePath(const QString& path) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOTE - this is intended to be a public interface for Agent scripts, and local scripts, but not for EntityScripts
    Q_INVOKABLE void stop();

    bool isFinished() const { return _isFinished; } // used by Application and ScriptWidget
    bool isRunning() const { return _isRunning; } // used by ScriptWidget

    static void stopAllScripts(QObject* application); // used by Application on shutdown

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOTE - These are the callback implementations for ScriptUser the get called by ScriptCache when the contents
    // of a script are available.
    virtual void scriptContentsAvailable(const QUrl& url, const QString& scriptContents);
    virtual void errorInLoadingScript(const QUrl& url);

    // These are currently used by Application to track if a script is user loaded or not. Consider finding a solution
    // inside of Application so that the ScriptEngine class is not polluted by this notion
    void setUserLoaded(bool isUserLoaded) { _isUserLoaded = isUserLoaded; }
    bool isUserLoaded() const { return _isUserLoaded; }

    // NOTE - this is used by the TypedArray implemetation. we need to review this for thread safety
    ArrayBufferClass* getArrayBufferClass() { return _arrayBufferClass; }

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
    QSet<QUrl> _includedURLs;
    bool _wantSignals = true;
    
private:
    QString getFilename() const;
    void waitTillDoneRunning();
    bool evaluatePending() const { return _evaluatesPending > 0; }
    void timerFired();
    void stopAllTimers();

    QObject* setupTimerWithInterval(const QScriptValue& function, int intervalMS, bool isSingleShot);
    void stopTimer(QTimer* timer);

    AbstractControllerScriptingInterface* _controllerScriptingInterface;
    AvatarData* _avatarData;
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

private:
    //FIXME- EntityTreeRender shouldn't be using these methods directly -- these methods need to be depricated from the public interfaces
    friend class EntityTreeRenderer;

    //FIXME - used in EntityTreeRenderer when evaluating entity scripts, which needs to be moved into ScriptEngine
    //        also used in Agent, but that would be fixed if Agent was using proper apis for loading content
    void setParentURL(const QString& parentURL) { _parentURL = parentURL; }

private:
    //FIXME- Agent shouldn't be using these methods directly -- these methods need to be depricated from the public interfaces
    friend class Agent;

    /// FIXME - DEPRICATED - remove callers and fix to use standard API
    /// sets the script contents, will return false if failed, will fail if script is already running
    bool setScriptContents(const QString& scriptContents, const QString& fileNameString = QString(""));

    /// FIXME - these should only be used internally
    void init();
    void run();

    // FIXME - all of these needto be removed and the code that depends on it in Agent.cpp should be moved into Agent.cpp
    void setIsAgent(bool isAgent) { _isAgent = isAgent; }
    void setIsAvatar(bool isAvatar);
    bool isAvatar() const { return _isAvatar; }
    void setAvatarData(AvatarData* avatarData, const QString& objectName);
    bool isListeningToAudioStream() const { return _isListeningToAudioStream; }
    void setIsListeningToAudioStream(bool isListeningToAudioStream) { _isListeningToAudioStream = isListeningToAudioStream; }
    void setAvatarSound(Sound* avatarSound) { _avatarSound = avatarSound; }
    bool isPlayingAvatarSound() const { return _avatarSound != NULL; }

    void sendAvatarIdentityPacket();
    void sendAvatarBillboardPacket();
};

#endif // hifi_ScriptEngine_h
