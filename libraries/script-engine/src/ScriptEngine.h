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

#include <unordered_map>
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
#include <EntityScriptUtils.h>

#include "PointerEvent.h"
#include "ArrayBufferClass.h"
#include "AssetScriptingInterface.h"
#include "AudioScriptingInterface.h"
#include "BaseScriptEngine.h"
#include "Quat.h"
#include "Mat4.h"
#include "ScriptCache.h"
#include "ScriptUUID.h"
#include "Vec3.h"
#include "ConsoleScriptingInterface.h"
#include "SettingHandle.h"
#include "Profile.h"

class QScriptEngineDebugger;

static const QString NO_SCRIPT("");

static const int SCRIPT_FPS = 60;
static const int DEFAULT_MAX_ENTITY_PPS = 9000;
static const int DEFAULT_ENTITY_PPS_PER_SCRIPT = 900;

class ScriptEngines;

Q_DECLARE_METATYPE(ScriptEnginePointer)

class CallbackData {
public:
    QScriptValue function;
    EntityItemID definingEntityIdentifier;
    QUrl definingSandboxURL;
};

class DeferredLoadEntity {
public:
    EntityItemID entityID;
    QString entityScript;
    //bool forceRedownload;
};

struct EntityScriptContentAvailable {
    EntityItemID entityID;
    QString scriptOrURL;
    QString contents;
    bool isURL;
    bool success;
    QString status;
};

typedef std::unordered_map<EntityItemID, EntityScriptContentAvailable> EntityScriptContentAvailableMap;

typedef QList<CallbackData> CallbackList;
typedef QHash<QString, CallbackList> RegisteredEventHandlers;

class EntityScriptDetails {
public:
    EntityScriptStatus status { EntityScriptStatus::PENDING };

    // If status indicates an error, this contains a human-readable string giving more information about the error.
    QString errorInfo { "" };

    QString scriptText { "" };
    QScriptValue scriptObject { QScriptValue() };
    int64_t lastModified { 0 };
    QUrl definingSandboxURL { QUrl("about:EntityScript") };
};

/**jsdoc
 * @namespace Script
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-server-entity
 * @hifi-assignment-client
 *
 * @property {string} context
 */
class ScriptEngine : public BaseScriptEngine, public EntitiesScriptEngineProvider {
    Q_OBJECT
    Q_PROPERTY(QString context READ getContext)
public:

    enum Context {
        CLIENT_SCRIPT,
        ENTITY_CLIENT_SCRIPT,
        ENTITY_SERVER_SCRIPT,
        AGENT_SCRIPT
    };

    enum Type {
        CLIENT,
        ENTITY_CLIENT,
        ENTITY_SERVER,
        AGENT,
        AVATAR
    };

    static int processLevelMaxRetries;
    ScriptEngine(Context context, const QString& scriptContents = NO_SCRIPT, const QString& fileNameString = QString("about:ScriptEngine"));
    ~ScriptEngine();

    /// run the script in a dedicated thread. This will have the side effect of evalulating
    /// the current script contents and calling run(). Callers will likely want to register the script with external
    /// services before calling this.
    void runInThread();

    void runDebuggable();

    /// run the script in the callers thread, exit when stop() is called.
    void run();

    QString getFilename() const;

    /**jsdoc
     * Stop the current script.
     * @function Script.stop
     * @param {boolean} [marshal=false]
     */
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOTE - this is intended to be a public interface for Agent scripts, and local scripts, but not for EntityScripts
    Q_INVOKABLE void stop(bool marshal = false);

    // Stop any evaluating scripts and wait for the scripting thread to finish.
    void waitTillDoneRunning();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOTE - these are NOT intended to be public interfaces available to scripts, the are only Q_INVOKABLE so we can
    //        properly ensure they are only called on the correct thread

    /**jsdoc
     * @function Script.registerGlobalObject
     * @param {string} name
     * @param {object} object
     */
    /// registers a global object by name
    Q_INVOKABLE void registerGlobalObject(const QString& name, QObject* object);

