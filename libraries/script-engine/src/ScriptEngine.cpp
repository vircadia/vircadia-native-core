//
//  ScriptEngine.cpp
//  libraries/script-engine/src
//
//  Created by Brad Hefta-Gaub on 12/14/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <chrono>
#include <thread>

#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtCore/QRegularExpression>

#include <QtCore/QFuture>
#include <QtConcurrent/QtConcurrentRun>

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMenu>

#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include <QtScript/QScriptContextInfo>
#include <QtScript/QScriptValue>
#include <QtScript/QScriptValueIterator>

#include <QtScriptTools/QScriptEngineDebugger>

#include <shared/QtHelpers.h>
#include <AudioConstants.h>
#include <AudioEffectOptions.h>
#include <AvatarData.h>
#include <DebugDraw.h>
#include <EntityScriptingInterface.h>
#include <MessagesClient.h>
#include <NetworkAccessManager.h>
#include <PathUtils.h>
#include <ResourceScriptingInterface.h>
#include <UserActivityLoggerScriptingInterface.h>
#include <NodeList.h>
#include <ScriptAvatarData.h>
#include <udt/PacketHeaders.h>
#include <UUID.h>

#include <controllers/ScriptingInterface.h>
#include <AnimationObject.h>

#include "ArrayBufferViewClass.h"
#include "BatchLoader.h"
#include "BaseScriptEngine.h"
#include "DataViewClass.h"
#include "EventTypes.h"
#include "FileScriptingInterface.h" // unzip project
#include "MenuItemProperties.h"
#include "ScriptAudioInjector.h"
#include "ScriptAvatarData.h"
#include "ScriptCache.h"
#include "ScriptEngineLogging.h"
#include "ScriptEngine.h"
#include "TypedArrays.h"
#include "XMLHttpRequestClass.h"
#include "WebSocketClass.h"
#include "RecordingScriptingInterface.h"
#include "ScriptEngines.h"
#include "ModelScriptingInterface.h"


#include <Profile.h>

#include "../../midi/src/Midi.h"        // FIXME why won't a simpler include work?
#include "MIDIEvent.h"

const QString ScriptEngine::_SETTINGS_ENABLE_EXTENDED_EXCEPTIONS {
    "com.highfidelity.experimental.enableExtendedJSExceptions"
};

static const int MAX_MODULE_ID_LENGTH { 4096 };
static const int MAX_DEBUG_VALUE_LENGTH { 80 };

static const QScriptEngine::QObjectWrapOptions DEFAULT_QOBJECT_WRAP_OPTIONS =
                QScriptEngine::ExcludeDeleteLater | QScriptEngine::ExcludeChildObjects;
static const QScriptValue::PropertyFlags READONLY_PROP_FLAGS { QScriptValue::ReadOnly | QScriptValue::Undeletable };
static const QScriptValue::PropertyFlags READONLY_HIDDEN_PROP_FLAGS { READONLY_PROP_FLAGS | QScriptValue::SkipInEnumeration };

static const bool HIFI_AUTOREFRESH_FILE_SCRIPTS { true };

Q_DECLARE_METATYPE(QScriptEngine::FunctionSignature)
int functionSignatureMetaID = qRegisterMetaType<QScriptEngine::FunctionSignature>();

Q_LOGGING_CATEGORY(scriptengineScript, "hifi.scriptengine.script")

static QScriptValue debugPrint(QScriptContext* context, QScriptEngine* engine) {
    QString message = "";
    for (int i = 0; i < context->argumentCount(); i++) {
        if (i > 0) {
            message += " ";
        }
        message += context->argument(i).toString();
    }
    qCDebug(scriptengineScript).noquote() << message;  // noquote() so that \n is treated as newline

    if (ScriptEngine *scriptEngine = qobject_cast<ScriptEngine*>(engine)) {
        scriptEngine->print(message);
    }

    return QScriptValue();
}

Q_DECLARE_METATYPE(controller::InputController*)
//static int inputControllerPointerId = qRegisterMetaType<controller::InputController*>();

QScriptValue inputControllerToScriptValue(QScriptEngine *engine, controller::InputController* const &in) {
    return engine->newQObject(in, QScriptEngine::QtOwnership, DEFAULT_QOBJECT_WRAP_OPTIONS);
}

void inputControllerFromScriptValue(const QScriptValue &object, controller::InputController* &out) {
    out = qobject_cast<controller::InputController*>(object.toQObject());
}

// FIXME Come up with a way to properly encode entity IDs in filename
// The purpose of the following two function is to embed entity ids into entity script filenames
// so that they show up in stacktraces
//
// Extract the url portion of a url that has been encoded with encodeEntityIdIntoEntityUrl(...)
QString extractUrlFromEntityUrl(const QString& url) {
    auto parts = url.split(' ', QString::SkipEmptyParts);
    if (parts.length() > 0) {
        return parts[0];
    } else {
        return "";
    }
}

// Encode an entity id into an entity url
// Example: http://www.example.com/some/path.js [EntityID:{9fdd355f-d226-4887-9484-44432d29520e}]
QString encodeEntityIdIntoEntityUrl(const QString& url, const QString& entityID) {
    return url + " [EntityID:" + entityID + "]";
}

QString ScriptEngine::logException(const QScriptValue& exception) {
    auto message = formatException(exception, _enableExtendedJSExceptions.get());
    scriptErrorMessage(message);
    return message;
}

ScriptEnginePointer scriptEngineFactory(ScriptEngine::Context context,
                                                 const QString& scriptContents,
                                                 const QString& fileNameString) {
    ScriptEngine* engine = new ScriptEngine(context, scriptContents, fileNameString);
    ScriptEnginePointer engineSP = ScriptEnginePointer(engine);
    DependencyManager::get<ScriptEngines>()->addScriptEngine(qSharedPointerCast<ScriptEngine>(engineSP));
    return engineSP;
}

int ScriptEngine::processLevelMaxRetries { ScriptRequest::MAX_RETRIES };
ScriptEngine::ScriptEngine(Context context, const QString& scriptContents, const QString& fileNameString) :
    BaseScriptEngine(),
    _context(context),
    _scriptContents(scriptContents),
    _timerFunctionMap(),
    _fileNameString(fileNameString),
    _arrayBufferClass(new ArrayBufferClass(this)),
    // don't delete `ScriptEngines` until all `ScriptEngine`s are gone
    _scriptEngines(DependencyManager::get<ScriptEngines>())
{
    connect(this, &QScriptEngine::signalHandlerException, this, [this](const QScriptValue& exception) {
        if (hasUncaughtException()) {
            // the engine's uncaughtException() seems to produce much better stack traces here
            emit unhandledException(cloneUncaughtException("signalHandlerException"));
            clearExceptions();
        } else {
            // ... but may not always be available -- so if needed we fallback to the passed exception
            emit unhandledException(exception);
        }
    }, Qt::DirectConnection);

    setProcessEventsInterval(MSECS_PER_SECOND);
    if (isEntityServerScript()) {
        qCDebug(scriptengine) << "isEntityServerScript() -- limiting maxRetries to 1";
        processLevelMaxRetries = 1;
    }

    // this is where all unhandled exceptions end up getting logged
    connect(this, &BaseScriptEngine::unhandledException, this, [this](const QScriptValue& err) {
        auto output = err.engine() == this ? err : makeError(err);
        if (!output.property("detail").isValid()) {
            output.setProperty("detail", "UnhandledException");
        }
        logException(output);
    });
}

QString ScriptEngine::getContext() const {
    switch (_context) {
        case CLIENT_SCRIPT:
            return "client";
        case ENTITY_CLIENT_SCRIPT:
            return "entity_client";
        case ENTITY_SERVER_SCRIPT:
            return "entity_server";
        case AGENT_SCRIPT:
            return "agent";
        default:
            return "unknown";
    }
    return "unknown";
}

ScriptEngine::~ScriptEngine() {
    auto scriptEngines = DependencyManager::get<ScriptEngines>();
    if (scriptEngines) {
        scriptEngines->removeScriptEngine(qSharedPointerCast<ScriptEngine>(sharedFromThis()));
    }
}

void ScriptEngine::disconnectNonEssentialSignals() {
    disconnect();
    QThread* workerThread;
    // Ensure the thread should be running, and does exist
    if (_isRunning && _isThreaded && (workerThread = thread())) {
        connect(this, &ScriptEngine::doneRunning, workerThread, &QThread::quit);
        connect(this, &QObject::destroyed, workerThread, &QObject::deleteLater);
    }
}

void ScriptEngine::runDebuggable() {
    static QMenuBar* menuBar { nullptr };
    static QMenu* scriptDebugMenu { nullptr };
    static size_t scriptMenuCount { 0 };
    if (!scriptDebugMenu) {
        for (auto window : qApp->topLevelWidgets()) {
            auto mainWindow = qobject_cast<QMainWindow*>(window);
            if (mainWindow) {
                menuBar = mainWindow->menuBar();
                break;
            }
        }
        if (menuBar) {
            scriptDebugMenu = menuBar->addMenu("Script Debug");
        }
    }

    init();
    _isRunning = true;
    _debuggable = true;
    _debugger = new QScriptEngineDebugger(this);
    _debugger->attachTo(this);

    QMenu* parentMenu = scriptDebugMenu;
    QMenu* scriptMenu { nullptr };
    if (parentMenu) {
        ++scriptMenuCount;
        scriptMenu = parentMenu->addMenu(_fileNameString);
        scriptMenu->addMenu(_debugger->createStandardMenu(qApp->activeWindow()));
    } else {
        qWarning() << "Unable to add script debug menu";
    }

    QScriptValue result = evaluate(_scriptContents, _fileNameString);

    _lastUpdate = usecTimestampNow();
    QTimer* timer = new QTimer(this);
    connect(this, &ScriptEngine::finished, [this, timer, parentMenu, scriptMenu] {
        if (scriptMenu) {
            parentMenu->removeAction(scriptMenu->menuAction());
            --scriptMenuCount;
            if (0 == scriptMenuCount) {
                menuBar->removeAction(scriptDebugMenu->menuAction());
                scriptDebugMenu = nullptr;
            }
        }
        disconnect(timer);
    });

    connect(timer, &QTimer::timeout, [this, timer] {
        if (_isFinished) {
            if (!_isRunning) {
                return;
            }
            stopAllTimers(); // make sure all our timers are stopped if the script is ending

            emit scriptEnding();
            emit finished(_fileNameString, qSharedPointerCast<ScriptEngine>(sharedFromThis()));
            _isRunning = false;

            emit runningStateChanged();
            emit doneRunning();

            timer->deleteLater();
            return;
        }

        qint64 now = usecTimestampNow();
        // we check for 'now' in the past in case people set their clock back
        if (_lastUpdate < now) {
            float deltaTime = (float)(now - _lastUpdate) / (float)USECS_PER_SECOND;
            if (!_isFinished) {
                emit update(deltaTime);
            }
        }
        _lastUpdate = now;

        // only clear exceptions if we are not in the middle of evaluating
        if (!isEvaluating() && hasUncaughtException()) {
            qCWarning(scriptengine) << __FUNCTION__ << "---------- UNCAUGHT EXCEPTION --------";
            qCWarning(scriptengine) << "runDebuggable" << uncaughtException().toString();
            logException(__FUNCTION__);
            clearExceptions();
        }
    });

    timer->start(10);
}


void ScriptEngine::runInThread() {
    Q_ASSERT_X(!_isThreaded, "ScriptEngine::runInThread()", "runInThread should not be called more than once");

    if (_isThreaded) {
        qCWarning(scriptengine) << "ScriptEngine already running in thread: " << getFilename();
        return;
    }

    _isThreaded = true;

    // The thread interface cannot live on itself, and we want to move this into the thread, so
    // the thread cannot have this as a parent.
    QThread* workerThread = new QThread();
    workerThread->setObjectName(QString("js:") + getFilename().replace("about:",""));
    moveToThread(workerThread);

    // NOTE: If you connect any essential signals for proper shutdown or cleanup of
    // the script engine, make sure to add code to "reconnect" them to the
    // disconnectNonEssentialSignals() method
    connect(workerThread, &QThread::started, this, &ScriptEngine::run);
    connect(this, &ScriptEngine::doneRunning, workerThread, &QThread::quit);
    connect(this, &QObject::destroyed, workerThread, &QObject::deleteLater);

    workerThread->start();
}

void ScriptEngine::executeOnScriptThread(std::function<void()> function, const Qt::ConnectionType& type ) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "executeOnScriptThread", type, Q_ARG(std::function<void()>, function));
        return;
    }

    function();
}

