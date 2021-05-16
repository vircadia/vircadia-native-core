//
//  ScriptEngineQtScript.h
//  libraries/script-engine-qtscript/src
//
//  Created by Brad Hefta-Gaub on 12/14/13.
//  Copyright 2013 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptEngineQtScript_h
#define hifi_ScriptEngineQtScript_h

//#include <unordered_map>
//#include <vector>

//#include <QtCore/QUrl>
//#include <QtCore/QSet>
//#include <QtCore/QWaitCondition>
//#include <QtCore/QStringList>
//#include <QMap>

#include <QtCore/QByteArray>
#include <QtCore/QMetaEnum>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtScript/QScriptEngine>
#include <QtScriptTools/QScriptEngineDebugger>
#include <QtCore/QString>

//#include <AnimationCache.h>
//#include <AnimVariant.h>
//#include <AvatarData.h>
//#include <AvatarHashMap.h>
//#include <LimitedNodeList.h>
//#include <EntityItemID.h>
//#include <EntityScriptUtils.h>
#include <ScriptEngine.h>
#include <ScriptManager.h>

//#include "PointerEvent.h"
#include "ArrayBufferClass.h"
//#include "AssetScriptingInterface.h"
//#include "AudioScriptingInterface.h"
#include "BaseScriptEngine.h"
//#include "ExternalResource.h"
//#include "Quat.h"
//#include "Mat4.h"
//#include "ScriptCache.h"
//#include "ScriptUUID.h"
//#include "Vec3.h"
//#include "ConsoleScriptingInterface.h"
//#include "SettingHandle.h"
//#include "Profile.h"

//class QScriptEngineDebugger;
class ScriptManager;

Q_DECLARE_METATYPE(ScriptEngineQtScriptPointer)

/*@jsdoc
 * The <code>Script</code> API provides facilities for working with scripts.
 *
 * @namespace Script
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 * @hifi-server-entity
 * @hifi-assignment-client
 *
 * @property {string} context - The context that the script is running in:
 *     <ul>
 *       <li><code>"client"</code>: An Interface or avatar script.</li>
 *       <li><code>"entity_client"</code>: A client entity script.</li>
 *       <li><code>"entity_server"</code>: A server entity script.</li>
 *       <li><code>"agent"</code>: An assignment client script.</li>
 *     </ul>
 *     <em>Read-only.</em>
 * @property {string} type - The type of script that is running:
 *     <ul>
 *       <li><code>"client"</code>: An Interface script.</li>
 *       <li><code>"entity_client"</code>: A client entity script.</li>
 *       <li><code>"avatar"</code>: An avatar script.</li>
 *       <li><code>"entity_server"</code>: A server entity script.</li>
 *       <li><code>"agent"</code>: An assignment client script.</li>
 *     </ul>
 *     <em>Read-only.</em>
 * @property {string} filename - The filename of the script file.
 *     <em>Read-only.</em>
 * @property {Script.ResourceBuckets} ExternalPaths - External resource buckets.
 */
class ScriptEngineQtScript : public BaseScriptEngine, public ScriptEngine {
    Q_OBJECT
    using Base = BaseScriptEngine;

public:  // ScriptEngine implementation
    virtual ScriptValuePointer globalObject() const;
    virtual ScriptManager* manager() const;
    virtual ScriptValuePointer newArray(uint length = 0);
    virtual ScriptValuePointer newArrayBuffer(const QByteArray& message);
    virtual ScriptValuePointer newObject();
    virtual ScriptProgramPointer newProgram(const QString& sourceCode, const QString& fileName);
    virtual ScriptValuePointer newQObject(QObject* obj);
    virtual ScriptValuePointer newValue(bool value);
    virtual ScriptValuePointer newValue(int value);
    virtual ScriptValuePointer newValue(uint value);
    virtual ScriptValuePointer newValue(double value);
    virtual ScriptValuePointer newValue(const QString& value);
    virtual ScriptValuePointer newValue(const QLatin1String& value);
    virtual ScriptValuePointer newValue(const char* value);
    virtual ScriptValuePointer newVariant(const QVariant& value);
    virtual ScriptValuePointer nullValue();
    virtual void setDefaultPrototype(int metaTypeId, const ScriptValuePointer& prototype);
    virtual ScriptValuePointer undefinedValue();

public:
    ScriptEngineQtScript(ScriptManager* scriptManager = nullptr);
    ~ScriptEngineQtScript();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOTE - these are NOT intended to be public interfaces available to scripts, the are only Q_INVOKABLE so we can
    //        properly ensure they are only called on the correct thread