    /**jsdoc
     * @function Script.registerGetterSetter
     * @param {string} name
     * @param {object} getter
     * @param {object} setter
     * @param {string} [parent=""]
     */
    /// registers a global getter/setter
    Q_INVOKABLE void registerGetterSetter(const QString& name, QScriptEngine::FunctionSignature getter,
                                          QScriptEngine::FunctionSignature setter, const QString& parent = QString(""));

    /**jsdoc
     * @function Script.registerFunction
     * @param {string} name
     * @param {object} function
     * @param {number} [numArguments=-1]
     */
    /// register a global function
    Q_INVOKABLE void registerFunction(const QString& name, QScriptEngine::FunctionSignature fun, int numArguments = -1);

    /**jsdoc
     * @function Script.registerFunction
     * @param {string} parent
     * @param {string} name
     * @param {object} function
     * @param {number} [numArguments=-1]
     */
    /// register a function as a method on a previously registered global object
    Q_INVOKABLE void registerFunction(const QString& parent, const QString& name, QScriptEngine::FunctionSignature fun,
                                      int numArguments = -1);

    /**jsdoc
     * @function Script.registerValue
     * @param {string} name
     * @param {object} value
     */
    /// registers a global object by name
    Q_INVOKABLE void registerValue(const QString& valueName, QScriptValue value);

    /**jsdoc
     * @function Script.evaluate
     * @param {string} program
     * @param {string} filename
     * @param {number} [lineNumber=-1]
     * @returns {object}
     */
    /// evaluate some code in the context of the ScriptEngine and return the result
    Q_INVOKABLE QScriptValue evaluate(const QString& program, const QString& fileName, int lineNumber = 1); // this is also used by the script tool widget

    /**jsdoc
     * @function Script.evaluateInClosure
     * @param {object} locals
     * @param {object} program
     * @returns {object}
     */
    Q_INVOKABLE QScriptValue evaluateInClosure(const QScriptValue& locals, const QScriptProgram& program);

    /// if the script engine is not already running, this will download the URL and start the process of seting it up
    /// to run... NOTE - this is used by Application currently to load the url. We don't really want it to be exposed
    /// to scripts. we may not need this to be invokable
    void loadURL(const QUrl& scriptURL, bool reload);
    bool hasValidScriptSuffix(const QString& scriptFileName);

    /**jsdoc
     * @function Script.getContext
     * @returns {string}
     */
    Q_INVOKABLE QString getContext() const;

    /**jsdoc
     * @function Script.isClientScript
     * @returns {boolean}
     */
    Q_INVOKABLE bool isClientScript() const { return _context == CLIENT_SCRIPT; }

    /**jsdoc
     * @function Script.isDebugMode
     * @returns {boolean}
     */
    Q_INVOKABLE bool isDebugMode() const;

    /**jsdoc
     * @function Script.isEntityClientScript
     * @returns {boolean}
     */
    Q_INVOKABLE bool isEntityClientScript() const { return _context == ENTITY_CLIENT_SCRIPT; }

    /**jsdoc
     * @function Script.isEntityServerScript
     * @returns {boolean}
     */
    Q_INVOKABLE bool isEntityServerScript() const { return _context == ENTITY_SERVER_SCRIPT; }

    /**jsdoc
     * @function Script.isAgentScript
     * @returns {boolean}
     */
    Q_INVOKABLE bool isAgentScript() const { return _context == AGENT_SCRIPT; }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOTE - these are intended to be public interfaces available to scripts

    /**jsdoc
     * @function Script.addEventHandler
     * @param {Uuid} entityID
     * @param {string} eventName
     * @param {function} handler
     */
    Q_INVOKABLE void addEventHandler(const EntityItemID& entityID, const QString& eventName, QScriptValue handler);