void ScriptEngine::waitTillDoneRunning() {
    auto workerThread = thread();

    if (_isThreaded && workerThread) {
        // We should never be waiting (blocking) on our own thread
        assert(workerThread != QThread::currentThread());

        // Engine should be stopped already, but be defensive
        stop();

        auto startedWaiting = usecTimestampNow();
        while (workerThread->isRunning()) {
            // If the final evaluation takes too long, then tell the script engine to stop running
            auto elapsedUsecs = usecTimestampNow() - startedWaiting;
            static const auto MAX_SCRIPT_EVALUATION_TIME = USECS_PER_SECOND;
            if (elapsedUsecs > MAX_SCRIPT_EVALUATION_TIME) {
                workerThread->quit();

                if (isEvaluating()) {
                    qCWarning(scriptengine) << "Script Engine has been running too long, aborting:" << getFilename();
                    abortEvaluation();
                } else {
                    qCWarning(scriptengine) << "Script Engine has been running too long, throwing:" << getFilename();
                    auto context = currentContext();
                    if (context) {
                        context->throwError("Timed out during shutdown");
                    }
                }

                // Wait for the scripting thread to stop running, as
                // flooding it with aborts/exceptions will persist it longer
                static const auto MAX_SCRIPT_QUITTING_TIME = 0.5 * MSECS_PER_SECOND;
                if (workerThread->wait(MAX_SCRIPT_QUITTING_TIME)) {
                    workerThread->terminate();
                }
            }

            // NOTE: This will be called on the main application thread (among other threads) from stopAllScripts.
            //       The thread will need to continue to process events, because
            //       the scripts will likely need to marshall messages across to the main thread, e.g.
            //       if they access Settings or Menu in any of their shutdown code. So:
            // Process events for this thread, allowing invokeMethod calls to pass between threads.
            QCoreApplication::processEvents();

            // Avoid a pure busy wait
            QThread::yieldCurrentThread();
        }

        scriptInfoMessage("Script Engine has stopped:" + getFilename());
    }
}

QString ScriptEngine::getFilename() const {
    QStringList fileNameParts = _fileNameString.split("/");
    QString lastPart;
    if (!fileNameParts.isEmpty()) {
        lastPart = fileNameParts.last();
    }
    return lastPart;
}

bool ScriptEngine::hasValidScriptSuffix(const QString& scriptFileName) {
    QFileInfo fileInfo(scriptFileName);
    QString scriptSuffixToLower = fileInfo.completeSuffix().toLower();
    return scriptSuffixToLower.contains(QString("js"), Qt::CaseInsensitive);
}

void ScriptEngine::loadURL(const QUrl& scriptURL, bool reload) {
    if (_isRunning) {
        return;
    }

    QUrl url = expandScriptUrl(scriptURL);
    _fileNameString = url.toString();
    _isReloading = reload;

    // Check that script has a supported file extension
    if (!hasValidScriptSuffix(_fileNameString)) {
        scriptErrorMessage("File extension of file: " + _fileNameString + " is not a currently supported script type");
        emit errorLoadingScript(_fileNameString);
        return;
    }

    const auto maxRetries = 0; // for consistency with previous scriptCache->getScript() behavior
    auto scriptCache = DependencyManager::get<ScriptCache>();
    scriptCache->getScriptContents(url.toString(), [this](const QString& url, const QString& scriptContents, bool isURL, bool success, const QString&status) {
        qCDebug(scriptengine) << "loadURL" << url << status << QThread::currentThread();
        if (!success) {
            scriptErrorMessage("ERROR Loading file (" + status + "):" + url);
            emit errorLoadingScript(_fileNameString);
            return;
        }

        _scriptContents = scriptContents;

        {
            static const QString DEBUG_FLAG("#debug");
            if (QRegularExpression(DEBUG_FLAG).match(scriptContents).hasMatch()) {
                qCWarning(scriptengine) << "NOTE: ScriptEngine for " << QUrl(url).fileName() << " will be launched in debug mode";
                _debuggable = true;
            }
        }
        emit scriptLoaded(url);
    }, reload, maxRetries);
}

void ScriptEngine::scriptErrorMessage(const QString& message) {
    qCCritical(scriptengine) << qPrintable(message);
    emit errorMessage(message, getFilename());
}

void ScriptEngine::scriptWarningMessage(const QString& message) {
    qCWarning(scriptengine) << qPrintable(message);
    emit warningMessage(message, getFilename());
}

void ScriptEngine::scriptInfoMessage(const QString& message) {
    qCInfo(scriptengine) << qPrintable(message);
    emit infoMessage(message, getFilename());
}

void ScriptEngine::scriptPrintedMessage(const QString& message) {
    qCDebug(scriptengine) << qPrintable(message);
    emit printedMessage(message, getFilename());
}

void ScriptEngine::clearDebugLogWindow() {
    emit clearDebugWindow();
}

// Even though we never pass AnimVariantMap directly to and from javascript, the queued invokeMethod of
// callAnimationStateHandler requires that the type be registered.
// These two are meaningful, if we ever do want to use them...
static QScriptValue animVarMapToScriptValue(QScriptEngine* engine, const AnimVariantMap& parameters) {
    QStringList unused;
    return parameters.animVariantMapToScriptValue(engine, unused, false);
}
static void animVarMapFromScriptValue(const QScriptValue& value, AnimVariantMap& parameters) {
    parameters.animVariantMapFromScriptValue(value);
}
// ... while these two are not. But none of the four are ever used.
static QScriptValue resultHandlerToScriptValue(QScriptEngine* engine,
                                               const AnimVariantResultHandler& resultHandler) {
    qCCritical(scriptengine) << "Attempt to marshall result handler to javascript";
    assert(false);
    return QScriptValue();
}
static void resultHandlerFromScriptValue(const QScriptValue& value, AnimVariantResultHandler& resultHandler) {
    qCCritical(scriptengine) << "Attempt to marshall result handler from javascript";
    assert(false);
}

// Templated qScriptRegisterMetaType fails to compile with raw pointers
using ScriptableResourceRawPtr = ScriptableResource*;

static QScriptValue scriptableResourceToScriptValue(QScriptEngine* engine,
                                                    const ScriptableResourceRawPtr& resource) {
    // The first script to encounter this resource will track its memory.
    // In this way, it will be more likely to GC.
    // This fails in the case that the resource is used across many scripts, but
    // in that case it would be too difficult to tell which one should track the memory, and
    // this serves the common case (use in a single script).
    auto data = resource->getResource();
    if (data && !resource->isInScript()) {
        resource->setInScript(true);
        QObject::connect(data.data(), SIGNAL(updateSize(qint64)), engine, SLOT(updateMemoryCost(qint64)));
    }

    auto object = engine->newQObject(
        const_cast<ScriptableResourceRawPtr>(resource),
        QScriptEngine::ScriptOwnership,
        DEFAULT_QOBJECT_WRAP_OPTIONS);
    return object;
}

static void scriptableResourceFromScriptValue(const QScriptValue& value, ScriptableResourceRawPtr& resource) {
    resource = static_cast<ScriptableResourceRawPtr>(value.toQObject());
}

static QScriptValue createScriptableResourcePrototype(ScriptEnginePointer engine) {
    auto prototype = engine->newObject();

    // Expose enum State to JS/QML via properties
    QObject* state = new QObject(engine.data());
    state->setObjectName("ResourceState");
    auto metaEnum = QMetaEnum::fromType<ScriptableResource::State>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        state->setProperty(metaEnum.key(i), metaEnum.value(i));
    }

    auto prototypeState = engine->newQObject(state, QScriptEngine::QtOwnership,
       QScriptEngine::ExcludeDeleteLater | QScriptEngine::ExcludeSlots | QScriptEngine::ExcludeSuperClassMethods);
    prototype.setProperty("State", prototypeState);

    return prototype;
}

QScriptValue avatarDataToScriptValue(QScriptEngine* engine, ScriptAvatarData* const& in) {
    return engine->newQObject(in, QScriptEngine::ScriptOwnership, DEFAULT_QOBJECT_WRAP_OPTIONS);
}

void avatarDataFromScriptValue(const QScriptValue& object, ScriptAvatarData*& out) {
    // This is not implemented because there are no slots/properties that take an AvatarSharedPointer from a script
    assert(false);
    out = nullptr;
}

void ScriptEngine::resetModuleCache(bool deleteScriptCache) {
    if (QThread::currentThread() != thread()) {
        executeOnScriptThread([=]() { resetModuleCache(deleteScriptCache); });
        return;
    }
    auto jsRequire = globalObject().property("Script").property("require");
    auto cache = jsRequire.property("cache");
    auto cacheMeta = jsRequire.data();

    if (deleteScriptCache) {
        QScriptValueIterator it(cache);
        while (it.hasNext()) {
            it.next();
            if (it.flags() & QScriptValue::SkipInEnumeration) {
                continue;
            }
            qCDebug(scriptengine) << "resetModuleCache(true) -- staging " << it.name() << " for cache reset at next require";
            cacheMeta.setProperty(it.name(), true);
        }
    }
    cache = newObject();
    if (!cacheMeta.isObject()) {
        cacheMeta = newObject();
        cacheMeta.setProperty("id", "Script.require.cacheMeta");
        cacheMeta.setProperty("type", "cacheMeta");
        jsRequire.setData(cacheMeta);
    }
    cache.setProperty("__created__", (double)QDateTime::currentMSecsSinceEpoch(), QScriptValue::SkipInEnumeration);
#if DEBUG_JS_MODULES
    cache.setProperty("__meta__", cacheMeta, READONLY_HIDDEN_PROP_FLAGS);
#endif
    jsRequire.setProperty("cache", cache, READONLY_PROP_FLAGS);
}

void ScriptEngine::init() {
    if (_isInitialized) {
        return; // only initialize once
    }

    _isInitialized = true;

    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
    entityScriptingInterface->init();

    // register various meta-types
    registerMetaTypes(this);
    registerMIDIMetaTypes(this);
    registerEventTypes(this);
    registerMenuItemProperties(this);
    registerAnimationTypes(this);
    registerAvatarTypes(this);
    registerAudioMetaTypes(this);

    qScriptRegisterMetaType(this, EntityPropertyFlagsToScriptValue, EntityPropertyFlagsFromScriptValue);
    qScriptRegisterMetaType(this, EntityItemPropertiesToScriptValue, EntityItemPropertiesFromScriptValueHonorReadOnly);
    qScriptRegisterMetaType(this, EntityItemIDtoScriptValue, EntityItemIDfromScriptValue);
    qScriptRegisterMetaType(this, RayToEntityIntersectionResultToScriptValue, RayToEntityIntersectionResultFromScriptValue);
    qScriptRegisterMetaType(this, RayToAvatarIntersectionResultToScriptValue, RayToAvatarIntersectionResultFromScriptValue);
    qScriptRegisterMetaType(this, AvatarEntityMapToScriptValue, AvatarEntityMapFromScriptValue);
    qScriptRegisterSequenceMetaType<QVector<QUuid>>(this);
    qScriptRegisterSequenceMetaType<QVector<EntityItemID>>(this);

    qScriptRegisterSequenceMetaType<QVector<glm::vec2> >(this);
    qScriptRegisterSequenceMetaType<QVector<glm::quat> >(this);
    qScriptRegisterSequenceMetaType<QVector<QString> >(this);

    QScriptValue xmlHttpRequestConstructorValue = newFunction(XMLHttpRequestClass::constructor);
    globalObject().setProperty("XMLHttpRequest", xmlHttpRequestConstructorValue);

    QScriptValue webSocketConstructorValue = newFunction(WebSocketClass::constructor);
    globalObject().setProperty("WebSocket", webSocketConstructorValue);

    globalObject().setProperty("print", newFunction(debugPrint));

    QScriptValue audioEffectOptionsConstructorValue = newFunction(AudioEffectOptions::constructor);
    globalObject().setProperty("AudioEffectOptions", audioEffectOptionsConstructorValue);

    qScriptRegisterMetaType(this, injectorToScriptValue, injectorFromScriptValue);
    qScriptRegisterMetaType(this, inputControllerToScriptValue, inputControllerFromScriptValue);
    qScriptRegisterMetaType(this, avatarDataToScriptValue, avatarDataFromScriptValue);
    qScriptRegisterMetaType(this, animationDetailsToScriptValue, animationDetailsFromScriptValue);
    qScriptRegisterMetaType(this, webSocketToScriptValue, webSocketFromScriptValue);
    qScriptRegisterMetaType(this, qWSCloseCodeToScriptValue, qWSCloseCodeFromScriptValue);
    qScriptRegisterMetaType(this, wscReadyStateToScriptValue, wscReadyStateFromScriptValue);

    // NOTE: You do not want to end up creating new instances of singletons here. They will be on the ScriptEngine thread
    // and are likely to be unusable if we "reset" the ScriptEngine by creating a new one (on a whole new thread).

    registerGlobalObject("Script", this);

    {
        // set up Script.require.resolve and Script.require.cache
        auto Script = globalObject().property("Script");
        auto require = Script.property("require");
        auto resolve = Script.property("_requireResolve");
        require.setProperty("resolve", resolve, READONLY_PROP_FLAGS);
        resetModuleCache();
    }

    registerGlobalObject("Audio", DependencyManager::get<AudioScriptingInterface>().data());

    registerGlobalObject("Midi", DependencyManager::get<Midi>().data());

    registerGlobalObject("Entities", entityScriptingInterface.data());
    registerGlobalObject("Quat", &_quatLibrary);
    registerGlobalObject("Vec3", &_vec3Library);
    registerGlobalObject("Mat4", &_mat4Library);
    registerGlobalObject("Uuid", &_uuidLibrary);
    registerGlobalObject("Messages", DependencyManager::get<MessagesClient>().data());
    registerGlobalObject("File", new FileScriptingInterface(this));
    registerGlobalObject("console", &_consoleScriptingInterface);
    registerFunction("console", "info", ConsoleScriptingInterface::info, currentContext()->argumentCount());
    registerFunction("console", "log", ConsoleScriptingInterface::log, currentContext()->argumentCount());
    registerFunction("console", "debug", ConsoleScriptingInterface::debug, currentContext()->argumentCount());
    registerFunction("console", "warn", ConsoleScriptingInterface::warn, currentContext()->argumentCount());
    registerFunction("console", "error", ConsoleScriptingInterface::error, currentContext()->argumentCount());
    registerFunction("console", "exception", ConsoleScriptingInterface::exception, currentContext()->argumentCount());
    registerFunction("console", "assert", ConsoleScriptingInterface::assertion, currentContext()->argumentCount());
    registerFunction("console", "group", ConsoleScriptingInterface::group, 1);
    registerFunction("console", "groupCollapsed", ConsoleScriptingInterface::groupCollapsed, 1);
    registerFunction("console", "groupEnd", ConsoleScriptingInterface::groupEnd, 0);

    qScriptRegisterMetaType(this, animVarMapToScriptValue, animVarMapFromScriptValue);
    qScriptRegisterMetaType(this, resultHandlerToScriptValue, resultHandlerFromScriptValue);

    // Scriptable cache access
    auto resourcePrototype = createScriptableResourcePrototype(qSharedPointerCast<ScriptEngine>(sharedFromThis()));
    globalObject().setProperty("Resource", resourcePrototype);
    setDefaultPrototype(qMetaTypeId<ScriptableResource*>(), resourcePrototype);
    qScriptRegisterMetaType(this, scriptableResourceToScriptValue, scriptableResourceFromScriptValue);

    // constants
    globalObject().setProperty("TREE_SCALE", newVariant(QVariant(TREE_SCALE)));

    registerGlobalObject("Assets", &_assetScriptingInterface);
    registerGlobalObject("Resources", DependencyManager::get<ResourceScriptingInterface>().data());

    registerGlobalObject("DebugDraw", &DebugDraw::getInstance());

    registerGlobalObject("Model", new ModelScriptingInterface(this));
    qScriptRegisterMetaType(this, meshToScriptValue, meshFromScriptValue);
    qScriptRegisterMetaType(this, meshesToScriptValue, meshesFromScriptValue);

    registerGlobalObject("UserActivityLogger", DependencyManager::get<UserActivityLoggerScriptingInterface>().data());
}