    /*@jsdoc
     * @function Script.registerGlobalObject
     * @param {string} name - Name.
     * @param {object} object - Object.
     * @deprecated This function is deprecated and will be removed.
     */
    /// registers a global object by name
    Q_INVOKABLE void registerGlobalObject(const QString& name, QObject* object);

    /*@jsdoc
     * @function Script.registerGetterSetter
     * @param {string} name - Name.
     * @param {function} getter - Getter.
     * @param {function} setter - Setter.
     * @param {string} [parent=""] - Parent.
     * @deprecated This function is deprecated and will be removed.
     */
    /// registers a global getter/setter
    Q_INVOKABLE void registerGetterSetter(const QString& name, QScriptEngine::FunctionSignature getter,
                                          QScriptEngine::FunctionSignature setter, const QString& parent = QString(""));

    /*@jsdoc
     * @function Script.registerFunction
     * @param {string} name - Name.
     * @param {function} function - Function.
     * @param {number} [numArguments=-1] - Number of arguments.
     * @deprecated This function is deprecated and will be removed.
     */
    /// register a global function
    Q_INVOKABLE void registerFunction(const QString& name, QScriptEngine::FunctionSignature fun, int numArguments = -1);

    /*@jsdoc
     * @function Script.registerFunction
     * @param {string} parent - Parent.
     * @param {string} name - Name.
     * @param {function} function - Function.
     * @param {number} [numArguments=-1] - Number of arguments.
     * @deprecated This function is deprecated and will be removed.
     */
    /// register a function as a method on a previously registered global object
    Q_INVOKABLE void registerFunction(const QString& parent, const QString& name, QScriptEngine::FunctionSignature fun,
                                      int numArguments = -1);

    /*@jsdoc
     * @function Script.registerEnum
     * @param {string} name - Name.
     * @param {object} enum - Enum.
     * @deprecated This function is deprecated and will be removed.
     */
    // WARNING: This function must be called after a registerGlobalObject that creates the namespace this enum is located in, or
    // the globalObject won't function. E.g., if you have a Foo object and a Foo.FooType enum, Foo must be registered first.
    /// registers a global enum
    Q_INVOKABLE void registerEnum(const QString& enumName, QMetaEnum newEnum);

    /*@jsdoc
     * @function Script.registerValue
     * @param {string} name - Name.
     * @param {object} value - Value.
     * @deprecated This function is deprecated and will be removed.
     */
    /// registers a global object by name
    Q_INVOKABLE void registerValue(const QString& valueName, QScriptValue value);

    /*@jsdoc
     * @function Script.evaluate
     * @param {string} program - Program.
     * @param {string} filename - File name.
     * @param {number} [lineNumber=-1] - Line number.
     * @returns {object} Object.
     * @deprecated This function is deprecated and will be removed.
     */
    /// evaluate some code in the context of the ScriptEngineQtScript and return the result
    Q_INVOKABLE QScriptValue evaluate(const QString& program, const QString& fileName, int lineNumber = 1); // this is also used by the script tool widget

    /*@jsdoc
     * @function Script.evaluateInClosure
     * @param {object} locals - Locals.
     * @param {object} program - Program.
     * @returns {object} Object.
     * @deprecated This function is deprecated and will be removed.
     */
    Q_INVOKABLE QScriptValue evaluateInClosure(const QScriptValue& locals, const QScriptProgram& program);

    /*@jsdoc
     * Checks whether the application was compiled as a debug build.
     * @function Script.isDebugMode
     * @returns {boolean} <code>true</code> if the application was compiled as a debug build, <code>false</code> if it was 
     *     compiled as a release build.
     */
    Q_INVOKABLE bool isDebugMode() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // MODULE related methods

    /*@jsdoc
     * Prints a message to the program log and emits {@link Script.printedMessage}.
     * <p>Alternatively, you can use {@link print} or one of the {@link console} API methods.</p>
     * @function Script.print
     * @param {string} message - The message to print.
     */
    Q_INVOKABLE void print(const QString& message);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Entity Script Related methods

    /*@jsdoc
     * Manually runs the JavaScript garbage collector which reclaims memory by disposing of objects that are no longer 
     * reachable.
     * @function Script.requestGarbageCollection
     */
    Q_INVOKABLE void requestGarbageCollection() { collectGarbage(); }

    bool isRunning() const { return _isRunning; } // used by ScriptWidget

    bool isDebuggable() const { return _debuggable; }

    void disconnectNonEssentialSignals();

    // NOTE - this is used by the TypedArray implementation. we need to review this for thread safety
    ArrayBufferClass* getArrayBufferClass() { return _arrayBufferClass; }

public slots:

    /*@jsdoc
     * @function Script.updateMemoryCost
     * @param {number} deltaSize - Delta size.
     * @deprecated This function is deprecated and will be removed.
     */
    void updateMemoryCost(const qint64&);

signals:

    /*@jsdoc
     * Triggered frequently at a system-determined interval.
     * @function Script.update
     * @param {number} deltaTime - The time since the last update, in s.
     * @returns {Signal}
     * @example <caption>Report script update intervals.</caption>
     * Script.update.connect(function (deltaTime) {
     *     print("Update: " + deltaTime);
     * });
     */
    void update(float deltaTime);

    /*@jsdoc
     * @function Script.finished
     * @param {string} filename - File name.
     * @param {object} engine - Engine.
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */
    void finished(const QString& fileNameString, ScriptEngineQtScriptPointer);

    /*@jsdoc
     * @function Script.cleanupMenuItem
     * @param {string} menuItem - Menu item.
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */
    void cleanupMenuItem(const QString& menuItemString);

    /*@jsdoc
     * Triggered when the script prints a message to the program log via {@link  print}, {@link Script.print}, 
     * {@link console.log}, {@link console.debug}, {@link console.group}, {@link console.groupEnd}, {@link console.time}, or 
     * {@link console.timeEnd}.
     * @function Script.printedMessage
     * @param {string} message - The message.
     * @param {string} scriptName - The name of the script that generated the message.
     * @returns {Signal}
     */
    void printedMessage(const QString& message, const QString& scriptName);

    /*@jsdoc
     * Triggered when the script generates an error, {@link console.error} or {@link console.exception} is called, or 
     * {@link console.assert} is called and fails.
     * @function Script.errorMessage
     * @param {string} message - The error message.
     * @param {string} scriptName - The name of the script that generated the error message.
     * @returns {Signal}
     */
    void errorMessage(const QString& message, const QString& scriptName);

    /*@jsdoc
     * Triggered when the script generates a warning or {@link console.warn} is called.
     * @function Script.warningMessage
     * @param {string} message - The warning message.
     * @param {string} scriptName - The name of the script that generated the warning message.
     * @returns {Signal}
     */
    void warningMessage(const QString& message, const QString& scriptName);

    /*@jsdoc
     * Triggered when the script generates an information message or {@link console.info} is called.
     * @function Script.infoMessage
     * @param {string} message - The information message.
     * @param {string} scriptName - The name of the script that generated the information message.
     * @returns {Signal}
     */
    void infoMessage(const QString& message, const QString& scriptName);

    /*@jsdoc
     * Triggered when the running state of the script changes, e.g., from running to stopping.
     * @function Script.runningStateChanged
     * @returns {Signal}
     */
    void runningStateChanged();

    /*@jsdoc
     * @function Script.clearDebugWindow
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */
    void clearDebugWindow();

    /*@jsdoc
     * @function Script.loadScript
     * @param {string} scriptName - Script name.
     * @param {boolean} isUserLoaded - Is user loaded.
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */
    void loadScript(const QString& scriptName, bool isUserLoaded);

    /*@jsdoc
     * @function Script.reloadScript
     * @param {string} scriptName - Script name.
     * @param {boolean} isUserLoaded - Is user loaded.
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */
    void reloadScript(const QString& scriptName, bool isUserLoaded);

    /*@jsdoc
     * Triggered when the script has stopped.
     * @function Script.doneRunning
     * @returns {Signal}
     */
    void doneRunning();

    /*@jsdoc
     * @function Script.entityScriptDetailsUpdated
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */
    // Emitted when an entity script is added or removed, or when the status of an entity
    // script is updated (goes from RUNNING to ERROR_RUNNING_SCRIPT, for example)
    void entityScriptDetailsUpdated();

protected:

    /*@jsdoc
     * @function Script.executeOnScriptThread
     * @param {function} function - Function.
     * @param {ConnectionType} [type=2] - Connection type.
     * @deprecated This function is deprecated and will be removed.
     */
    Q_INVOKABLE void executeOnScriptThread(std::function<void()> function, const Qt::ConnectionType& type = Qt::QueuedConnection );

    QPointer<ScriptManager> _manager;

    std::atomic<bool> _isRunning { false };

    bool _isThreaded { false };
    QScriptEngineDebugger* _debugger { nullptr };
    bool _debuggable { false };
    qint64 _lastUpdate;

    ArrayBufferClass* _arrayBufferClass;
};

#endif // hifi_ScriptEngineQtScript_h