    /**jsdoc
     * @function Script.removeEventHandler
     * @param {Uuid} entityID
     * @param {string} eventName
     * @param {function} handler
     */
    Q_INVOKABLE void removeEventHandler(const EntityItemID& entityID, const QString& eventName, QScriptValue handler);

    /**jsdoc
     * Start a new Interface or entity script.
     * @function Script.load
     * @param {string} filename - The URL of the script to load. Can be relative to the current script.
     * @example <caption>Load a script from another script.</caption>
     * // First file: scriptA.js
     * print("This is script A");
     *
     * // Second file: scriptB.js
     * print("This is script B");
     * Script.load("scriptA.js");
     *
     * // If you run scriptB.js you should see both scripts in the running scripts list.
     * // And you should see the following output:
     * // This is script B
     * // This is script A
     */
    Q_INVOKABLE void load(const QString& loadfile);

    /**jsdoc
     * Include JavaScript from other files in the current script. If a callback is specified the files are loaded and included 
     * asynchronously, otherwise they are included synchronously (i.e., script execution blocks while the files are included).
     * @function Script.include
     * @param {string[]} filenames - The URLs of the scripts to include. Each can be relative to the current script.
     * @param {function} [callback=null] - The function to call back when the scripts have been included. Can be an in-line 
     * function or the name of a function.
     */
    Q_INVOKABLE void include(const QStringList& includeFiles, QScriptValue callback = QScriptValue());

    /**jsdoc
     * Include JavaScript from another file in the current script. If a callback is specified the file is loaded and included 
     * asynchronously, otherwise it is included synchronously (i.e., script execution blocks while the file is included).
     * @function Script.include
     * @param {string} filename - The URL of the script to include. Can be relative to the current script.
     * @param {function} [callback=null] - The function to call back when the script has been included. Can be an in-line 
     * function or the name of a function.
     * @example <caption>Include a script file asynchronously.</caption>
     * // First file: scriptA.js
     * print("This is script A");
     *
     * // Second file: scriptB.js
     * print("This is script B");
     * Script.include("scriptA.js", function () {
     *     print("Script A has been included");
     * });
     *
     * // If you run scriptB.js you should see only scriptB.js in the running scripts list.
     * // And you should see the following output:
     * // This is script B
     * // This is script A
     * // Script A has been included
     */
    Q_INVOKABLE void include(const QString& includeFile, QScriptValue callback = QScriptValue());

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // MODULE related methods

    /**jsdoc
     * @function Script.require
     * @param {string} module
     */
    Q_INVOKABLE QScriptValue require(const QString& moduleId);

    /**jsdoc
     * @function Script.resetModuleCache
     * @param {boolean} [deleteScriptCache=false]
     */
    Q_INVOKABLE void resetModuleCache(bool deleteScriptCache = false);
    QScriptValue currentModule();
    bool registerModuleWithParent(const QScriptValue& module, const QScriptValue& parent);
    QScriptValue newModule(const QString& modulePath, const QScriptValue& parent = QScriptValue());
    QVariantMap fetchModuleSource(const QString& modulePath, const bool forceDownload = false);
    QScriptValue instantiateModule(const QScriptValue& module, const QString& sourceCode);

    /**jsdoc
     * Call a function at a set interval.
     * @function Script.setInterval
     * @param {function} function - The function to call. Can be an in-line function or the name of a function.
     * @param {number} interval - The interval at which to call the function, in ms.
     * @returns {object} A handle to the interval timer. Can be used by {@link Script.clearInterval}.
     * @example <caption>Print a message every second.</caption>
     * Script.setInterval(function () {
     *     print("Timer fired");
     * }, 1000);
    */
    Q_INVOKABLE QObject* setInterval(const QScriptValue& function, int intervalMS);

    /**jsdoc
     * Call a function after a delay.
     * @function Script.setTimeout
     * @param {function} function - The function to call. Can be an in-line function or the name of a function.
     * @param {number} timeout - The delay after which to call the function, in ms.
     * @returns {object} A handle to the timeout timer. Can be used by {@link Script.clearTimeout}.
     * @example <caption>Print a message after a second.</caption>
     * Script.setTimeout(function () {
     *     print("Timer fired");
     * }, 1000);
     */
    Q_INVOKABLE QObject* setTimeout(const QScriptValue& function, int timeoutMS);