void ScriptEngine::registerValue(const QString& valueName, QScriptValue value) {
    if (QThread::currentThread() != thread()) {
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngine::registerValue() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "]";
#endif
        QMetaObject::invokeMethod(this, "registerValue",
                                  Q_ARG(const QString&, valueName),
                                  Q_ARG(QScriptValue, value));
        return;
    }

    QStringList pathToValue = valueName.split(".");
    int partsToGo = pathToValue.length();
    QScriptValue partObject = globalObject();

    for (const auto& pathPart : pathToValue) {
        partsToGo--;
        if (!partObject.property(pathPart).isValid()) {
            if (partsToGo > 0) {
                //QObject *object = new QObject;
                QScriptValue partValue = newArray(); //newQObject(object, QScriptEngine::ScriptOwnership);
                partObject.setProperty(pathPart, partValue);
            } else {
                partObject.setProperty(pathPart, value);
            }
        }
        partObject = partObject.property(pathPart);
    }
}

void ScriptEngine::registerGlobalObject(const QString& name, QObject* object) {
    if (QThread::currentThread() != thread()) {
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngine::registerGlobalObject() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "]  name:" << name;
#endif
        QMetaObject::invokeMethod(this, "registerGlobalObject",
                                  Q_ARG(const QString&, name),
                                  Q_ARG(QObject*, object));
        return;
    }
#ifdef THREAD_DEBUGGING
    qCDebug(scriptengine) << "ScriptEngine::registerGlobalObject() called on thread [" << QThread::currentThread() << "] name:" << name;
#endif

    if (!globalObject().property(name).isValid()) {
        if (object) {
            QScriptValue value = newQObject(object, QScriptEngine::QtOwnership, DEFAULT_QOBJECT_WRAP_OPTIONS);
            globalObject().setProperty(name, value);
        } else {
            globalObject().setProperty(name, QScriptValue());
        }
    }
}

void ScriptEngine::registerFunction(const QString& name, QScriptEngine::FunctionSignature functionSignature, int numArguments) {
    if (QThread::currentThread() != thread()) {
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngine::registerFunction() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "] name:" << name;
#endif
        QMetaObject::invokeMethod(this, "registerFunction",
                                  Q_ARG(const QString&, name),
                                  Q_ARG(QScriptEngine::FunctionSignature, functionSignature),
                                  Q_ARG(int, numArguments));
        return;
    }
#ifdef THREAD_DEBUGGING
    qCDebug(scriptengine) << "ScriptEngine::registerFunction() called on thread [" << QThread::currentThread() << "] name:" << name;
#endif

    QScriptValue scriptFun = newFunction(functionSignature, numArguments);
    globalObject().setProperty(name, scriptFun);
}

void ScriptEngine::registerFunction(const QString& parent, const QString& name, QScriptEngine::FunctionSignature functionSignature, int numArguments) {
    if (QThread::currentThread() != thread()) {
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngine::registerFunction() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "] parent:" << parent << "name:" << name;
#endif
        QMetaObject::invokeMethod(this, "registerFunction",
                                  Q_ARG(const QString&, name),
                                  Q_ARG(QScriptEngine::FunctionSignature, functionSignature),
                                  Q_ARG(int, numArguments));
        return;
    }
#ifdef THREAD_DEBUGGING
    qCDebug(scriptengine) << "ScriptEngine::registerFunction() called on thread [" << QThread::currentThread() << "] parent:" << parent << "name:" << name;
#endif

    QScriptValue object = globalObject().property(parent);
    if (object.isValid()) {
        QScriptValue scriptFun = newFunction(functionSignature, numArguments);
        object.setProperty(name, scriptFun);
    }
}

void ScriptEngine::registerGetterSetter(const QString& name, QScriptEngine::FunctionSignature getter,
                                        QScriptEngine::FunctionSignature setter, const QString& parent) {
    if (QThread::currentThread() != thread()) {
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngine::registerGetterSetter() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "] "
            " name:" << name << "parent:" << parent;
#endif
        QMetaObject::invokeMethod(this, "registerGetterSetter",
                                  Q_ARG(const QString&, name),
                                  Q_ARG(QScriptEngine::FunctionSignature, getter),
                                  Q_ARG(QScriptEngine::FunctionSignature, setter),
                                  Q_ARG(const QString&, parent));
        return;
    }
#ifdef THREAD_DEBUGGING
    qCDebug(scriptengine) << "ScriptEngine::registerGetterSetter() called on thread [" << QThread::currentThread() << "] name:" << name << "parent:" << parent;
#endif

    QScriptValue setterFunction = newFunction(setter, 1);
    QScriptValue getterFunction = newFunction(getter);

    if (!parent.isNull() && !parent.isEmpty()) {
        QScriptValue object = globalObject().property(parent);
        if (object.isValid()) {
            object.setProperty(name, setterFunction, QScriptValue::PropertySetter);
            object.setProperty(name, getterFunction, QScriptValue::PropertyGetter);
        }
    } else {
        globalObject().setProperty(name, setterFunction, QScriptValue::PropertySetter);
        globalObject().setProperty(name, getterFunction, QScriptValue::PropertyGetter);
    }
}

// Unregister the handlers for this eventName and entityID.
void ScriptEngine::removeEventHandler(const EntityItemID& entityID, const QString& eventName, QScriptValue handler) {
    if (QThread::currentThread() != thread()) {
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngine::removeEventHandler() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "] "
            "entityID:" << entityID << " eventName:" << eventName;
#endif
        QMetaObject::invokeMethod(this, "removeEventHandler",
                                  Q_ARG(const EntityItemID&, entityID),
                                  Q_ARG(const QString&, eventName),
                                  Q_ARG(QScriptValue, handler));
        return;
    }
#ifdef THREAD_DEBUGGING
    qCDebug(scriptengine) << "ScriptEngine::removeEventHandler() called on thread [" << QThread::currentThread() << "] entityID:" << entityID << " eventName : " << eventName;
#endif

    if (!_registeredHandlers.contains(entityID)) {
        return;
    }
    RegisteredEventHandlers& handlersOnEntity = _registeredHandlers[entityID];
    CallbackList& handlersForEvent = handlersOnEntity[eventName];
    // QScriptValue does not have operator==(), so we can't use QList::removeOne and friends. So iterate.
    for (int i = 0; i < handlersForEvent.count(); ++i) {
        if (handlersForEvent[i].function.equals(handler)) {
            handlersForEvent.removeAt(i);
            return; // Design choice: since comparison is relatively expensive, just remove the first matching handler.
        }
    }
}
// Register the handler.
void ScriptEngine::addEventHandler(const EntityItemID& entityID, const QString& eventName, QScriptValue handler) {
    if (QThread::currentThread() != thread()) {
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngine::addEventHandler() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "] "
        "entityID:" << entityID << " eventName:" << eventName;
#endif

        QMetaObject::invokeMethod(this, "addEventHandler",
                                  Q_ARG(const EntityItemID&, entityID),
                                  Q_ARG(const QString&, eventName),
                                  Q_ARG(QScriptValue, handler));
        return;
    }
#ifdef THREAD_DEBUGGING
    qCDebug(scriptengine) << "ScriptEngine::addEventHandler() called on thread [" << QThread::currentThread() << "] entityID:" << entityID << " eventName : " << eventName;
#endif

    if (_registeredHandlers.count() == 0) { // First time any per-entity handler has been added in this script...
        // Connect up ALL the handlers to the global entities object's signals.
        // (We could go signal by signal, or even handler by handler, but I don't think the efficiency is worth the complexity.)
        auto entities = DependencyManager::get<EntityScriptingInterface>();
        // Bug? These handlers are deleted when entityID is deleted, which is nice.
        // But if they are created by an entity script on a different entity, should they also be deleted when the entity script unloads?
        // E.g., suppose a bow has an entity script that causes arrows to be created with a potential lifetime greater than the bow,
        // and that the entity script adds (e.g., collision) handlers to the arrows. Should those handlers fire if the bow is unloaded?
        // Also, what about when the entity script is REloaded?
        // For now, we are leaving them around. Changing that would require some non-trivial digging around to find the
        // handlers that were added while a given currentEntityIdentifier was in place. I don't think this is dangerous. Just perhaps unexpected. -HRS
        connect(entities.data(), &EntityScriptingInterface::deletingEntity, this, [this](const EntityItemID& entityID) {
            _registeredHandlers.remove(entityID);
        });

        // Two common cases of event handler, differing only in argument signature.
        using SingleEntityHandler = std::function<void(const EntityItemID&)>;
        auto makeSingleEntityHandler = [this](QString eventName) -> SingleEntityHandler {
            return [this, eventName](const EntityItemID& entityItemID) {
                forwardHandlerCall(entityItemID, eventName, { entityItemID.toScriptValue(this) });
            };
        };

        using PointerHandler = std::function<void(const EntityItemID&, const PointerEvent&)>;
        auto makePointerHandler = [this](QString eventName) -> PointerHandler {
            return [this, eventName](const EntityItemID& entityItemID, const PointerEvent& event) {
                forwardHandlerCall(entityItemID, eventName, { entityItemID.toScriptValue(this), event.toScriptValue(this) });
            };
        };

        using CollisionHandler = std::function<void(const EntityItemID&, const EntityItemID&, const Collision&)>;
        auto makeCollisionHandler = [this](QString eventName) -> CollisionHandler {
            return [this, eventName](const EntityItemID& idA, const EntityItemID& idB, const Collision& collision) {
                forwardHandlerCall(idA, eventName, { idA.toScriptValue(this), idB.toScriptValue(this),
                    collisionToScriptValue(this, collision) });
            };
        };

        connect(entities.data(), &EntityScriptingInterface::enterEntity, this, makeSingleEntityHandler("enterEntity"));
        connect(entities.data(), &EntityScriptingInterface::leaveEntity, this, makeSingleEntityHandler("leaveEntity"));

        connect(entities.data(), &EntityScriptingInterface::mousePressOnEntity, this, makePointerHandler("mousePressOnEntity"));
        connect(entities.data(), &EntityScriptingInterface::mouseMoveOnEntity, this, makePointerHandler("mouseMoveOnEntity"));
        connect(entities.data(), &EntityScriptingInterface::mouseReleaseOnEntity, this, makePointerHandler("mouseReleaseOnEntity"));

        connect(entities.data(), &EntityScriptingInterface::clickDownOnEntity, this, makePointerHandler("clickDownOnEntity"));
        connect(entities.data(), &EntityScriptingInterface::holdingClickOnEntity, this, makePointerHandler("holdingClickOnEntity"));
        connect(entities.data(), &EntityScriptingInterface::clickReleaseOnEntity, this, makePointerHandler("clickReleaseOnEntity"));

        connect(entities.data(), &EntityScriptingInterface::hoverEnterEntity, this, makePointerHandler("hoverEnterEntity"));
        connect(entities.data(), &EntityScriptingInterface::hoverOverEntity, this, makePointerHandler("hoverOverEntity"));
        connect(entities.data(), &EntityScriptingInterface::hoverLeaveEntity, this, makePointerHandler("hoverLeaveEntity"));

        connect(entities.data(), &EntityScriptingInterface::collisionWithEntity, this, makeCollisionHandler("collisionWithEntity"));
    }
    if (!_registeredHandlers.contains(entityID)) {
        _registeredHandlers[entityID] = RegisteredEventHandlers();
    }
    CallbackList& handlersForEvent = _registeredHandlers[entityID][eventName];
    CallbackData handlerData = { handler, currentEntityIdentifier, currentSandboxURL };
    handlersForEvent << handlerData; // Note that the same handler can be added many times. See removeEntityEventHandler().
}

