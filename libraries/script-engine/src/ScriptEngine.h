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
#include <QtCore/QStringList>

#include <QtScript/QScriptEngine>

#include <AnimationCache.h>
#include <AnimVariant.h>
#include <AvatarData.h>
#include <AvatarHashMap.h>
#include <LimitedNodeList.h>
#include <EntityItemID.h>
#include <EntitiesScriptEngineProvider.h>

#include "PointerEvent.h"
#include "ArrayBufferClass.h"
#include "AssetScriptingInterface.h"
#include "AudioScriptingInterface.h"
#include "Quat.h"
#include "Mat4.h"
#include "ScriptCache.h"
#include "ScriptUUID.h"
#include "Vec3.h"

class QScriptEngineDebugger;

static const QString NO_SCRIPT("");

static const int SCRIPT_FPS = 60;

class CallbackData {
public:
    QScriptValue function;
    EntityItemID definingEntityIdentifier;
    QUrl definingSandboxURL;
};

typedef QList<CallbackData> CallbackList;
typedef QHash<QString, CallbackList> RegisteredEventHandlers;

class EntityScriptDetails {
public:
    QString scriptText;
    QScriptValue scriptObject;
    int64_t lastModified;
    QUrl definingSandboxURL;
};

class ScriptEngine : public QScriptEngine, public ScriptUser, public EntitiesScriptEngineProvider {
    Q_OBJECT
public:
    ScriptEngine(const QString& scriptContents = NO_SCRIPT, const QString& fileNameString = QString(""));
    ~ScriptEngine();

    /// run the script in a dedicated thread. This will have the side effect of evalulating
    /// the current script contents and calling run(). Callers will likely want to register the script with external
    /// services before calling this.
    void runInThread();

    void runDebuggable();

    /// run the script in the callers thread, exit when stop() is called.
    void run();

    QString getFilename() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOTE - this is intended to be a public interface for Agent scripts, and local scripts, but not for EntityScripts
    Q_INVOKABLE void stop(bool marshal = false);

    // Stop any evaluating scripts and wait for the scripting thread to finish.
    void waitTillDoneRunning();

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

    /// registers a global object by name
    Q_INVOKABLE void registerValue(const QString& valueName, QScriptValue value);

    /// evaluate some code in the context of the ScriptEngine and return the result
    Q_INVOKABLE QScriptValue evaluate(const QString& program, const QString& fileName, int lineNumber = 1); // this is also used by the script tool widget

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
    Q_INVOKABLE QUrl resourcesPath() const;

    // Entity Script Related methods
    static void loadEntityScript(QWeakPointer<ScriptEngine> theEngine, const EntityItemID& entityID, const QString& entityScript, bool forceRedownload);
    Q_INVOKABLE void unloadEntityScript(const EntityItemID& entityID); // will call unload method
    Q_INVOKABLE void unloadAllEntityScripts();
    Q_INVOKABLE void callEntityScriptMethod(const EntityItemID& entityID, const QString& methodName,
                                            const QStringList& params = QStringList()) override;
    Q_INVOKABLE void callEntityScriptMethod(const EntityItemID& entityID, const QString& methodName, const PointerEvent& event);
    Q_INVOKABLE void callEntityScriptMethod(const EntityItemID& entityID, const QString& methodName, const EntityItemID& otherID, const Collision& collision);

    Q_INVOKABLE void requestGarbageCollection() { collectGarbage(); }

    Q_INVOKABLE QUuid generateUUID() { return QUuid::createUuid(); }

    bool isFinished() const { return _isFinished; } // used by Application and ScriptWidget
    bool isRunning() const { return _isRunning; } // used by ScriptWidget

    // this is used by code in ScriptEngines.cpp during the "reload all" operation
    bool isStopping() const { return _isStopping; }

    bool isDebuggable() const { return _debuggable; }

    void disconnectNonEssentialSignals();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOTE - These are the callback implementations for ScriptUser the get called by ScriptCache when the contents
    // of a script are available.
    virtual void scriptContentsAvailable(const QUrl& url, const QString& scriptContents) override;
    virtual void errorInLoadingScript(const QUrl& url) override;

    // These are currently used by Application to track if a script is user loaded or not. Consider finding a solution
    // inside of Application so that the ScriptEngine class is not polluted by this notion
    void setUserLoaded(bool isUserLoaded) { _isUserLoaded = isUserLoaded; }
    bool isUserLoaded() const { return _isUserLoaded; }

    // NOTE - this is used by the TypedArray implementation. we need to review this for thread safety
    ArrayBufferClass* getArrayBufferClass() { return _arrayBufferClass; }

    void setEmitScriptUpdatesFunction(std::function<bool()> func) { _emitScriptUpdates = func; }

public slots:
    void callAnimationStateHandler(QScriptValue callback, AnimVariantMap parameters, QStringList names, bool useNames, AnimVariantResultHandler resultHandler);
    void updateMemoryCost(const qint64&);

signals:
    void scriptLoaded(const QString& scriptFilename);
    void errorLoadingScript(const QString& scriptFilename);
    void update(float deltaTime);
    void scriptEnding();
    void finished(const QString& fileNameString, ScriptEngine* engine);
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
    std::atomic<bool> _isFinished { false };
    std::atomic<bool> _isRunning { false };
    std::atomic<bool> _isStopping { false };
    int _evaluatesPending { 0 };
    bool _isInitialized { false };
    QHash<QTimer*, CallbackData> _timerFunctionMap;
    QSet<QUrl> _includedURLs;
    QHash<EntityItemID, EntityScriptDetails> _entityScripts;
    bool _isThreaded { false };
    QScriptEngineDebugger* _debugger { nullptr };
    bool _debuggable { false };
    qint64 _lastUpdate;

    void init();

    bool evaluatePending() const { return _evaluatesPending > 0; }
    void timerFired();
    void stopAllTimers();
    void stopAllTimersForEntityScript(const EntityItemID& entityID);
    void refreshFileScript(const EntityItemID& entityID);

    void setParentURL(const QString& parentURL) { _parentURL = parentURL; }

    QObject* setupTimerWithInterval(const QScriptValue& function, int intervalMS, bool isSingleShot);
    void stopTimer(QTimer* timer);

    QString _fileNameString;
    Quat _quatLibrary;
    Vec3 _vec3Library;
    Mat4 _mat4Library;
    ScriptUUID _uuidLibrary;
    std::atomic<bool> _isUserLoaded { false };
    bool _isReloading { false };

    ArrayBufferClass* _arrayBufferClass;

    AssetScriptingInterface _assetScriptingInterface{ this };

    QHash<EntityItemID, RegisteredEventHandlers> _registeredHandlers;
    void forwardHandlerCall(const EntityItemID& entityID, const QString& eventName, QScriptValueList eventHanderArgs);
    Q_INVOKABLE void entityScriptContentAvailable(const EntityItemID& entityID, const QString& scriptOrURL, const QString& contents, bool isURL, bool success);

    EntityItemID currentEntityIdentifier {}; // Contains the defining entity script entity id during execution, if any. Empty for interface script execution.
    QUrl currentSandboxURL {}; // The toplevel url string for the entity script that loaded the code being executed, else empty.
    void doWithEnvironment(const EntityItemID& entityID, const QUrl& sandboxURL, std::function<void()> operation);
    void callWithEnvironment(const EntityItemID& entityID, const QUrl& sandboxURL, QScriptValue function, QScriptValue thisObject, QScriptValueList args);

    std::function<bool()> _emitScriptUpdates{ [](){ return true; }  };

};

#endif // hifi_ScriptEngine_h