    /**jsdoc
     * Stop an interval timer set by {@link Script.setInterval|setInterval}.
     * @function Script.clearInterval
     * @param {object} timer - The interval timer to clear.
     * @example <caption>Stop an interval timer.</caption>
     * // Print a message every second.
     * var timer = Script.setInterval(function () {
     *     print("Timer fired");
     * }, 1000);
     *
     * // Stop the timer after 10 seconds.
     * Script.setTimeout(function () {
     *     print("Stop timer");
     *     Script.clearInterval(timer);
     * }, 10000);
     */
    Q_INVOKABLE void clearInterval(QObject* timer) { stopTimer(reinterpret_cast<QTimer*>(timer)); }

    /**jsdoc
     * Clear a timeout timer set by {@link Script.setTimeout|setTimeout}.
     * @function Script.clearTimeout
     * @param {object} timer - The timeout timer to clear.
     * @example <caption>Stop a timeout timer.</caption>
     * // Print a message after two seconds.
     * var timer = Script.setTimeout(function () {
     *     print("Timer fired");
     * }, 2000);
     *
     * // Uncomment the following line to stop the timer from firing.
     * //Script.clearTimeout(timer);
     */
    Q_INVOKABLE void clearTimeout(QObject* timer) { stopTimer(reinterpret_cast<QTimer*>(timer)); }

    /**jsdoc
     * @function Script.print
     * @param {string} message
     */
    Q_INVOKABLE void print(const QString& message);

    /**jsdoc
     * Resolve a relative path to an absolute path.
     * @function Script.resolvePath
     * @param {string} path - The relative path to resolve.
     * @returns {string} The absolute path.
     */
    Q_INVOKABLE QUrl resolvePath(const QString& path) const;

    /**jsdoc
     * @function Script.resourcesPath
     * @returns {string}
     */
    Q_INVOKABLE QUrl resourcesPath() const;

    /**jsdoc
     * @function Script.beginProfileRange
     * @param {string} label
     */
    Q_INVOKABLE void beginProfileRange(const QString& label) const;

    /**jsdoc
     * @function Script.endProfileRange
     * @param {string} label
     */
    Q_INVOKABLE void endProfileRange(const QString& label) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Entity Script Related methods

    /**jsdoc
     * @function Script.isEntityScriptRunning
     * @param {Uuid} entityID
     * @returns {boolean}
     */
    Q_INVOKABLE bool isEntityScriptRunning(const EntityItemID& entityID) {
        QReadLocker locker { &_entityScriptsLock };
        auto it = _entityScripts.constFind(entityID);
        return it != _entityScripts.constEnd() && it->status == EntityScriptStatus::RUNNING;
    }
    QVariant cloneEntityScriptDetails(const EntityItemID& entityID);
    QFuture<QVariant> getLocalEntityScriptDetails(const EntityItemID& entityID) override;

    /**jsdoc
     * @function Script.loadEntityScript
     * @param {Uuid} entityID
     * @param {string} script
     * @param {boolean} forceRedownload
     */
    Q_INVOKABLE void loadEntityScript(const EntityItemID& entityID, const QString& entityScript, bool forceRedownload);

    /**jsdoc
     * @function Script.unloadEntityScript
     * @param {Uuid} entityID
     * @param {boolean} [shouldRemoveFromMap=false]
     */
    Q_INVOKABLE void unloadEntityScript(const EntityItemID& entityID, bool shouldRemoveFromMap = false); // will call unload method

    /**jsdoc
     * @function Script.unloadAllEntityScripts
     */
    Q_INVOKABLE void unloadAllEntityScripts();