// this is not redundant -- the version in BaseScriptEngine is specifically not Q_INVOKABLE
QScriptValue ScriptEngine::evaluateInClosure(const QScriptValue& closure, const QScriptProgram& program) {
    return BaseScriptEngine::evaluateInClosure(closure, program);
}

QScriptValue ScriptEngine::evaluate(const QString& sourceCode, const QString& fileName, int lineNumber) {
    if (DependencyManager::get<ScriptEngines>()->isStopped()) {
        return QScriptValue(); // bail early
    }

    if (QThread::currentThread() != thread()) {
        QScriptValue result;
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngine::evaluate() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "] "
            "sourceCode:" << sourceCode << " fileName:" << fileName << "lineNumber:" << lineNumber;
#endif
        BLOCKING_INVOKE_METHOD(this, "evaluate",
                                  Q_RETURN_ARG(QScriptValue, result),
                                  Q_ARG(const QString&, sourceCode),
                                  Q_ARG(const QString&, fileName),
                                  Q_ARG(int, lineNumber));
        return result;
    }

    // Check syntax
    auto syntaxError = lintScript(sourceCode, fileName);
    if (syntaxError.isError()) {
        if (!isEvaluating()) {
            syntaxError.setProperty("detail", "evaluate");
        }
        raiseException(syntaxError);
        maybeEmitUncaughtException("lint");
        return syntaxError;
    }
    QScriptProgram program { sourceCode, fileName, lineNumber };
    if (program.isNull()) {
        // can this happen?
        auto err = makeError("could not create QScriptProgram for " + fileName);
        raiseException(err);
        maybeEmitUncaughtException("compile");
        return err;
    }

    QScriptValue result;
    {
        result = BaseScriptEngine::evaluate(program);
        maybeEmitUncaughtException("evaluate");
    }
    return result;
}

void ScriptEngine::run() {
    auto filenameParts = _fileNameString.split("/");
    auto name = filenameParts.size() > 0 ? filenameParts[filenameParts.size() - 1] : "unknown";
    PROFILE_SET_THREAD_NAME("Script: " + name);

    if (DependencyManager::get<ScriptEngines>()->isStopped()) {
        return; // bail early - avoid setting state in init(), as evaluate() will bail too
    }

    scriptInfoMessage("Script Engine starting:" + getFilename());

    if (!_isInitialized) {
        init();
    }

    _isRunning = true;
    emit runningStateChanged();

    {
        PROFILE_RANGE(script, _fileNameString);
        evaluate(_scriptContents, _fileNameString);
        maybeEmitUncaughtException(__FUNCTION__);
    }
#ifdef _WIN32
    // VS13 does not sleep_until unless it uses the system_clock, see:
    // https://www.reddit.com/r/cpp_questions/comments/3o71ic/sleep_until_not_working_with_a_time_pointsteady/
    using clock = std::chrono::system_clock;
#else
    using clock = std::chrono::high_resolution_clock;
#endif

    clock::time_point startTime = clock::now();
    int thisFrame = 0;

    auto nodeList = DependencyManager::get<NodeList>();
    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();

    _lastUpdate = usecTimestampNow();

    std::chrono::microseconds totalUpdates(0);

    // TODO: Integrate this with signals/slots instead of reimplementing throttling for ScriptEngine
    while (!_isFinished) {
        auto beforeSleep = clock::now();

        // Throttle to SCRIPT_FPS
        // We'd like to try to keep the script at a solid SCRIPT_FPS update rate. And so we will
        // calculate a sleepUntil to be the time from our start time until the original target
        // sleepUntil for this frame. This approach will allow us to "catch up" in the event
        // that some of our script udpates/frames take a little bit longer than the target average
        // to execute.
        // NOTE: if we go to variable SCRIPT_FPS, then we will need to reconsider this approach
        const std::chrono::microseconds TARGET_SCRIPT_FRAME_DURATION(USECS_PER_SECOND / SCRIPT_FPS + 1);
        clock::time_point targetSleepUntil(startTime + (thisFrame++ * TARGET_SCRIPT_FRAME_DURATION));

        // However, if our sleepUntil is not at least our average update and timer execution time
        // into the future it means our script is taking too long in its updates, and we want to
        // punish the script a little bit. So we will force the sleepUntil to be at least our
        // averageUpdate + averageTimerPerFrame time into the future.
        auto averageUpdate = totalUpdates / thisFrame;
        auto averageTimerPerFrame = _totalTimerExecution / thisFrame;
        auto averageTimerAndUpdate = averageUpdate + averageTimerPerFrame;
        auto sleepUntil = std::max(targetSleepUntil, beforeSleep + averageTimerAndUpdate);

        // We don't want to actually sleep for too long, because it causes our scripts to hang
        // on shutdown and stop... so we want to loop and sleep until we've spent our time in
        // purgatory, constantly checking to see if our script was asked to end
        bool processedEvents = false;
        if (!_isFinished) {
            PROFILE_RANGE(script, "processEvents-sleep");
            std::chrono::milliseconds sleepFor =
                std::chrono::duration_cast<std::chrono::milliseconds>(sleepUntil - clock::now());
            if (sleepFor > std::chrono::milliseconds(0)) {
                QEventLoop loop;
                QTimer timer;
                timer.setSingleShot(true);
                connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
                timer.start(sleepFor.count());
                loop.exec();
            } else {
                QCoreApplication::processEvents();
            }
            processedEvents = true;
        }

        PROFILE_RANGE(script, "ScriptMainLoop");

#ifdef SCRIPT_DELAY_DEBUG
        {
            auto actuallySleptUntil = clock::now();
            uint64_t seconds = std::chrono::duration_cast<std::chrono::seconds>(actuallySleptUntil - startTime).count();
            if (seconds > 0) { // avoid division by zero and time travel
                uint64_t fps = thisFrame / seconds;
                // Overreporting artificially reduces the reported rate
                if (thisFrame % SCRIPT_FPS == 0) {
                    qCDebug(scriptengine) <<
                        "Frame:" << thisFrame <<
                        "Slept (us):" << std::chrono::duration_cast<std::chrono::microseconds>(actuallySleptUntil - beforeSleep).count() <<
                        "Avg Updates (us):" << averageUpdate.count() <<
                        "FPS:" << fps;
                }
            }
        }
#endif
        if (_isFinished) {
            break;
        }

        // Only call this if we didn't processEvents as part of waiting for next frame
        if (!processedEvents) {
            PROFILE_RANGE(script, "processEvents");
            QCoreApplication::processEvents();
        }

        if (_isFinished) {
            break;
        }

        if (!_isFinished && entityScriptingInterface->getEntityPacketSender()->serversExist()) {
            // release the queue of edit entity messages.
            entityScriptingInterface->getEntityPacketSender()->releaseQueuedMessages();

            // since we're in non-threaded mode, call process so that the packets are sent
            if (!entityScriptingInterface->getEntityPacketSender()->isThreaded()) {
                entityScriptingInterface->getEntityPacketSender()->process();
            }
        }

        qint64 now = usecTimestampNow();

        // we check for 'now' in the past in case people set their clock back
        if (_emitScriptUpdates() && _lastUpdate < now) {
            float deltaTime = (float) (now - _lastUpdate) / (float) USECS_PER_SECOND;
            if (!_isFinished) {
                auto preUpdate = clock::now();
                {
                    PROFILE_RANGE(script, "ScriptUpdate");
                    emit update(deltaTime);
                }
                auto postUpdate = clock::now();
                auto elapsed = (postUpdate - preUpdate);
                totalUpdates += std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
            }
        }
        _lastUpdate = now;

        // only clear exceptions if we are not in the middle of evaluating
        if (!isEvaluating() && hasUncaughtException()) {
            qCWarning(scriptengine) << __FUNCTION__ << "---------- UNCAUGHT EXCEPTION --------";
            qCWarning(scriptengine) << "runInThread" << uncaughtException().toString();
            emit unhandledException(cloneUncaughtException(__FUNCTION__));
            clearExceptions();
        }
    }
    scriptInfoMessage("Script Engine stopping:" + getFilename());

    stopAllTimers(); // make sure all our timers are stopped if the script is ending
    emit scriptEnding();

    if (entityScriptingInterface->getEntityPacketSender()->serversExist()) {
        // release the queue of edit entity messages.
        entityScriptingInterface->getEntityPacketSender()->releaseQueuedMessages();

        // since we're in non-threaded mode, call process so that the packets are sent
        if (!entityScriptingInterface->getEntityPacketSender()->isThreaded()) {
            // wait here till the edit packet sender is completely done sending
            while (entityScriptingInterface->getEntityPacketSender()->hasPacketsToSend()) {
                entityScriptingInterface->getEntityPacketSender()->process();
                QCoreApplication::processEvents();
            }
        } else {
            // FIXME - do we need to have a similar "wait here" loop for non-threaded packet senders?
        }
    }

    emit finished(_fileNameString, qSharedPointerCast<ScriptEngine>(sharedFromThis()));

    _isRunning = false;
    emit runningStateChanged();
    emit doneRunning();
}

// NOTE: This is private because it must be called on the same thread that created the timers, which is why
// we want to only call it in our own run "shutdown" processing.
void ScriptEngine::stopAllTimers() {
    QMutableHashIterator<QTimer*, CallbackData> i(_timerFunctionMap);
    int j {0};
    while (i.hasNext()) {
        i.next();
        QTimer* timer = i.key();
        qCDebug(scriptengine) << getFilename() << "stopAllTimers[" << j++ << "]";
        stopTimer(timer);
    }
}

void ScriptEngine::stopAllTimersForEntityScript(const EntityItemID& entityID) {
     // We could maintain a separate map of entityID => QTimer, but someone will have to prove to me that it's worth the complexity. -HRS
    QVector<QTimer*> toDelete;
    QMutableHashIterator<QTimer*, CallbackData> i(_timerFunctionMap);
    while (i.hasNext()) {
        i.next();
        if (i.value().definingEntityIdentifier != entityID) {
            continue;
        }
        QTimer* timer = i.key();
        toDelete << timer; // don't delete while we're iterating. save it.
    }
    for (auto timer:toDelete) { // now reap 'em
        stopTimer(timer);
    }

}

void ScriptEngine::stop(bool marshal) {
    _isStopping = true; // this can be done on any thread

    if (marshal) {
        QMetaObject::invokeMethod(this, "stop");
        return;
    }
    if (!_isFinished) {
        _isFinished = true;
        emit runningStateChanged();
    }
}

// Other threads can invoke this through invokeMethod, which causes the callback to be asynchronously executed in this script's thread.
void ScriptEngine::callAnimationStateHandler(QScriptValue callback, AnimVariantMap parameters, QStringList names, bool useNames, AnimVariantResultHandler resultHandler) {
    if (QThread::currentThread() != thread()) {
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngine::callAnimationStateHandler() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "]  name:" << name;
#endif
        QMetaObject::invokeMethod(this, "callAnimationStateHandler",
                                  Q_ARG(QScriptValue, callback),
                                  Q_ARG(AnimVariantMap, parameters),
                                  Q_ARG(QStringList, names),
                                  Q_ARG(bool, useNames),
                                  Q_ARG(AnimVariantResultHandler, resultHandler));
        return;
    }
    QScriptValue javascriptParameters = parameters.animVariantMapToScriptValue(this, names, useNames);
    QScriptValueList callingArguments;
    callingArguments << javascriptParameters;
    assert(currentEntityIdentifier.isInvalidID()); // No animation state handlers from entity scripts.
    QScriptValue result = callback.call(QScriptValue(), callingArguments);

    // validate result from callback function.
    if (result.isValid() && result.isObject()) {
        resultHandler(result);
    } else {
        qCWarning(scriptengine) << "ScriptEngine::callAnimationStateHandler invalid return argument from callback, expected an object";
    }
}

void ScriptEngine::updateMemoryCost(const qint64& deltaSize) {
    if (deltaSize > 0) {
        reportAdditionalMemoryCost(deltaSize);
    }
}

void ScriptEngine::timerFired() {
    {
        auto engine = DependencyManager::get<ScriptEngines>();
        if (!engine || engine->isStopped()) {
            scriptWarningMessage("Script.timerFired() while shutting down is ignored... parent script:" + getFilename());
            return; // bail early
        }
    }

    QTimer* callingTimer = reinterpret_cast<QTimer*>(sender());
    CallbackData timerData = _timerFunctionMap.value(callingTimer);

    if (!callingTimer->isActive()) {
        // this timer is done, we can kill it
        _timerFunctionMap.remove(callingTimer);
        delete callingTimer;
    }

    // call the associated JS function, if it exists
    if (timerData.function.isValid()) {
        PROFILE_RANGE(script, __FUNCTION__);
        auto preTimer = p_high_resolution_clock::now();
        callWithEnvironment(timerData.definingEntityIdentifier, timerData.definingSandboxURL, timerData.function, timerData.function, QScriptValueList());
        auto postTimer = p_high_resolution_clock::now();
        auto elapsed = (postTimer - preTimer);
        _totalTimerExecution += std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
    } else {
        qCWarning(scriptengine) << "timerFired -- invalid function" << timerData.function.toVariant().toString();
    }
}