    /**jsdoc
     * @function Script.callEntityScriptMethod
     * @param {Uuid} entityID
     * @param {string} methodName
     * @param {string[]} parameters
     * @param {Uuid} [remoteCallerID=Uuid.NULL]
     */
    Q_INVOKABLE void callEntityScriptMethod(const EntityItemID& entityID, const QString& methodName,
                                            const QStringList& params = QStringList(),
                                            const QUuid& remoteCallerID = QUuid()) override;

    /**jsdoc
     * @function Script.callEntityScriptMethod
     * @param {Uuid} entityID
     * @param {string} methodName
     * @param {PointerEvent} event
     */
    Q_INVOKABLE void callEntityScriptMethod(const EntityItemID& entityID, const QString& methodName, const PointerEvent& event);

    /**jsdoc
     * @function Script.callEntityScriptMethod
     * @param {Uuid} entityID
     * @param {string} methodName
     * @param {Uuid} otherID
     * @param {Collision} collision
     */
    Q_INVOKABLE void callEntityScriptMethod(const EntityItemID& entityID, const QString& methodName, const EntityItemID& otherID, const Collision& collision);

    /**jsdoc
     * @function Script.requestGarbageCollection
     */
    Q_INVOKABLE void requestGarbageCollection() { collectGarbage(); }

    /**jsdoc
     * @function Script.generateUUID
     * @returns {Uuid}
     */
    Q_INVOKABLE QUuid generateUUID() { return QUuid::createUuid(); }

    void setType(Type type) { _type = type; };
    Type getType() { return _type; };

    bool isFinished() const { return _isFinished; } // used by Application and ScriptWidget
    bool isRunning() const { return _isRunning; } // used by ScriptWidget

    // this is used by code in ScriptEngines.cpp during the "reload all" operation
    bool isStopping() const { return _isStopping; }

    bool isDebuggable() const { return _debuggable; }

    void disconnectNonEssentialSignals();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // These are currently used by Application to track if a script is user loaded or not. Consider finding a solution
    // inside of Application so that the ScriptEngine class is not polluted by this notion
    void setUserLoaded(bool isUserLoaded) { _isUserLoaded = isUserLoaded; }
    bool isUserLoaded() const { return _isUserLoaded; }

    void setQuitWhenFinished(const bool quitWhenFinished) { _quitWhenFinished = quitWhenFinished; }
    bool isQuitWhenFinished() const { return _quitWhenFinished; }

    // NOTE - this is used by the TypedArray implementation. we need to review this for thread safety
    ArrayBufferClass* getArrayBufferClass() { return _arrayBufferClass; }

    void setEmitScriptUpdatesFunction(std::function<bool()> func) { _emitScriptUpdates = func; }

    void scriptErrorMessage(const QString& message);
    void scriptWarningMessage(const QString& message);
    void scriptInfoMessage(const QString& message);
    void scriptPrintedMessage(const QString& message);
    void clearDebugLogWindow();
    int getNumRunningEntityScripts() const;
    bool getEntityScriptDetails(const EntityItemID& entityID, EntityScriptDetails &details) const;
    bool hasEntityScriptDetails(const EntityItemID& entityID) const;

public slots:

    /**jsdoc
     * @function Script.callAnimationStateHandler
     * @param {function} callback
     * @param {object} parameters
     * @param {string[]} names
     * @param {boolean} useNames
     * @param {object} resultHandler
     */
    void callAnimationStateHandler(QScriptValue callback, AnimVariantMap parameters, QStringList names, bool useNames, AnimVariantResultHandler resultHandler);

    /**jsdoc
     * @function Script.updateMemoryCost
     * @param {number} deltaSize
     */
    void updateMemoryCost(const qint64&);

signals:

    /**jsdoc
     * @function Script.scriptLoaded
     * @param {string} filename
     * @returns {Signal}
     */
    void scriptLoaded(const QString& scriptFilename);

    /**jsdoc
     * @function Script.errorLoadingScript
     * @param {string} filename
     * @returns {Signal}
     */
    void errorLoadingScript(const QString& scriptFilename);

    /**jsdoc
     * Triggered regularly at a system-determined frequency.
     * @function Script.update
     * @param {number} deltaTime - The time since the last update, in s.
     * @returns {Signal}
     */
    void update(float deltaTime);

    /**jsdoc
     * Triggered when the script is ending.
     * @function Script.scriptEnding
     * @returns {Signal}
     * @example <caption>Connect to the <code>scriptEnding</code> signal.</caption>
     * print("Script started");
     *
     * Script.scriptEnding.connect(function () {
     *     print("Script ending");
     * });
     *
     * Script.setTimeout(function () {
     *     print("Stopping script");
     *     Script.stop();
     * }, 1000);
     */
    void scriptEnding();

    /**jsdoc
     * @function Script.finished
     * @param {string} filename
     * @param {object} engine
     * @returns {Signal}
     */
    void finished(const QString& fileNameString, ScriptEnginePointer);

    /**jsdoc
     * @function Script.cleanupMenuItem
     * @param {string} menuItem
     * @returns {Signal}
     */
    void cleanupMenuItem(const QString& menuItemString);

    /**jsdoc
     * @function Script.printedMessage
     * @param {string} message
     * @param {string} scriptName
     * @returns {Signal}
     */
    void printedMessage(const QString& message, const QString& scriptName);

    /**jsdoc
     * @function Script.errorMessage
     * @param {string} message
     * @param {string} scriptName
     * @returns {Signal}
     */
    void errorMessage(const QString& message, const QString& scriptName);

    /**jsdoc
     * @function Script.warningMessage
     * @param {string} message
     * @param {string} scriptName
     * @returns {Signal}
     */
    void warningMessage(const QString& message, const QString& scriptName);

    /**jsdoc
     * @function Script.infoMessage
     * @param {string} message
     * @param {string} scriptName
     * @returns {Signal}
     */
    void infoMessage(const QString& message, const QString& scriptName);

    /**jsdoc
     * @function Script.runningStateChanged
     * @returns {Signal}
     */
    void runningStateChanged();

    /**jsdoc
     * @function Script.clearDebugWindow
     * @returns {Signal}
     */
    void clearDebugWindow();

    /**jsdoc
     * @function Script.loadScript
     * @param {string} scriptName
     * @param {boolean} isUserLoaded
     * @returns {Signal}
     */
    void loadScript(const QString& scriptName, bool isUserLoaded);

    /**jsdoc
     * @function Script.reloadScript
     * @param {string} scriptName
     * @param {boolean} isUserLoaded
     * @returns {Signal}
     */
    void reloadScript(const QString& scriptName, bool isUserLoaded);

    /**jsdoc
     * @function Script.doneRunning
     * @returns {Signal}
     */
    void doneRunning();

    /**jsdoc
     * @function Script.entityScriptDetailsUpdated
     * @returns {Signal}
     */
    // Emitted when an entity script is added or removed, or when the status of an entity
    // script is updated (goes from RUNNING to ERROR_RUNNING_SCRIPT, for example)
    void entityScriptDetailsUpdated();

protected:
    void init();

    /**jsdoc
     * @function Script.executeOnScriptThread
     * @param {object} function
     * @param {ConnectionType} [type=2]
     */
    Q_INVOKABLE void executeOnScriptThread(std::function<void()> function, const Qt::ConnectionType& type = Qt::QueuedConnection );

    /**jsdoc
     * @function Script._requireResolve
     * @param {string} module
     * @param {string} [relativeTo=""]
     * @returns {string}
     */
    // note: this is not meant to be called directly, but just to have QMetaObject take care of wiring it up in general;
    //   then inside of init() we just have to do "Script.require.resolve = Script._requireResolve;"
    Q_INVOKABLE QString _requireResolve(const QString& moduleId, const QString& relativeTo = QString());