QObject* ScriptEngine::setupTimerWithInterval(const QScriptValue& function, int intervalMS, bool isSingleShot) {
    // create the timer, add it to the map, and start it
    QTimer* newTimer = new QTimer(this);
    newTimer->setSingleShot(isSingleShot);

    // The default timer type is not very accurate below about 200ms http://doc.qt.io/qt-5/qt.html#TimerType-enum
    static const int MIN_TIMEOUT_FOR_COARSE_TIMER = 200;
    if (intervalMS < MIN_TIMEOUT_FOR_COARSE_TIMER) {
        newTimer->setTimerType(Qt::PreciseTimer);
    }

    connect(newTimer, &QTimer::timeout, this, &ScriptEngine::timerFired);

    // make sure the timer stops when the script does
    connect(this, &ScriptEngine::scriptEnding, newTimer, &QTimer::stop);


    CallbackData timerData = { function, currentEntityIdentifier, currentSandboxURL };
    _timerFunctionMap.insert(newTimer, timerData);

    newTimer->start(intervalMS);
    return newTimer;
}

QObject* ScriptEngine::setInterval(const QScriptValue& function, int intervalMS) {
    if (DependencyManager::get<ScriptEngines>()->isStopped()) {
        scriptWarningMessage("Script.setInterval() while shutting down is ignored... parent script:" + getFilename());
        return NULL; // bail early
    }

    return setupTimerWithInterval(function, intervalMS, false);
}

QObject* ScriptEngine::setTimeout(const QScriptValue& function, int timeoutMS) {
    if (DependencyManager::get<ScriptEngines>()->isStopped()) {
        scriptWarningMessage("Script.setTimeout() while shutting down is ignored... parent script:" + getFilename());
        return NULL; // bail early
    }

    return setupTimerWithInterval(function, timeoutMS, true);
}

void ScriptEngine::stopTimer(QTimer *timer) {
    if (_timerFunctionMap.contains(timer)) {
        timer->stop();
        _timerFunctionMap.remove(timer);
        delete timer;
    } else {
        qCDebug(scriptengine) << "stopTimer -- not in _timerFunctionMap" << timer;
    }
}

QUrl ScriptEngine::resolvePath(const QString& include) const {
    QUrl url(include);
    // first lets check to see if it's already a full URL -- or a Windows path like "c:/"
    if (include.startsWith("/") || url.scheme().length() == 1) {
        url = QUrl::fromLocalFile(include);
    }
    if (!url.isRelative()) {
        return expandScriptUrl(url);
    }

    // we apparently weren't a fully qualified url, so, let's assume we're relative
    // to the first absolute URL in the JS scope chain
    QUrl parentURL;
    auto context = currentContext();
    do {
        QScriptContextInfo contextInfo { context };
        parentURL = QUrl(contextInfo.fileName());
        context = context->parentContext();
    } while (parentURL.isRelative() && context);

    if (parentURL.isRelative()) {
        // fallback to the "include" parent (if defined, this will already be absolute)
        parentURL = QUrl(_parentURL);
    }

    if (parentURL.isRelative()) {
        // fallback to the original script engine URL
        parentURL = QUrl(_fileNameString);

        // if still relative and path-like, then this is probably a local file...
        if (parentURL.isRelative() && url.path().contains("/")) {
            parentURL = QUrl::fromLocalFile(_fileNameString);
        }
    }

    // at this point we should have a legitimate fully qualified URL for our parent
    url = expandScriptUrl(parentURL.resolved(url));
    return url;
}

QUrl ScriptEngine::resourcesPath() const {
    return QUrl::fromLocalFile(PathUtils::resourcesPath());
}

void ScriptEngine::print(const QString& message) {
    emit printedMessage(message, getFilename());
}


void ScriptEngine::beginProfileRange(const QString& label) const {
    PROFILE_SYNC_BEGIN(script, label.toStdString().c_str(), label.toStdString().c_str());
}

void ScriptEngine::endProfileRange(const QString& label) const {
    PROFILE_SYNC_END(script, label.toStdString().c_str(), label.toStdString().c_str());
}

// Script.require.resolve -- like resolvePath, but performs more validation and throws exceptions on invalid module identifiers (for consistency with Node.js)
QString ScriptEngine::_requireResolve(const QString& moduleId, const QString& relativeTo) {
    if (!IS_THREADSAFE_INVOCATION(thread(), __FUNCTION__)) {
        return QString();
    }
    QUrl defaultScriptsLoc = PathUtils::defaultScriptsLocation();
    QUrl url(moduleId);

    auto displayId = moduleId;
    if (displayId.length() > MAX_DEBUG_VALUE_LENGTH) {
        displayId = displayId.mid(0, MAX_DEBUG_VALUE_LENGTH) + "...";
    }
    auto message = QString("Cannot find module '%1' (%2)").arg(displayId);

    auto throwResolveError = [&](const QScriptValue& error) -> QString {
        raiseException(error);
        maybeEmitUncaughtException("require.resolve");
        return QString();
    };

    // de-fuzz the input a little by restricting to rational sizes
    auto idLength = url.toString().length();
    if (idLength < 1 || idLength > MAX_MODULE_ID_LENGTH) {
        auto details = QString("rejecting invalid module id size (%1 chars [1,%2])")
            .arg(idLength).arg(MAX_MODULE_ID_LENGTH);
        return throwResolveError(makeError(message.arg(details), "RangeError"));
    }

    // this regex matches: absolute, dotted or path-like URLs
    // (ie: the kind of stuff ScriptEngine::resolvePath already handles)
    QRegularExpression qualified ("^\\w+:|^/|^[.]{1,2}(/|$)");

    // this is for module.require (which is a bound version of require that's always relative to the module path)
    if (!relativeTo.isEmpty()) {
        url = QUrl(relativeTo).resolved(moduleId);
        url = resolvePath(url.toString());
    } else if (qualified.match(moduleId).hasMatch()) {
        url = resolvePath(moduleId);
    } else {
        // check if the moduleId refers to a "system" module
        QString systemPath = defaultScriptsLoc.path();
        QString systemModulePath = QString("%1/modules/%2.js").arg(systemPath).arg(moduleId);
        url = defaultScriptsLoc;
        url.setPath(systemModulePath);
        if (!QFileInfo(url.toLocalFile()).isFile()) {
            if (!moduleId.contains("./")) {
                // the user might be trying to refer to a relative file without anchoring it
                // let's do them a favor and test for that case -- offering specific advice if detected
                auto unanchoredUrl = resolvePath("./" + moduleId);
                if (QFileInfo(unanchoredUrl.toLocalFile()).isFile()) {
                    auto msg = QString("relative module ids must be anchored; use './%1' instead")
                        .arg(moduleId);
                    return throwResolveError(makeError(message.arg(msg)));
                }
            }
            return throwResolveError(makeError(message.arg("system module not found")));
        }
    }

    if (url.isRelative()) {
        return throwResolveError(makeError(message.arg("could not resolve module id")));
    }

    // if it looks like a local file, verify that it's an allowed path and really a file
    if (url.isLocalFile()) {
        QFileInfo file(url.toLocalFile());
        QUrl canonical = url;
        if (file.exists()) {
            canonical.setPath(file.canonicalFilePath());
        }

        bool disallowOutsideFiles = !PathUtils::defaultScriptsLocation().isParentOf(canonical) && !currentSandboxURL.isLocalFile();
        if (disallowOutsideFiles && !PathUtils::isDescendantOf(canonical, currentSandboxURL)) {
            return throwResolveError(makeError(message.arg(
                QString("path '%1' outside of origin script '%2' '%3'")
                    .arg(PathUtils::stripFilename(url))
                    .arg(PathUtils::stripFilename(currentSandboxURL))
                    .arg(canonical.toString())
            )));
        }
        if (!file.exists()) {
            return throwResolveError(makeError(message.arg("path does not exist: " + url.toLocalFile())));
        }
        if (!file.isFile()) {
            return throwResolveError(makeError(message.arg("path is not a file: " + url.toLocalFile())));
        }
    }

    maybeEmitUncaughtException(__FUNCTION__);
    return url.toString();
}

// retrieves the current parent module from the JS scope chain
QScriptValue ScriptEngine::currentModule() {
    if (!IS_THREADSAFE_INVOCATION(thread(), __FUNCTION__)) {
        return unboundNullValue();
    }
    auto jsRequire = globalObject().property("Script").property("require");
    auto cache = jsRequire.property("cache");
    auto candidate = QScriptValue();
    for (auto c = currentContext(); c && !candidate.isObject(); c = c->parentContext()) {
        QScriptContextInfo contextInfo { c };
        candidate = cache.property(contextInfo.fileName());
    }
    if (!candidate.isObject()) {
        return QScriptValue();
    }
    return candidate;
}

// replaces or adds "module" to "parent.children[]" array
// (for consistency with Node.js and userscript cache invalidation without "cache busters")
bool ScriptEngine::registerModuleWithParent(const QScriptValue& module, const QScriptValue& parent) {
    auto children = parent.property("children");
    if (children.isArray()) {
        auto key = module.property("id");
        auto length = children.property("length").toInt32();
        for (int i = 0; i < length; i++) {
            if (children.property(i).property("id").strictlyEquals(key)) {
                qCDebug(scriptengine_module) << key.toString() << " updating parent.children[" << i << "] = module";
                children.setProperty(i, module);
                return true;
            }
        }
        qCDebug(scriptengine_module) << key.toString() << " appending parent.children[" << length << "] = module";
        children.setProperty(length, module);
        return true;
    } else if (parent.isValid()) {
        qCDebug(scriptengine_module) << "registerModuleWithParent -- unrecognized parent" << parent.toVariant().toString();
    }
    return false;
}

// creates a new JS "module" Object with default metadata properties
QScriptValue ScriptEngine::newModule(const QString& modulePath, const QScriptValue& parent) {
    auto closure = newObject();
    auto exports = newObject();
    auto module = newObject();
    qCDebug(scriptengine_module) << "newModule" << modulePath << parent.property("filename").toString();

    closure.setProperty("module", module, READONLY_PROP_FLAGS);

    // note: this becomes the "exports" free variable, so should not be set read only
    closure.setProperty("exports", exports);

    // make the closure available to module instantiation
    module.setProperty("__closure__", closure, READONLY_HIDDEN_PROP_FLAGS);

    // for consistency with Node.js Module
    module.setProperty("id", modulePath, READONLY_PROP_FLAGS);
    module.setProperty("filename", modulePath, READONLY_PROP_FLAGS);
    module.setProperty("exports", exports); // not readonly
    module.setProperty("loaded", false, READONLY_PROP_FLAGS);
    module.setProperty("parent", parent, READONLY_PROP_FLAGS);
    module.setProperty("children", newArray(), READONLY_PROP_FLAGS);

    // module.require is a bound version of require that always resolves relative to that module's path
    auto boundRequire = QScriptEngine::evaluate("(function(id) { return Script.require(Script.require.resolve(id, this.filename)); })", "(boundRequire)");
    module.setProperty("require", boundRequire, READONLY_PROP_FLAGS);

    return module;
}

// synchronously fetch a module's source code using BatchLoader
QVariantMap ScriptEngine::fetchModuleSource(const QString& modulePath, const bool forceDownload) {
    using UrlMap = QMap<QUrl, QString>;
    auto scriptCache = DependencyManager::get<ScriptCache>();
    QVariantMap req;
    qCDebug(scriptengine_module) << "require.fetchModuleSource: " << QUrl(modulePath).fileName() << QThread::currentThread();

    auto onload = [=, &req](const UrlMap& data, const UrlMap& _status) {
        auto url = modulePath;
        auto status = _status[url];
        auto contents = data[url];
        qCDebug(scriptengine_module) << "require.fetchModuleSource.onload: " << QUrl(url).fileName() << status << QThread::currentThread();
        if (isStopping()) {
            req["status"] = "Stopped";
            req["success"] = false;
        } else {
            req["url"] = url;
            req["status"] = status;
            req["success"] = ScriptCache::isSuccessStatus(status);
            req["contents"] = contents;
        }
    };

    if (forceDownload) {
        qCDebug(scriptengine_module) << "require.requestScript -- clearing cache for" << modulePath;
        scriptCache->deleteScript(modulePath);
    }
    BatchLoader* loader = new BatchLoader(QList<QUrl>({ modulePath }));
    connect(loader, &BatchLoader::finished, this, onload);
    connect(this, &QObject::destroyed, loader, &QObject::deleteLater);
    // fail faster? (since require() blocks the engine thread while resolving dependencies)
    const int MAX_RETRIES = 1;

    loader->start(MAX_RETRIES);

    if (!loader->isFinished()) {
        QTimer monitor;
        QEventLoop loop;
        QObject::connect(loader, &BatchLoader::finished, this, [this, &monitor, &loop]{
            monitor.stop();
            loop.quit();
        });

        // this helps detect the case where stop() is invoked during the download
        //  but not seen in time to abort processing in onload()...
        connect(&monitor, &QTimer::timeout, this, [this, &loop, &loader]{
            if (isStopping()) {
                loop.exit(-1);
            }
        });
        monitor.start(500);
        loop.exec();
    }
    loader->deleteLater();
    return req;
}

// evaluate a pending module object using the fetched source code
QScriptValue ScriptEngine::instantiateModule(const QScriptValue& module, const QString& sourceCode) {
    QScriptValue result;
    auto modulePath = module.property("filename").toString();
    auto closure = module.property("__closure__");

    qCDebug(scriptengine_module) << QString("require.instantiateModule: %1 / %2 bytes")
        .arg(QUrl(modulePath).fileName()).arg(sourceCode.length());

    if (module.property("content-type").toString() == "application/json") {
        qCDebug(scriptengine_module) << "... parsing as JSON";
        closure.setProperty("__json", sourceCode);
        result = evaluateInClosure(closure, { "module.exports = JSON.parse(__json)", modulePath });
    } else {
        // scoped vars for consistency with Node.js
        closure.setProperty("require", module.property("require"));
        closure.setProperty("__filename", modulePath, READONLY_HIDDEN_PROP_FLAGS);
        closure.setProperty("__dirname", QString(modulePath).replace(QRegExp("/[^/]*$"), ""), READONLY_HIDDEN_PROP_FLAGS);
        result = evaluateInClosure(closure, { sourceCode, modulePath });
    }
    maybeEmitUncaughtException(__FUNCTION__);
    return result;
}

// CommonJS/Node.js like require/module support
QScriptValue ScriptEngine::require(const QString& moduleId) {
    qCDebug(scriptengine_module) << "ScriptEngine::require(" << moduleId.left(MAX_DEBUG_VALUE_LENGTH) << ")";
    if (!IS_THREADSAFE_INVOCATION(thread(), __FUNCTION__)) {
        return unboundNullValue();
    }

    auto jsRequire = globalObject().property("Script").property("require");
    auto cacheMeta = jsRequire.data();
    auto cache = jsRequire.property("cache");
    auto parent = currentModule();

    auto throwModuleError = [&](const QString& modulePath, const QScriptValue& error) {
        cache.setProperty(modulePath, nullValue());
        if (!error.isNull()) {
#ifdef DEBUG_JS_MODULES
            qCWarning(scriptengine_module) << "throwing module error:" << error.toString() << modulePath << error.property("stack").toString();
#endif
            raiseException(error);
        }
        maybeEmitUncaughtException("module");
        return unboundNullValue();
    };

    // start by resolving the moduleId into a fully-qualified path/URL
    QString modulePath = _requireResolve(moduleId);
    if (modulePath.isNull() || hasUncaughtException()) {
        // the resolver already threw an exception -- bail early
        maybeEmitUncaughtException(__FUNCTION__);
        return unboundNullValue();
    }

    // check the resolved path against the cache
    auto module = cache.property(modulePath);

    // modules get cached in `Script.require.cache` and (similar to Node.js) users can access it
    // to inspect particular entries and invalidate them by deleting the key:
    //   `delete Script.require.cache[Script.require.resolve(moduleId)];`

    // cacheMeta is just used right now to tell deleted keys apart from undefined ones
    bool invalidateCache = module.isUndefined() && cacheMeta.property(moduleId).isValid();

    // reset the cacheMeta record so invalidation won't apply next time, even if the module fails to load
    cacheMeta.setProperty(modulePath, QScriptValue());

    auto exports = module.property("exports");
    if (!invalidateCache && exports.isObject()) {
        // we have found a cached module -- just need to possibly register it with current parent
        qCDebug(scriptengine_module) << QString("require - using cached module '%1' for '%2' (loaded: %3)")
            .arg(modulePath).arg(moduleId).arg(module.property("loaded").toString());
        registerModuleWithParent(module, parent);
        maybeEmitUncaughtException("cached module");
        return exports;
    }

    // bootstrap / register new empty module
    module = newModule(modulePath, parent);
    registerModuleWithParent(module, parent);

    // add it to the cache (this is done early so any cyclic dependencies pick up)
    cache.setProperty(modulePath, module);

    // download the module source
    auto req = fetchModuleSource(modulePath, invalidateCache);

    if (!req.contains("success") || !req["success"].toBool()) {
        auto error = QString("error retrieving script (%1)").arg(req["status"].toString());
        return throwModuleError(modulePath, error);
    }

#if DEBUG_JS_MODULES
    qCDebug(scriptengine_module) << "require.loaded: " <<
        QUrl(req["url"].toString()).fileName() << req["status"].toString();
#endif

    auto sourceCode = req["contents"].toString();

    if (QUrl(modulePath).fileName().endsWith(".json", Qt::CaseInsensitive)) {
        module.setProperty("content-type", "application/json");
    } else {
        module.setProperty("content-type", "application/javascript");
    }

    // evaluate the module
    auto result = instantiateModule(module, sourceCode);

    if (result.isError() && !result.strictlyEquals(module.property("exports"))) {
        qCWarning(scriptengine_module) << "-- result.isError --" << result.toString();
        return throwModuleError(modulePath, result);
    }

    // mark as fully-loaded
    module.setProperty("loaded", true, READONLY_PROP_FLAGS);

    // set up a new reference point for detecting cache key deletion
    cacheMeta.setProperty(modulePath, module);

    qCDebug(scriptengine_module) << "//ScriptEngine::require(" << moduleId << ")";

    maybeEmitUncaughtException(__FUNCTION__);
    return module.property("exports");
}

// If a callback is specified, the included files will be loaded asynchronously and the callback will be called
// when all of the files have finished loading.
// If no callback is specified, the included files will be loaded synchronously and will block execution until
// all of the files have finished loading.
void ScriptEngine::include(const QStringList& includeFiles, QScriptValue callback) {
    if (!IS_THREADSAFE_INVOCATION(thread(), __FUNCTION__)) {
        return;
    }
    if (DependencyManager::get<ScriptEngines>()->isStopped()) {
        scriptWarningMessage("Script.include() while shutting down is ignored... includeFiles:"
                + includeFiles.join(",") + "parent script:" + getFilename());
        return; // bail early
    }
    QList<QUrl> urls;

    for (QString includeFile : includeFiles) {
        QString file = DependencyManager::get<ResourceManager>()->normalizeURL(includeFile);
        QUrl thisURL;
        bool isStandardLibrary = false;
        if (file.startsWith("/~/")) {
            thisURL = expandScriptUrl(QUrl::fromLocalFile(expandScriptPath(file)));
            QUrl defaultScriptsLoc = PathUtils::defaultScriptsLocation();
            if (!defaultScriptsLoc.isParentOf(thisURL)) {
                scriptWarningMessage("Script.include() -- skipping" + file + "-- outside of standard libraries");
                continue;
            }
            isStandardLibrary = true;
        } else {
            thisURL = resolvePath(file);
        }

        bool disallowOutsideFiles = thisURL.isLocalFile() && !isStandardLibrary && !currentSandboxURL.isLocalFile();
        if (disallowOutsideFiles && !PathUtils::isDescendantOf(thisURL, currentSandboxURL)) {
            scriptWarningMessage("Script.include() ignoring file path" + thisURL.toString()
                                + "outside of original entity script" + currentSandboxURL.toString());
        } else {
            // We could also check here for CORS, but we don't yet.
            // It turns out that QUrl.resolve will not change hosts and copy authority, so we don't need to check that here.
            urls.append(thisURL);
        }
    }

    // If there are no URLs left to download, don't bother attempting to download anything and return early
    if (urls.size() == 0) {
        return;
    }

    BatchLoader* loader = new BatchLoader(urls);
    EntityItemID capturedEntityIdentifier = currentEntityIdentifier;
    QUrl capturedSandboxURL = currentSandboxURL;

    auto evaluateScripts = [=](const QMap<QUrl, QString>& data, const QMap<QUrl, QString>& status) {
        auto parentURL = _parentURL;
        for (QUrl url : urls) {
            QString contents = data[url];
            if (contents.isNull()) {
                scriptErrorMessage("Error loading file (" + status[url] +"): " + url.toString());
            } else {
                std::lock_guard<std::recursive_mutex> lock(_lock);
                if (!_includedURLs.contains(url)) {
                    _includedURLs << url;
                    // Set the parent url so that path resolution will be relative
                    // to this script's url during its initial evaluation
                    _parentURL = url.toString();
                    auto operation = [&]() {
                        evaluate(contents, url.toString());
                    };

                    doWithEnvironment(capturedEntityIdentifier, capturedSandboxURL, operation);
                    if (hasUncaughtException()) {
                        emit unhandledException(cloneUncaughtException("evaluateInclude"));
                        clearExceptions();
                    }
                } else {
                    scriptPrintedMessage("Script.include() skipping evaluation of previously included url:" + url.toString());
                }
            }
        }
        _parentURL = parentURL;

        if (callback.isFunction()) {
            callWithEnvironment(capturedEntityIdentifier, capturedSandboxURL, QScriptValue(callback), QScriptValue(), QScriptValueList());
        }

        loader->deleteLater();
    };

    connect(loader, &BatchLoader::finished, this, evaluateScripts);

    // If we are destroyed before the loader completes, make sure to clean it up
    connect(this, &QObject::destroyed, loader, &QObject::deleteLater);

    loader->start(processLevelMaxRetries);

    if (!callback.isFunction() && !loader->isFinished()) {
        QEventLoop loop;
        QObject::connect(loader, &BatchLoader::finished, &loop, &QEventLoop::quit);
        loop.exec();
    }
}

void ScriptEngine::include(const QString& includeFile, QScriptValue callback) {
    if (DependencyManager::get<ScriptEngines>()->isStopped()) {
        scriptWarningMessage("Script.include() while shutting down is ignored...  includeFile:"
                    + includeFile + "parent script:" + getFilename());
        return; // bail early
    }

    QStringList urls;
    urls.append(includeFile);
    include(urls, callback);
}

// NOTE: The load() command is similar to the include() command except that it loads the script
// as a stand-alone script. To accomplish this, the ScriptEngine class just emits a signal which
// the Application or other context will connect to in order to know to actually load the script
void ScriptEngine::load(const QString& loadFile) {
    if (!IS_THREADSAFE_INVOCATION(thread(), __FUNCTION__)) {
        return;
    }
    if (DependencyManager::get<ScriptEngines>()->isStopped()) {
        scriptWarningMessage("Script.load() while shutting down is ignored... loadFile:"
                + loadFile + "parent script:" + getFilename());
        return; // bail early
    }
    if (!currentEntityIdentifier.isInvalidID()) {
        scriptWarningMessage("Script.load() from entity script is ignored...  loadFile:"
                + loadFile + "parent script:" + getFilename() + "entity: " + currentEntityIdentifier.toString());
        return; // bail early
    }

    QUrl url = resolvePath(loadFile);
    if (_isReloading) {
        auto scriptCache = DependencyManager::get<ScriptCache>();
        scriptCache->deleteScript(url.toString());
        emit reloadScript(url.toString(), false);
    } else {
        emit loadScript(url.toString(), false);
    }
}

// Look up the handler associated with eventName and entityID. If found, evalute the argGenerator thunk and call the handler with those args
void ScriptEngine::forwardHandlerCall(const EntityItemID& entityID, const QString& eventName, QScriptValueList eventHandlerArgs) {
    if (QThread::currentThread() != thread()) {
        qCDebug(scriptengine) << "*** ERROR *** ScriptEngine::forwardHandlerCall() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "]";
        assert(false);
        return ;
    }
    if (!_registeredHandlers.contains(entityID)) {
        return;
    }
    const RegisteredEventHandlers& handlersOnEntity = _registeredHandlers[entityID];
    if (!handlersOnEntity.contains(eventName)) {
        return;
    }
    CallbackList handlersForEvent = handlersOnEntity[eventName];
    if (!handlersForEvent.isEmpty()) {
        for (int i = 0; i < handlersForEvent.count(); ++i) {
            // handlersForEvent[i] can contain many handlers that may have each been added by different interface or entity scripts,
            // and the entity scripts may be for entities other than the one this is a handler for.
            // Fortunately, the definingEntityIdentifier captured the entity script id (if any) when the handler was added.
            CallbackData& handler = handlersForEvent[i];
            callWithEnvironment(handler.definingEntityIdentifier, handler.definingSandboxURL, handler.function, QScriptValue(), eventHandlerArgs);
        }
    }
}

int ScriptEngine::getNumRunningEntityScripts() const {
    int sum = 0;
    for (auto& st : _entityScripts) {
        if (st.status == EntityScriptStatus::RUNNING) {
            ++sum;
        }
    }
    return sum;
}

void ScriptEngine::setEntityScriptDetails(const EntityItemID& entityID, const EntityScriptDetails& details) {
    _entityScripts[entityID] = details;
    emit entityScriptDetailsUpdated();
}

void ScriptEngine::updateEntityScriptStatus(const EntityItemID& entityID, const EntityScriptStatus &status, const QString& errorInfo) {
    EntityScriptDetails &details = _entityScripts[entityID];
    details.status = status;
    details.errorInfo = errorInfo;
    emit entityScriptDetailsUpdated();
}