    QString logException(const QScriptValue& exception);
    void timerFired();
    void stopAllTimers();
    void stopAllTimersForEntityScript(const EntityItemID& entityID);
    void refreshFileScript(const EntityItemID& entityID);
    void updateEntityScriptStatus(const EntityItemID& entityID, const EntityScriptStatus& status, const QString& errorInfo = QString());
    void setEntityScriptDetails(const EntityItemID& entityID, const EntityScriptDetails& details);
    void setParentURL(const QString& parentURL) { _parentURL = parentURL; }
    void processDeferredEntityLoads(const QString& entityScript, const EntityItemID& leaderID);

    QObject* setupTimerWithInterval(const QScriptValue& function, int intervalMS, bool isSingleShot);
    void stopTimer(QTimer* timer);

    QHash<EntityItemID, RegisteredEventHandlers> _registeredHandlers;
    void forwardHandlerCall(const EntityItemID& entityID, const QString& eventName, QScriptValueList eventHanderArgs);

    /**jsdoc
     * @function Script.entityScriptContentAvailable
     * @param {Uuid} entityID
     * @param {string} scriptOrURL
     * @param {string} contents
     * @param {boolean} isURL
     * @param {boolean} success
     * @param {string} status
     */
    Q_INVOKABLE void entityScriptContentAvailable(const EntityItemID& entityID, const QString& scriptOrURL, const QString& contents, bool isURL, bool success, const QString& status);

    EntityItemID currentEntityIdentifier {}; // Contains the defining entity script entity id during execution, if any. Empty for interface script execution.
    QUrl currentSandboxURL {}; // The toplevel url string for the entity script that loaded the code being executed, else empty.
    void doWithEnvironment(const EntityItemID& entityID, const QUrl& sandboxURL, std::function<void()> operation);
    void callWithEnvironment(const EntityItemID& entityID, const QUrl& sandboxURL, QScriptValue function, QScriptValue thisObject, QScriptValueList args);

    Context _context;
    Type _type;
    QString _scriptContents;
    QString _parentURL;
    std::atomic<bool> _isFinished { false };
    std::atomic<bool> _isRunning { false };
    std::atomic<bool> _isStopping { false };
    bool _isInitialized { false };
    QHash<QTimer*, CallbackData> _timerFunctionMap;
    QSet<QUrl> _includedURLs;
    mutable QReadWriteLock _entityScriptsLock { QReadWriteLock::Recursive };
    QHash<EntityItemID, EntityScriptDetails> _entityScripts;
    QHash<QString, EntityItemID> _occupiedScriptURLs;
    QList<DeferredLoadEntity> _deferredEntityLoads;
    EntityScriptContentAvailableMap _contentAvailableQueue;

    bool _isThreaded { false };
    QScriptEngineDebugger* _debugger { nullptr };
    bool _debuggable { false };
    qint64 _lastUpdate;

    QString _fileNameString;
    Quat _quatLibrary;
    Vec3 _vec3Library;
    Mat4 _mat4Library;
    ScriptUUID _uuidLibrary;
    ConsoleScriptingInterface _consoleScriptingInterface;
    std::atomic<bool> _isUserLoaded { false };
    bool _isReloading { false };

    std::atomic<bool> _quitWhenFinished;

    ArrayBufferClass* _arrayBufferClass;

    AssetScriptingInterface* _assetScriptingInterface;

    std::function<bool()> _emitScriptUpdates{ []() { return true; }  };

    std::recursive_mutex _lock;

    std::chrono::microseconds _totalTimerExecution { 0 };

    static const QString _SETTINGS_ENABLE_EXTENDED_MODULE_COMPAT;
    static const QString _SETTINGS_ENABLE_EXTENDED_EXCEPTIONS;

    Setting::Handle<bool> _enableExtendedJSExceptions { _SETTINGS_ENABLE_EXTENDED_EXCEPTIONS, true };
};

ScriptEnginePointer scriptEngineFactory(ScriptEngine::Context context,
                                        const QString& scriptContents,
                                        const QString& fileNameString);

#endif // hifi_ScriptEngine_h