QVariant ScriptEngine::cloneEntityScriptDetails(const EntityItemID& entityID) {
    static const QVariant NULL_VARIANT { qVariantFromValue((QObject*)nullptr) };
    QVariantMap map;
    if (entityID.isNull()) {
        // TODO: find better way to report JS Error across thread/process boundaries
        map["isError"] = true;
        map["errorInfo"] = "Error: getEntityScriptDetails -- invalid entityID";
    } else {
#ifdef DEBUG_ENTITY_STATES
        qDebug() << "cloneEntityScriptDetails" << entityID << QThread::currentThread();
#endif
        EntityScriptDetails scriptDetails;
        if (getEntityScriptDetails(entityID, scriptDetails)) {
#ifdef DEBUG_ENTITY_STATES
            qDebug() << "gotEntityScriptDetails" << scriptDetails.status << QThread::currentThread();
#endif
            map["isRunning"] = isEntityScriptRunning(entityID);
            map["status"] = EntityScriptStatus_::valueToKey(scriptDetails.status).toLower();
            map["errorInfo"] = scriptDetails.errorInfo;
            map["entityID"] = entityID.toString();
#ifdef DEBUG_ENTITY_STATES
            {
                auto debug = QVariantMap();
                debug["script"] = scriptDetails.scriptText;
                debug["scriptObject"] = scriptDetails.scriptObject.toVariant();
                debug["lastModified"] = (qlonglong)scriptDetails.lastModified;
                debug["sandboxURL"] = scriptDetails.definingSandboxURL;
                map["debug"] = debug;
            }
#endif
        } else {
#ifdef DEBUG_ENTITY_STATES
            qDebug() << "!gotEntityScriptDetails" <<  QThread::currentThread();
#endif
            map["isError"] = true;
            map["errorInfo"] = "Entity script details unavailable";
            map["entityID"] = entityID.toString();
        }
    }
    return map;
}

QFuture<QVariant> ScriptEngine::getLocalEntityScriptDetails(const EntityItemID& entityID) {
    return QtConcurrent::run(this, &ScriptEngine::cloneEntityScriptDetails, entityID);
}

bool ScriptEngine::getEntityScriptDetails(const EntityItemID& entityID, EntityScriptDetails &details) const {
    auto it = _entityScripts.constFind(entityID);
    if (it == _entityScripts.constEnd()) {
        return false;
    }
    details = it.value();
    return true;
}

const static EntityItemID BAD_SCRIPT_UUID_PLACEHOLDER { "{20170224-dead-face-0000-cee000021114}" };

void ScriptEngine::processDeferredEntityLoads(const QString& entityScript, const EntityItemID& leaderID) {
    QList<DeferredLoadEntity> retryLoads;
    QMutableListIterator<DeferredLoadEntity> i(_deferredEntityLoads);
    while (i.hasNext()) {
        auto retry = i.next();
        if (retry.entityScript == entityScript) {
            retryLoads << retry;
            i.remove();
        }
    }
    foreach(DeferredLoadEntity retry, retryLoads) {
        // check whether entity was since been deleted
        if (!_entityScripts.contains(retry.entityID)) {
            qCDebug(scriptengine) << "processDeferredEntityLoads -- entity details gone (entity deleted?)"
                                  << retry.entityID;
            continue;
        }

        // check whether entity has since been unloaded or otherwise errored-out
        auto details = _entityScripts[retry.entityID];
        if (details.status != EntityScriptStatus::PENDING) {
            qCDebug(scriptengine) << "processDeferredEntityLoads -- entity status no longer PENDING; "
                                  << retry.entityID << details.status;
            continue;
        }

        // propagate leader's failure reasons to the pending entity
        const auto leaderDetails = _entityScripts[leaderID];
        if (leaderDetails.status != EntityScriptStatus::RUNNING) {
            qCDebug(scriptengine) << QString("... pending load of %1 cancelled (leader: %2 status: %3)")
                .arg(retry.entityID.toString()).arg(leaderID.toString()).arg(leaderDetails.status);

            auto extraDetail = QString("\n(propagated from %1)").arg(leaderID.toString());
            if (leaderDetails.status == EntityScriptStatus::ERROR_LOADING_SCRIPT ||
                leaderDetails.status == EntityScriptStatus::ERROR_RUNNING_SCRIPT) {
                // propagate same error so User doesn't have to hunt down stampede's leader
                updateEntityScriptStatus(retry.entityID, leaderDetails.status, leaderDetails.errorInfo + extraDetail);
            } else {
                // the leader Entity somehow ended up in some other state (rapid-fire delete or unload could cause)
                updateEntityScriptStatus(retry.entityID, EntityScriptStatus::ERROR_LOADING_SCRIPT,
                    "A previous Entity failed to load using this script URL; reload to try again." + extraDetail);
            }
            continue;
        }

        if (_occupiedScriptURLs.contains(retry.entityScript)) {
            qCWarning(scriptengine) << "--- SHOULD NOT HAPPEN -- recursive call into processDeferredEntityLoads" << retry.entityScript;
            continue;
        }

        // if we made it here then the leading entity was successful so proceed with normal load
        loadEntityScript(retry.entityID, retry.entityScript, false);
    }
}

void ScriptEngine::loadEntityScript(const EntityItemID& entityID, const QString& entityScript, bool forceRedownload) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "loadEntityScript",
            Q_ARG(const EntityItemID&, entityID),
            Q_ARG(const QString&, entityScript),
            Q_ARG(bool, forceRedownload)
        );
        return;
    }
    PROFILE_RANGE(script, __FUNCTION__);

    if (isStopping() || DependencyManager::get<ScriptEngines>()->isStopped()) {
        qCDebug(scriptengine) << "loadEntityScript.start " << entityScript << entityID.toString()
                                     << " but isStopping==" << isStopping()
                                     << " || engines->isStopped==" << DependencyManager::get<ScriptEngines>()->isStopped();
        return;
    }

    if (!_entityScripts.contains(entityID)) {
        // make sure EntityScriptDetails has an entry for this UUID right away
        // (which allows bailing from the loading/provisioning process early if the Entity gets deleted mid-flight)
        updateEntityScriptStatus(entityID, EntityScriptStatus::PENDING, "...pending...");
    }

    // This "occupied" approach allows multiple Entities to boot from the same script URL while still taking
    // full advantage of cacheable require modules.  This only affects Entities literally coming in back-to-back
    // before the first one has time to finish loading.
    if (_occupiedScriptURLs.contains(entityScript)) {
        auto currentEntityID = _occupiedScriptURLs[entityScript];
        if (currentEntityID == BAD_SCRIPT_UUID_PLACEHOLDER) {
            if (forceRedownload) {
                // script was previously marked unusable, but we're reloading so reset it
                _occupiedScriptURLs.remove(entityScript);
            } else {
                // since not reloading, assume that the exact same input would produce the exact same output again
                // note: this state gets reset with "reload all scripts," leaving/returning to a Domain, clear cache, etc.
#ifdef DEBUG_ENTITY_STATES
                qCDebug(scriptengine) << QString("loadEntityScript.cancelled entity: %1 script: %2 (previous script failure)")
                    .arg(entityID.toString()).arg(entityScript);
#endif
                updateEntityScriptStatus(entityID, EntityScriptStatus::ERROR_LOADING_SCRIPT,
                                         "A previous Entity failed to load using this script URL; reload to try again.");
                return;
            }
        } else {
            // another entity is busy loading from this script URL so wait for them to finish
#ifdef DEBUG_ENTITY_STATES
            qCDebug(scriptengine) << QString("loadEntityScript.deferring[%0] entity: %1 script: %2 (waiting on %3)")
                .arg(_deferredEntityLoads.size()).arg(entityID.toString()).arg(entityScript).arg(currentEntityID.toString());
#endif
            _deferredEntityLoads.push_back({ entityID, entityScript });
            return;
        }
    }

    // the scriptURL slot is available; flag as in-use
    _occupiedScriptURLs[entityScript] = entityID;

#ifdef DEBUG_ENTITY_STATES
    auto previousStatus = _entityScripts.contains(entityID) ? _entityScripts[entityID].status : EntityScriptStatus::PENDING;
    qCDebug(scriptengine) << "loadEntityScript.LOADING: " << entityScript << entityID.toString()
                                 << "(previous: " << previousStatus << ")";
#endif

    EntityScriptDetails newDetails;
    newDetails.scriptText = entityScript;
    newDetails.status = EntityScriptStatus::LOADING;
    newDetails.definingSandboxURL = currentSandboxURL;
    setEntityScriptDetails(entityID, newDetails);

    auto scriptCache = DependencyManager::get<ScriptCache>();
    // note: see EntityTreeRenderer.cpp for shared pointer lifecycle management
    QWeakPointer<BaseScriptEngine> weakRef(sharedFromThis());
    scriptCache->getScriptContents(entityScript,
        [this, weakRef, entityScript, entityID](const QString& url, const QString& contents, bool isURL, bool success, const QString& status) {
            QSharedPointer<BaseScriptEngine> strongRef(weakRef);
            if (!strongRef) {
                qCWarning(scriptengine) << "loadEntityScript.contentAvailable -- ScriptEngine was deleted during getScriptContents!!";
                return;
            }
            if (isStopping()) {
#ifdef DEBUG_ENTITY_STATES
                qCDebug(scriptengine) << "loadEntityScript.contentAvailable -- stopping";
#endif
                return;
            }
            executeOnScriptThread([=]{
#ifdef DEBUG_ENTITY_STATES
                qCDebug(scriptengine) << "loadEntityScript.contentAvailable" << status << QUrl(url).fileName() << entityID.toString();
#endif
                if (!isStopping() && _entityScripts.contains(entityID)) {
                    entityScriptContentAvailable(entityID, url, contents, isURL, success, status);
                } else {
#ifdef DEBUG_ENTITY_STATES
                    qCDebug(scriptengine) << "loadEntityScript.contentAvailable -- aborting";
#endif
                }
                // recheck whether us since may have been set to BAD_SCRIPT_UUID_PLACEHOLDER in entityScriptContentAvailable
                if (_occupiedScriptURLs.contains(entityScript) && _occupiedScriptURLs[entityScript] == entityID) {
                    _occupiedScriptURLs.remove(entityScript);
                }
            });
    }, forceRedownload);
}

// since all of these operations can be asynch we will always do the actual work in the response handler
// for the download
void ScriptEngine::entityScriptContentAvailable(const EntityItemID& entityID, const QString& scriptOrURL, const QString& contents, bool isURL, bool success , const QString& status) {
    if (QThread::currentThread() != thread()) {
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngine::entityScriptContentAvailable() called on wrong thread ["
            << QThread::currentThread() << "], invoking on correct thread [" << thread()
            << "]  " "entityID:" << entityID << "scriptOrURL:" << scriptOrURL << "contents:"
            << contents << "isURL:" << isURL << "success:" << success;
#endif

        QMetaObject::invokeMethod(this, "entityScriptContentAvailable",
                                  Q_ARG(const EntityItemID&, entityID),
                                  Q_ARG(const QString&, scriptOrURL),
                                  Q_ARG(const QString&, contents),
                                  Q_ARG(bool, isURL),
                                  Q_ARG(bool, success),
                                  Q_ARG(const QString&, status));
        return;
    }

#ifdef THREAD_DEBUGGING
    qCDebug(scriptengine) << "ScriptEngine::entityScriptContentAvailable() thread [" << QThread::currentThread() << "] expected thread [" << thread() << "]";
#endif

    auto scriptCache = DependencyManager::get<ScriptCache>();
    bool isFileUrl = isURL && scriptOrURL.startsWith("file://");
    auto fileName = isURL ? scriptOrURL : "about:EmbeddedEntityScript";

    const EntityScriptDetails &oldDetails = _entityScripts[entityID];
    const QString entityScript = oldDetails.scriptText;

    EntityScriptDetails newDetails;
    newDetails.scriptText = scriptOrURL;

    // If an error happens below, we want to update newDetails with the new status info
    // and also abort any pending Entity loads that are waiting on the exact same script URL.
    auto setError = [&](const QString &errorInfo, const EntityScriptStatus& status) {
        newDetails.errorInfo = errorInfo;
        newDetails.status = status;
        setEntityScriptDetails(entityID, newDetails);

#ifdef DEBUG_ENTITY_STATES
        qCDebug(scriptengine) << "entityScriptContentAvailable -- flagging " << entityScript << " as BAD_SCRIPT_UUID_PLACEHOLDER";
#endif
        // flag the original entityScript as unusuable
        _occupiedScriptURLs[entityScript] = BAD_SCRIPT_UUID_PLACEHOLDER;
        processDeferredEntityLoads(entityScript, entityID);
    };

    // NETWORK / FILESYSTEM ERRORS
    if (!success) {
        setError("Failed to load script (" + status + ")", EntityScriptStatus::ERROR_LOADING_SCRIPT);
        return;
    }

    // SYNTAX ERRORS
    auto syntaxError = lintScript(contents, fileName);
    if (syntaxError.isError()) {
        auto message = syntaxError.property("formatted").toString();
        if (message.isEmpty()) {
            message = syntaxError.toString();
        }
        setError(QString("Bad syntax (%1)").arg(message), EntityScriptStatus::ERROR_RUNNING_SCRIPT);
        syntaxError.setProperty("detail", entityID.toString());
        emit unhandledException(syntaxError);
        return;
    }
    QScriptProgram program { contents, fileName };
    if (program.isNull()) {
        setError("Bad program (isNull)", EntityScriptStatus::ERROR_RUNNING_SCRIPT);
        emit unhandledException(makeError("program.isNull"));
        return; // done processing script
    }

    if (isURL) {
        setParentURL(scriptOrURL);
    }

    // SANITY/PERFORMANCE CHECK USING SANDBOX
    const int SANDBOX_TIMEOUT = 0.25 * MSECS_PER_SECOND;
    BaseScriptEngine sandbox;
    sandbox.setProcessEventsInterval(SANDBOX_TIMEOUT);
    QScriptValue testConstructor, exception;
    {
        QTimer timeout;
        timeout.setSingleShot(true);
        timeout.start(SANDBOX_TIMEOUT);
        connect(&timeout, &QTimer::timeout, [&sandbox, SANDBOX_TIMEOUT, scriptOrURL]{
                qCDebug(scriptengine) << "ScriptEngine::entityScriptContentAvailable timeout(" << scriptOrURL << ")";

                // Guard against infinite loops and non-performant code
                sandbox.raiseException(
                    sandbox.makeError(QString("Timed out (entity constructors are limited to %1ms)").arg(SANDBOX_TIMEOUT))
                );
        });

        testConstructor = sandbox.evaluate(program);

        if (sandbox.hasUncaughtException()) {
            exception = sandbox.cloneUncaughtException(QString("(preflight %1)").arg(entityID.toString()));
            sandbox.clearExceptions();
        } else if (testConstructor.isError()) {
            exception = testConstructor;
        }
    }

    if (exception.isError()) {
        // create a local copy using makeError to decouple from the sandbox engine
        exception = makeError(exception);
        setError(formatException(exception, _enableExtendedJSExceptions.get()), EntityScriptStatus::ERROR_RUNNING_SCRIPT);
        emit unhandledException(exception);
        return;
    }

    // CONSTRUCTOR VIABILITY
    if (!testConstructor.isFunction()) {
        QString testConstructorType = QString(testConstructor.toVariant().typeName());
        if (testConstructorType == "") {
            testConstructorType = "empty";
        }
        QString testConstructorValue = testConstructor.toString();
        if (testConstructorValue.size() > MAX_DEBUG_VALUE_LENGTH) {
            testConstructorValue = testConstructorValue.mid(0, MAX_DEBUG_VALUE_LENGTH) + "...";
        }
        auto message = QString("failed to load entity script -- expected a function, got %1, %2")
            .arg(testConstructorType).arg(testConstructorValue);

        auto err = makeError(message);
        err.setProperty("fileName", scriptOrURL);
        err.setProperty("detail", "(constructor " + entityID.toString() + ")");

        setError("Could not find constructor (" + testConstructorType + ")", EntityScriptStatus::ERROR_RUNNING_SCRIPT);
        emit unhandledException(err);
        return; // done processing script
    }

    // (this feeds into refreshFileScript)
    int64_t lastModified = 0;
    if (isFileUrl) {
        QString file = QUrl(scriptOrURL).toLocalFile();
        lastModified = (quint64)QFileInfo(file).lastModified().toMSecsSinceEpoch();
    }

    // THE ACTUAL EVALUATION AND CONSTRUCTION
    QScriptValue entityScriptConstructor, entityScriptObject;
    QUrl sandboxURL = currentSandboxURL.isEmpty() ? scriptOrURL : currentSandboxURL;
    auto initialization = [&]{
        entityScriptConstructor = evaluate(contents, fileName);
        entityScriptObject = entityScriptConstructor.construct();

        if (hasUncaughtException()) {
            entityScriptObject = cloneUncaughtException("(construct " + entityID.toString() + ")");
            clearExceptions();
        }
    };

    doWithEnvironment(entityID, sandboxURL, initialization);

    if (entityScriptObject.isError()) {
        auto exception = entityScriptObject;
        setError(formatException(exception, _enableExtendedJSExceptions.get()), EntityScriptStatus::ERROR_RUNNING_SCRIPT);
        emit unhandledException(exception);
        return;
    }

    // ... AND WE HAVE LIFTOFF
    newDetails.status = EntityScriptStatus::RUNNING;
    newDetails.scriptObject = entityScriptObject;
    newDetails.lastModified = lastModified;
    newDetails.definingSandboxURL = sandboxURL;
    setEntityScriptDetails(entityID, newDetails);

    if (isURL) {
        setParentURL("");
    }

    // if we got this far, then call the preload method
    callEntityScriptMethod(entityID, "preload");

    _occupiedScriptURLs.remove(entityScript);
    processDeferredEntityLoads(entityScript, entityID);
}

void ScriptEngine::unloadEntityScript(const EntityItemID& entityID, bool shouldRemoveFromMap) {
    if (QThread::currentThread() != thread()) {
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngine::unloadEntityScript() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "]  "
            "entityID:" << entityID;
#endif

        QMetaObject::invokeMethod(this, "unloadEntityScript",
                                  Q_ARG(const EntityItemID&, entityID),
                                  Q_ARG(bool, shouldRemoveFromMap));
        return;
    }
#ifdef THREAD_DEBUGGING
    qCDebug(scriptengine) << "ScriptEngine::unloadEntityScript() called on correct thread [" << thread() << "]  "
        "entityID:" << entityID;
#endif

    if (_entityScripts.contains(entityID)) {
        const EntityScriptDetails &oldDetails = _entityScripts[entityID];
        auto scriptText = oldDetails.scriptText;

        if (isEntityScriptRunning(entityID)) {
            callEntityScriptMethod(entityID, "unload");
        }
#ifdef DEBUG_ENTITY_STATES
        else {
            qCDebug(scriptengine) << "unload called while !running" << entityID << oldDetails.status;
        }
#endif
        if (shouldRemoveFromMap) {
            // this was a deleted entity, we've been asked to remove it from the map
            _entityScripts.remove(entityID);
            emit entityScriptDetailsUpdated();
        } else if (oldDetails.status != EntityScriptStatus::UNLOADED) {
            EntityScriptDetails newDetails;
            newDetails.status = EntityScriptStatus::UNLOADED;
            newDetails.lastModified = QDateTime::currentMSecsSinceEpoch();
            // keep scriptText populated for the current need to "debouce" duplicate calls to unloadEntityScript
            newDetails.scriptText = scriptText;
            setEntityScriptDetails(entityID, newDetails);
        }

        stopAllTimersForEntityScript(entityID);
        {
            // FIXME: shouldn't have to do this here, but currently something seems to be firing unloads moments after firing initial load requests
            processDeferredEntityLoads(scriptText, entityID);
        }
    }
}

void ScriptEngine::unloadAllEntityScripts() {
    if (QThread::currentThread() != thread()) {
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngine::unloadAllEntityScripts() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "]";
#endif

        QMetaObject::invokeMethod(this, "unloadAllEntityScripts");
        return;
    }
#ifdef THREAD_DEBUGGING
    qCDebug(scriptengine) << "ScriptEngine::unloadAllEntityScripts() called on correct thread [" << thread() << "]";
#endif
    foreach(const EntityItemID& entityID, _entityScripts.keys()) {
        unloadEntityScript(entityID);
    }
    _entityScripts.clear();
    emit entityScriptDetailsUpdated();
    _occupiedScriptURLs.clear();

#ifdef DEBUG_ENGINE_STATE
    _debugDump(
        "---- CURRENT STATE OF ENGINE: --------------------------",
        globalObject(),
        "--------------------------------------------------------"
    );
#endif // DEBUG_ENGINE_STATE
}

void ScriptEngine::refreshFileScript(const EntityItemID& entityID) {
    if (!HIFI_AUTOREFRESH_FILE_SCRIPTS || !_entityScripts.contains(entityID)) {
        return;
    }

    static bool recurseGuard = false;
    if (recurseGuard) {
        return;
    }
    recurseGuard = true;

    EntityScriptDetails details = _entityScripts[entityID];
    // Check to see if a file based script needs to be reloaded (easier debugging)
    if (details.lastModified > 0) {
        QString filePath = QUrl(details.scriptText).toLocalFile();
        auto lastModified = QFileInfo(filePath).lastModified().toMSecsSinceEpoch();
        if (lastModified > details.lastModified) {
            scriptInfoMessage("Reloading modified script " + details.scriptText);
            loadEntityScript(entityID, details.scriptText, true);
        }
    }
    recurseGuard = false;
}

// Execute operation in the appropriate context for (the possibly empty) entityID.
// Even if entityID is supplied as currentEntityIdentifier, this still documents the source
// of the code being executed (e.g., if we ever sandbox different entity scripts, or provide different
// global values for different entity scripts).
void ScriptEngine::doWithEnvironment(const EntityItemID& entityID, const QUrl& sandboxURL, std::function<void()> operation) {
    EntityItemID oldIdentifier = currentEntityIdentifier;
    QUrl oldSandboxURL = currentSandboxURL;
    currentEntityIdentifier = entityID;
    currentSandboxURL = sandboxURL;

#if DEBUG_CURRENT_ENTITY
    QScriptValue oldData = this->globalObject().property("debugEntityID");
    this->globalObject().setProperty("debugEntityID", entityID.toScriptValue(this)); // Make the entityID available to javascript as a global.
    operation();
    this->globalObject().setProperty("debugEntityID", oldData);
#else
    operation();
#endif
    maybeEmitUncaughtException(!entityID.isNull() ? entityID.toString() : __FUNCTION__);
    currentEntityIdentifier = oldIdentifier;
    currentSandboxURL = oldSandboxURL;
}

void ScriptEngine::callWithEnvironment(const EntityItemID& entityID, const QUrl& sandboxURL, QScriptValue function, QScriptValue thisObject, QScriptValueList args) {
    auto operation = [&]() {
        function.call(thisObject, args);
    };
    doWithEnvironment(entityID, sandboxURL, operation);
}

void ScriptEngine::callEntityScriptMethod(const EntityItemID& entityID, const QString& methodName, const QStringList& params) {
    if (QThread::currentThread() != thread()) {
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngine::callEntityScriptMethod() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "]  "
            "entityID:" << entityID << "methodName:" << methodName;
#endif

        QMetaObject::invokeMethod(this, "callEntityScriptMethod",
                                  Q_ARG(const EntityItemID&, entityID),
                                  Q_ARG(const QString&, methodName),
                                  Q_ARG(const QStringList&, params));
        return;
    }
#ifdef THREAD_DEBUGGING
    qCDebug(scriptengine) << "ScriptEngine::callEntityScriptMethod() called on correct thread [" << thread() << "]  "
        "entityID:" << entityID << "methodName:" << methodName;
#endif

    if (HIFI_AUTOREFRESH_FILE_SCRIPTS && methodName != "unload") {
        refreshFileScript(entityID);
    }
    if (isEntityScriptRunning(entityID)) {
        EntityScriptDetails details = _entityScripts[entityID];
        QScriptValue entityScript = details.scriptObject; // previously loaded
        if (entityScript.property(methodName).isFunction()) {
            QScriptValueList args;
            args << entityID.toScriptValue(this);
            args << qScriptValueFromSequence(this, params);
            callWithEnvironment(entityID, details.definingSandboxURL, entityScript.property(methodName), entityScript, args);
        }

    }
}

void ScriptEngine::callEntityScriptMethod(const EntityItemID& entityID, const QString& methodName, const PointerEvent& event) {
    if (QThread::currentThread() != thread()) {
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngine::callEntityScriptMethod() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "]  "
            "entityID:" << entityID << "methodName:" << methodName << "event: mouseEvent";
#endif

        QMetaObject::invokeMethod(this, "callEntityScriptMethod",
                                  Q_ARG(const EntityItemID&, entityID),
                                  Q_ARG(const QString&, methodName),
                                  Q_ARG(const PointerEvent&, event));
        return;
    }
#ifdef THREAD_DEBUGGING
    qCDebug(scriptengine) << "ScriptEngine::callEntityScriptMethod() called on correct thread [" << thread() << "]  "
        "entityID:" << entityID << "methodName:" << methodName << "event: pointerEvent";
#endif

    if (HIFI_AUTOREFRESH_FILE_SCRIPTS) {
        refreshFileScript(entityID);
    }
    if (isEntityScriptRunning(entityID)) {
        EntityScriptDetails details = _entityScripts[entityID];
        QScriptValue entityScript = details.scriptObject; // previously loaded
        if (entityScript.property(methodName).isFunction()) {
            QScriptValueList args;
            args << entityID.toScriptValue(this);
            args << event.toScriptValue(this);
            callWithEnvironment(entityID, details.definingSandboxURL, entityScript.property(methodName), entityScript, args);
        }
    }
}

void ScriptEngine::callEntityScriptMethod(const EntityItemID& entityID, const QString& methodName, const EntityItemID& otherID, const Collision& collision) {
    if (QThread::currentThread() != thread()) {
#ifdef THREAD_DEBUGGING
        qCDebug(scriptengine) << "*** WARNING *** ScriptEngine::callEntityScriptMethod() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "]  "
            "entityID:" << entityID << "methodName:" << methodName << "otherID:" << otherID << "collision: collision";
#endif

        QMetaObject::invokeMethod(this, "callEntityScriptMethod",
                                  Q_ARG(const EntityItemID&, entityID),
                                  Q_ARG(const QString&, methodName),
                                  Q_ARG(const EntityItemID&, otherID),
                                  Q_ARG(const Collision&, collision));
        return;
    }
#ifdef THREAD_DEBUGGING
    qCDebug(scriptengine) << "ScriptEngine::callEntityScriptMethod() called on correct thread [" << thread() << "]  "
        "entityID:" << entityID << "methodName:" << methodName << "otherID:" << otherID << "collision: collision";
#endif

    if (HIFI_AUTOREFRESH_FILE_SCRIPTS) {
        refreshFileScript(entityID);
    }
    if (isEntityScriptRunning(entityID)) {
        EntityScriptDetails details = _entityScripts[entityID];
        QScriptValue entityScript = details.scriptObject; // previously loaded
        if (entityScript.property(methodName).isFunction()) {
            QScriptValueList args;
            args << entityID.toScriptValue(this);
            args << otherID.toScriptValue(this);
            args << collisionToScriptValue(this, collision);
            callWithEnvironment(entityID, details.definingSandboxURL, entityScript.property(methodName), entityScript, args);
        }
    }
}

