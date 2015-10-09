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

#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>

#include <AudioConstants.h>
#include <AudioEffectOptions.h>
#include <AvatarData.h>
#include <EntityScriptingInterface.h>
#include <NetworkAccessManager.h>
#include <NodeList.h>
#include <udt/PacketHeaders.h>
#include <UUID.h>

#include "AnimationObject.h"
#include "ArrayBufferViewClass.h"
#include "BatchLoader.h"
#include "DataViewClass.h"
#include "EventTypes.h"
#include "MenuItemProperties.h"
#include "ScriptAudioInjector.h"
#include "ScriptCache.h"
#include "ScriptEngineLogging.h"
#include "ScriptEngine.h"
#include "TypedArrays.h"
#include "XMLHttpRequestClass.h"
#include "WebSocketClass.h"

#include "SceneScriptingInterface.h"

#include "MIDIEvent.h"

Q_DECLARE_METATYPE(QScriptEngine::FunctionSignature)
static int functionSignatureMetaID = qRegisterMetaType<QScriptEngine::FunctionSignature>();

static QScriptValue debugPrint(QScriptContext* context, QScriptEngine* engine){
    QString message = "";
    for (int i = 0; i < context->argumentCount(); i++) {
        if (i > 0) {
            message += " ";
        }
        message += context->argument(i).toString();
    }
    qCDebug(scriptengine) << "script:print()<<" << message;

    message = message.replace("\\", "\\\\")
                     .replace("\n", "\\n")
                     .replace("\r", "\\r")
                     .replace("'", "\\'");
    engine->evaluate("Script.print('" + message + "')");

    return QScriptValue();
}

QScriptValue avatarDataToScriptValue(QScriptEngine* engine, AvatarData* const &in) {
    return engine->newQObject(in);
}

void avatarDataFromScriptValue(const QScriptValue &object, AvatarData* &out) {
    out = qobject_cast<AvatarData*>(object.toQObject());
}

QScriptValue inputControllerToScriptValue(QScriptEngine *engine, AbstractInputController* const &in) {
    return engine->newQObject(in);
}

void inputControllerFromScriptValue(const QScriptValue &object, AbstractInputController* &out) {
    out = qobject_cast<AbstractInputController*>(object.toQObject());
}

ScriptEngine::ScriptEngine(const QString& scriptContents, const QString& fileNameString,
            AbstractControllerScriptingInterface* controllerScriptingInterface, bool wantSignals) :

    _scriptContents(scriptContents),
    _isFinished(false),
    _isRunning(false),
    _isInitialized(false),
    _timerFunctionMap(),
    _wantSignals(wantSignals),
    _controllerScriptingInterface(controllerScriptingInterface),
    _fileNameString(fileNameString),
    _quatLibrary(),
    _vec3Library(),
    _uuidLibrary(),
    _isUserLoaded(false),
    _isReloading(false),
    _arrayBufferClass(new ArrayBufferClass(this))
{
    _allScriptsMutex.lock();
    _allKnownScriptEngines.insert(this);
    _allScriptsMutex.unlock();
}

ScriptEngine::~ScriptEngine() {
    // If we're not already in the middle of stopping all scripts, then we should remove ourselves
    // from the list of running scripts. We don't do this if we're in the process of stopping all scripts
    // because that method removes scripts from its list as it iterates them
    if (!_stoppingAllScripts) {
        _allScriptsMutex.lock();
        _allKnownScriptEngines.remove(this);
        _allScriptsMutex.unlock();
    }
}

void ScriptEngine::runInThread() {
    QThread* workerThread = new QThread(); // thread is not owned, so we need to manage the delete
    QString scriptEngineName = QString("Script Thread:") + getFilename();
    workerThread->setObjectName(scriptEngineName);

    // when the worker thread is started, call our engine's run..
    connect(workerThread, &QThread::started, this, &ScriptEngine::run);

    // tell the thread to stop when the script engine is done
    connect(this, &ScriptEngine::doneRunning, workerThread, &QThread::quit);

    // when the thread is finished, add thread to the deleteLater queue
    connect(workerThread, &QThread::finished, workerThread, &QThread::deleteLater);

    // when the thread is finished, add scriptEngine to the deleteLater queue
    connect(workerThread, &QThread::finished, this, &ScriptEngine::deleteLater);

    moveToThread(workerThread);

    // Starts an event loop, and emits workerThread->started()
    workerThread->start();
}

QSet<ScriptEngine*> ScriptEngine::_allKnownScriptEngines;
QMutex ScriptEngine::_allScriptsMutex;
bool ScriptEngine::_stoppingAllScripts = false;
bool ScriptEngine::_doneRunningThisScript = false;

void ScriptEngine::stopAllScripts(QObject* application) {
    _allScriptsMutex.lock();
    _stoppingAllScripts = true;

    QMutableSetIterator<ScriptEngine*> i(_allKnownScriptEngines);
    while (i.hasNext()) {
        ScriptEngine* scriptEngine = i.next();

        QString scriptName = scriptEngine->getFilename();

        // NOTE: typically all script engines are running. But there's at least one known exception to this, the
        // "entities sandbox" which is only used to evaluate entities scripts to test their validity before using
        // them. We don't need to stop scripts that aren't running.
        if (scriptEngine->isRunning()) {

            // If the script is running, but still evaluating then we need to wait for its evaluation step to
            // complete. After that we can handle the stop process appropriately
            if (scriptEngine->evaluatePending()) {
                while (scriptEngine->evaluatePending()) {

                    // This event loop allows any started, but not yet finished evaluate() calls to complete
                    // we need to let these complete so that we can be guaranteed that the script engine isn't
                    // in a partially setup state, which can confuse our shutdown unwinding.
                    QEventLoop loop;
                    QObject::connect(scriptEngine, &ScriptEngine::evaluationFinished, &loop, &QEventLoop::quit);
                    loop.exec();
                }
            }

            // We disconnect any script engine signals from the application because we don't want to do any
            // extra stopScript/loadScript processing that the Application normally does when scripts start
            // and stop. We can safely short circuit this because we know we're in the "quitting" process
            scriptEngine->disconnect(application);

            // Calling stop on the script engine will set it's internal _isFinished state to true, and result
            // in the ScriptEngine gracefully ending it's run() method.
            scriptEngine->stop();

            // We need to wait for the engine to be done running before we proceed, because we don't
            // want any of the scripts final "scriptEnding()" or pending "update()" methods from accessing
            // any application state after we leave this stopAllScripts() method
            scriptEngine->waitTillDoneRunning();

            // If the script is stopped, we can remove it from our set
            i.remove();
        }
    }
    _stoppingAllScripts = false;
    _allScriptsMutex.unlock();
}


void ScriptEngine::waitTillDoneRunning() {
    // If the script never started running or finished running before we got here, we don't need to wait for it
    if (_isRunning) {

        _doneRunningThisScript = false; // NOTE: this is static, we serialize our waiting for scripts to finish

        // NOTE: waitTillDoneRunning() will be called on the main Application thread, inside of stopAllScripts()
        // we want the application thread to continue to process events, because the scripts will likely need to
        // marshall messages across to the main thread. For example if they access Settings or Meny in any of their
        // shutdown code.
        while (!_doneRunningThisScript) {

            // process events for the main application thread, allowing invokeMethod calls to pass between threads
            QCoreApplication::processEvents();
        }
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


// FIXME - switch this to the new model of ScriptCache callbacks
void ScriptEngine::loadURL(const QUrl& scriptURL, bool reload) {
    if (_isRunning) {
        return;
    }

    _fileNameString = scriptURL.toString();
    _isReloading = reload;

    QUrl url(scriptURL);

    bool isPending;
    auto scriptCache = DependencyManager::get<ScriptCache>();
    scriptCache->getScript(url, this, isPending, reload);
}

// FIXME - switch this to the new model of ScriptCache callbacks
void ScriptEngine::scriptContentsAvailable(const QUrl& url, const QString& scriptContents) {
    _scriptContents = scriptContents;
    if (_wantSignals) {
        emit scriptLoaded(_fileNameString);
    }
}

// FIXME - switch this to the new model of ScriptCache callbacks
void ScriptEngine::errorInLoadingScript(const QUrl& url) {
    qCDebug(scriptengine) << "ERROR Loading file:" << url.toString() << "line:" << __LINE__;
    if (_wantSignals) {
        emit errorLoadingScript(_fileNameString); // ??
    }
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

    if (_controllerScriptingInterface) {
        _controllerScriptingInterface->registerControllerTypes(this);
    }

    qScriptRegisterMetaType(this, EntityPropertyFlagsToScriptValue, EntityPropertyFlagsFromScriptValue);
    qScriptRegisterMetaType(this, EntityItemPropertiesToScriptValue, EntityItemPropertiesFromScriptValueHonorReadOnly);
    qScriptRegisterMetaType(this, EntityItemIDtoScriptValue, EntityItemIDfromScriptValue);
    qScriptRegisterMetaType(this, RayToEntityIntersectionResultToScriptValue, RayToEntityIntersectionResultFromScriptValue);
    qScriptRegisterSequenceMetaType<QVector<QUuid>>(this);
    qScriptRegisterSequenceMetaType<QVector<EntityItemID>>(this);

    qScriptRegisterSequenceMetaType<QVector<glm::vec2> >(this);
    qScriptRegisterSequenceMetaType<QVector<glm::quat> >(this);
    qScriptRegisterSequenceMetaType<QVector<QString> >(this);

    QScriptValue xmlHttpRequestConstructorValue = newFunction(XMLHttpRequestClass::constructor);
    globalObject().setProperty("XMLHttpRequest", xmlHttpRequestConstructorValue);

    QScriptValue webSocketConstructorValue = newFunction(WebSocketClass::constructor);
    globalObject().setProperty("WebSocket", webSocketConstructorValue);

    QScriptValue printConstructorValue = newFunction(debugPrint);
    globalObject().setProperty("print", printConstructorValue);

    QScriptValue audioEffectOptionsConstructorValue = newFunction(AudioEffectOptions::constructor);
    globalObject().setProperty("AudioEffectOptions", audioEffectOptionsConstructorValue);

    qScriptRegisterMetaType(this, injectorToScriptValue, injectorFromScriptValue);
    qScriptRegisterMetaType(this, inputControllerToScriptValue, inputControllerFromScriptValue);
    qScriptRegisterMetaType(this, avatarDataToScriptValue, avatarDataFromScriptValue);
    qScriptRegisterMetaType(this, animationDetailsToScriptValue, animationDetailsFromScriptValue);
    qScriptRegisterMetaType(this, webSocketToScriptValue, webSocketFromScriptValue);
    qScriptRegisterMetaType(this, qWSCloseCodeToScriptValue, qWSCloseCodeFromScriptValue);
    qScriptRegisterMetaType(this, wscReadyStateToScriptValue, wscReadyStateFromScriptValue);

    registerGlobalObject("Script", this);
    registerGlobalObject("Audio", &AudioScriptingInterface::getInstance());
    registerGlobalObject("Controller", _controllerScriptingInterface);
    registerGlobalObject("Entities", entityScriptingInterface.data());
    registerGlobalObject("Quat", &_quatLibrary);
    registerGlobalObject("Vec3", &_vec3Library);
    registerGlobalObject("Uuid", &_uuidLibrary);
    registerGlobalObject("AnimationCache", DependencyManager::get<AnimationCache>().data());

    // constants
    globalObject().setProperty("TREE_SCALE", newVariant(QVariant(TREE_SCALE)));
}

void ScriptEngine::registerGlobalObject(const QString& name, QObject* object) {
    if (QThread::currentThread() != thread()) {
        #ifdef THREAD_DEBUGGING
        qDebug() << "*** WARNING *** ScriptEngine::registerGlobalObject() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "]  name:" << name;
        #endif
        QMetaObject::invokeMethod(this, "registerGlobalObject",
            Q_ARG(const QString&, name),
            Q_ARG(QObject*, object));
        return;
    }
    #ifdef THREAD_DEBUGGING
    qDebug() << "ScriptEngine::registerGlobalObject() called on thread [" << QThread::currentThread() << "] name:" << name;
    #endif

    if (object) {
        QScriptValue value = newQObject(object);
        globalObject().setProperty(name, value);
    }
}

void ScriptEngine::registerFunction(const QString& name, QScriptEngine::FunctionSignature functionSignature, int numArguments) {
    if (QThread::currentThread() != thread()) {
        #ifdef THREAD_DEBUGGING
        qDebug() << "*** WARNING *** ScriptEngine::registerFunction() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "] name:" << name;
        #endif
        QMetaObject::invokeMethod(this, "registerFunction",
            Q_ARG(const QString&, name),
            Q_ARG(QScriptEngine::FunctionSignature, functionSignature),
            Q_ARG(int, numArguments));
        return;
    }
    #ifdef THREAD_DEBUGGING
    qDebug() << "ScriptEngine::registerFunction() called on thread [" << QThread::currentThread() << "] name:" << name;
    #endif

    QScriptValue scriptFun = newFunction(functionSignature, numArguments);
    globalObject().setProperty(name, scriptFun);
}

void ScriptEngine::registerFunction(const QString& parent, const QString& name, QScriptEngine::FunctionSignature functionSignature, int numArguments) {
    if (QThread::currentThread() != thread()) {
        #ifdef THREAD_DEBUGGING
        qDebug() << "*** WARNING *** ScriptEngine::registerFunction() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "] parent:" << parent << "name:" << name;
        #endif
        QMetaObject::invokeMethod(this, "registerFunction",
            Q_ARG(const QString&, name),
            Q_ARG(QScriptEngine::FunctionSignature, functionSignature),
            Q_ARG(int, numArguments));
        return;
    }
    #ifdef THREAD_DEBUGGING
    qDebug() << "ScriptEngine::registerFunction() called on thread [" << QThread::currentThread() << "] parent:" << parent << "name:" << name;
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
        qDebug() << "*** WARNING *** ScriptEngine::registerGetterSetter() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "] "
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
    qDebug() << "ScriptEngine::registerGetterSetter() called on thread [" << QThread::currentThread() << "] name:" << name << "parent:" << parent;
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
        qDebug() << "*** WARNING *** ScriptEngine::removeEventHandler() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "] "
                    "entityID:" << entityID << " eventName:" << eventName;
        #endif
        QMetaObject::invokeMethod(this, "removeEventHandler",
            Q_ARG(const EntityItemID&, entityID),
            Q_ARG(const QString&, eventName),
            Q_ARG(QScriptValue, handler));
        return;
    }
    #ifdef THREAD_DEBUGGING
    qDebug() << "ScriptEngine::removeEventHandler() called on thread [" << QThread::currentThread() << "] entityID:" << entityID << " eventName : " << eventName;
    #endif

    if (!_registeredHandlers.contains(entityID)) {
        return;
    }
    RegisteredEventHandlers& handlersOnEntity = _registeredHandlers[entityID];
    QScriptValueList& handlersForEvent = handlersOnEntity[eventName];
    // QScriptValue does not have operator==(), so we can't use QList::removeOne and friends. So iterate.
    for (int i = 0; i < handlersForEvent.count(); ++i) {
        if (handlersForEvent[i].equals(handler)) {
            handlersForEvent.removeAt(i);
            return; // Design choice: since comparison is relatively expensive, just remove the first matching handler.
        }
    }
}
// Register the handler.
void ScriptEngine::addEventHandler(const EntityItemID& entityID, const QString& eventName, QScriptValue handler) {
    if (QThread::currentThread() != thread()) {
        #ifdef THREAD_DEBUGGING
        qDebug() << "*** WARNING *** ScriptEngine::addEventHandler() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "] "
            "entityID:" << entityID << " eventName:" << eventName;
        #endif

        QMetaObject::invokeMethod(this, "addEventHandler",
            Q_ARG(const EntityItemID&, entityID),
            Q_ARG(const QString&, eventName),
            Q_ARG(QScriptValue, handler));
        return;
    }
    #ifdef THREAD_DEBUGGING
    qDebug() << "ScriptEngine::addEventHandler() called on thread [" << QThread::currentThread() << "] entityID:" << entityID << " eventName : " << eventName;
    #endif

    if (_registeredHandlers.count() == 0) { // First time any per-entity handler has been added in this script...
        // Connect up ALL the handlers to the global entities object's signals.
        // (We could go signal by signal, or even handler by handler, but I don't think the efficiency is worth the complexity.)
        auto entities = DependencyManager::get<EntityScriptingInterface>();
        connect(entities.data(), &EntityScriptingInterface::deletingEntity, this,
                [=](const EntityItemID& entityID) {
                    _registeredHandlers.remove(entityID);
                });

        // Two common cases of event handler, differing only in argument signature.
        auto makeSingleEntityHandler = [=](const QString& eventName) -> std::function<void(const EntityItemID&)> {
            return [=](const EntityItemID& entityItemID) -> void {
                generalHandler(entityItemID, eventName, [=]() -> QScriptValueList {
                    return QScriptValueList() << entityItemID.toScriptValue(this);
                });
            };
        };
        auto makeMouseHandler = [=](const QString& eventName) -> std::function<void(const EntityItemID&, const MouseEvent&)>  {
            return [=](const EntityItemID& entityItemID, const MouseEvent& event) -> void {
                generalHandler(entityItemID, eventName, [=]() -> QScriptValueList {
                    return QScriptValueList() << entityItemID.toScriptValue(this) << event.toScriptValue(this);
                });
            };
        };
        connect(entities.data(), &EntityScriptingInterface::enterEntity, this, makeSingleEntityHandler("enterEntity"));
        connect(entities.data(), &EntityScriptingInterface::leaveEntity, this, makeSingleEntityHandler("leaveEntity"));

        connect(entities.data(), &EntityScriptingInterface::mousePressOnEntity, this, makeMouseHandler("mousePressOnEntity"));
        connect(entities.data(), &EntityScriptingInterface::mouseMoveOnEntity, this, makeMouseHandler("mouseMoveOnEntity"));
        connect(entities.data(), &EntityScriptingInterface::mouseReleaseOnEntity, this, makeMouseHandler("mouseReleaseOnEntity"));

        connect(entities.data(), &EntityScriptingInterface::clickDownOnEntity, this, makeMouseHandler("clickDownOnEntity"));
        connect(entities.data(), &EntityScriptingInterface::holdingClickOnEntity, this, makeMouseHandler("holdingClickOnEntity"));
        connect(entities.data(), &EntityScriptingInterface::clickReleaseOnEntity, this, makeMouseHandler("clickReleaseOnEntity"));

        connect(entities.data(), &EntityScriptingInterface::hoverEnterEntity, this, makeMouseHandler("hoverEnterEntity"));
        connect(entities.data(), &EntityScriptingInterface::hoverOverEntity, this, makeMouseHandler("hoverOverEntity"));
        connect(entities.data(), &EntityScriptingInterface::hoverLeaveEntity, this, makeMouseHandler("hoverLeaveEntity"));

        connect(entities.data(), &EntityScriptingInterface::collisionWithEntity, this,
                [=](const EntityItemID& idA, const EntityItemID& idB, const Collision& collision) {
                    generalHandler(idA, "collisionWithEntity", [=]() {
                        return QScriptValueList () << idA.toScriptValue(this) << idB.toScriptValue(this) << collisionToScriptValue(this, collision);
                    });
                });
    }
    if (!_registeredHandlers.contains(entityID)) {
        _registeredHandlers[entityID] = RegisteredEventHandlers();
    }
    QScriptValueList& handlersForEvent = _registeredHandlers[entityID][eventName];
    handlersForEvent << handler; // Note that the same handler can be added many times. See removeEntityEventHandler().
}


QScriptValue ScriptEngine::evaluate(const QString& program, const QString& fileName, int lineNumber) {
    if (_stoppingAllScripts) {
        return QScriptValue(); // bail early
    }

    if (QThread::currentThread() != thread()) {
        QScriptValue result;
        #ifdef THREAD_DEBUGGING
        qDebug() << "*** WARNING *** ScriptEngine::evaluate() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "] "
            "program:" << program << " fileName:" << fileName << "lineNumber:" << lineNumber;
        #endif
        QMetaObject::invokeMethod(this, "evaluate", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(QScriptValue, result),
            Q_ARG(const QString&, program),
            Q_ARG(const QString&, fileName),
            Q_ARG(int, lineNumber));
        return result;
    }

    _evaluatesPending++;
    QScriptValue result = QScriptEngine::evaluate(program, fileName, lineNumber);
    if (hasUncaughtException()) {
        int line = uncaughtExceptionLineNumber();
        qCDebug(scriptengine) << "Uncaught exception at (" << _fileNameString << " : " << fileName << ") line" << line << ": " << result.toString();
    }
    _evaluatesPending--;
    if (_wantSignals) {
        emit evaluationFinished(result, hasUncaughtException());
    }
    clearExceptions();
    return result;
}

void ScriptEngine::run() {
    // TODO: can we add a short circuit for _stoppingAllScripts here? What does it mean to not start running if
    // we're in the process of stopping?

    if (!_isInitialized) {
        init();
    }
    
    _isRunning = true;
    _isFinished = false;
    if (_wantSignals) {
        emit runningStateChanged();
    }

    QScriptValue result = evaluate(_scriptContents);

    QElapsedTimer startTime;
    startTime.start();

    int thisFrame = 0;

    auto nodeList = DependencyManager::get<NodeList>();
    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();

    qint64 lastUpdate = usecTimestampNow();

    while (!_isFinished) {
        int usecToSleep = (thisFrame++ * SCRIPT_DATA_CALLBACK_USECS) - startTime.nsecsElapsed() / 1000; // nsec to usec
        if (usecToSleep > 0) {
            usleep(usecToSleep);
        }

        if (_isFinished) {
            break;
        }

        QCoreApplication::processEvents();

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
        float deltaTime = (float) (now - lastUpdate) / (float) USECS_PER_SECOND;

        if (hasUncaughtException()) {
            int line = uncaughtExceptionLineNumber();
            qCDebug(scriptengine) << "Uncaught exception at (" << _fileNameString << ") line" << line << ":" << uncaughtException().toString();
            if (_wantSignals) {
                emit errorMessage("Uncaught exception at (" + _fileNameString + ") line" + QString::number(line) + ":" + uncaughtException().toString());
            }
            clearExceptions();
        }

        if (!_isFinished) {
            if (_wantSignals) {
                emit update(deltaTime);
            }
        }
        lastUpdate = now;

    }

    stopAllTimers(); // make sure all our timers are stopped if the script is ending
    if (_wantSignals) {
        emit scriptEnding();
    }

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

    if (_wantSignals) {
        emit finished(_fileNameString);
    }

    _isRunning = false;
    if (_wantSignals) {
        emit runningStateChanged();
        emit doneRunning();
    }

    _doneRunningThisScript = true;
}

// NOTE: This is private because it must be called on the same thread that created the timers, which is why
// we want to only call it in our own run "shutdown" processing.
void ScriptEngine::stopAllTimers() {
    QMutableHashIterator<QTimer*, QScriptValue> i(_timerFunctionMap);
    while (i.hasNext()) {
        i.next();
        QTimer* timer = i.key();
        stopTimer(timer);
    }
}

void ScriptEngine::stop() {
    if (!_isFinished) {
        _isFinished = true;
        if (_wantSignals) {
            emit runningStateChanged();
        }
    }
}

void ScriptEngine::timerFired() {
    QTimer* callingTimer = reinterpret_cast<QTimer*>(sender());
    QScriptValue timerFunction = _timerFunctionMap.value(callingTimer);

    if (!callingTimer->isActive()) {
        // this timer is done, we can kill it
        _timerFunctionMap.remove(callingTimer);
        delete callingTimer;
    }

    // call the associated JS function, if it exists
    if (timerFunction.isValid()) {
        timerFunction.call();
    }
}

QObject* ScriptEngine::setupTimerWithInterval(const QScriptValue& function, int intervalMS, bool isSingleShot) {
    // create the timer, add it to the map, and start it
    QTimer* newTimer = new QTimer(this);
    newTimer->setSingleShot(isSingleShot);

    connect(newTimer, &QTimer::timeout, this, &ScriptEngine::timerFired);

    // make sure the timer stops when the script does
    connect(this, &ScriptEngine::scriptEnding, newTimer, &QTimer::stop);

    _timerFunctionMap.insert(newTimer, function);

    newTimer->start(intervalMS);
    return newTimer;
}

QObject* ScriptEngine::setInterval(const QScriptValue& function, int intervalMS) {
    if (_stoppingAllScripts) {
        qCDebug(scriptengine) << "Script.setInterval() while shutting down is ignored... parent script:" << getFilename();
        return NULL; // bail early
    }

    return setupTimerWithInterval(function, intervalMS, false);
}

QObject* ScriptEngine::setTimeout(const QScriptValue& function, int timeoutMS) {
    if (_stoppingAllScripts) {
        qCDebug(scriptengine) << "Script.setTimeout() while shutting down is ignored... parent script:" << getFilename();
        return NULL; // bail early
    }

    return setupTimerWithInterval(function, timeoutMS, true);
}

void ScriptEngine::stopTimer(QTimer *timer) {
    if (_timerFunctionMap.contains(timer)) {
        timer->stop();
        _timerFunctionMap.remove(timer);
        delete timer;
    }
}

QUrl ScriptEngine::resolvePath(const QString& include) const {
    QUrl url(include);
    // first lets check to see if it's already a full URL
    if (!url.scheme().isEmpty()) {
        return url;
    }

    // we apparently weren't a fully qualified url, so, let's assume we're relative
    // to the original URL of our script
    QUrl parentURL;
    if (_parentURL.isEmpty()) {
        parentURL = QUrl(_fileNameString);
    } else {
        parentURL = QUrl(_parentURL);
    }
    // if the parent URL's scheme is empty, then this is probably a local file...
    if (parentURL.scheme().isEmpty()) {
        parentURL = QUrl::fromLocalFile(_fileNameString);
    }

    // at this point we should have a legitimate fully qualified URL for our parent
    url = parentURL.resolved(url);
    return url;
}

void ScriptEngine::print(const QString& message) {
    if (_wantSignals) {
        emit printedMessage(message);
    }
}

// If a callback is specified, the included files will be loaded asynchronously and the callback will be called
// when all of the files have finished loading.
// If no callback is specified, the included files will be loaded synchronously and will block execution until
// all of the files have finished loading.
void ScriptEngine::include(const QStringList& includeFiles, QScriptValue callback) {
    if (_stoppingAllScripts) {
        qCDebug(scriptengine) << "Script.include() while shutting down is ignored..."
                 << "includeFiles:" << includeFiles << "parent script:" << getFilename();
        return; // bail early
    }
    QList<QUrl> urls;
    for (QString file : includeFiles) {
        QUrl thisURL { resolvePath(file) };
        if (!_includedURLs.contains(thisURL)) {
            urls.append(thisURL);
            _includedURLs << thisURL;
        }
        else {
            qCDebug(scriptengine) << "Script.include() ignoring previously included url:" << thisURL;
        }
    }

    BatchLoader* loader = new BatchLoader(urls);

    auto evaluateScripts = [=](const QMap<QUrl, QString>& data) {
        for (QUrl url : urls) {
            QString contents = data[url];
            if (contents.isNull()) {
                qCDebug(scriptengine) << "Error loading file: " << url << "line:" << __LINE__;
            } else {
                QScriptValue result = evaluate(contents, url.toString());
            }
        }

        if (callback.isFunction()) {
            QScriptValue(callback).call();
        }

        loader->deleteLater();
    };

    connect(loader, &BatchLoader::finished, this, evaluateScripts);

    // If we are destroyed before the loader completes, make sure to clean it up
    connect(this, &QObject::destroyed, loader, &QObject::deleteLater);

    loader->start();

    if (!callback.isFunction() && !loader->isFinished()) {
        QEventLoop loop;
        QObject::connect(loader, &BatchLoader::finished, &loop, &QEventLoop::quit);
        loop.exec();
    }
}

void ScriptEngine::include(const QString& includeFile, QScriptValue callback) {
    if (_stoppingAllScripts) {
        qCDebug(scriptengine) << "Script.include() while shutting down is ignored... "
                 << "includeFile:" << includeFile << "parent script:" << getFilename();
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
    if (_stoppingAllScripts) {
        qCDebug(scriptengine) << "Script.load() while shutting down is ignored... "
                 << "loadFile:" << loadFile << "parent script:" << getFilename();
        return; // bail early
    }

    QUrl url = resolvePath(loadFile);
    if (_isReloading) {
        auto scriptCache = DependencyManager::get<ScriptCache>();
        scriptCache->deleteScript(url.toString());
        if (_wantSignals) {
            emit reloadScript(url.toString(), false);
        }
    } else {
        if (_wantSignals) {
            emit loadScript(url.toString(), false);
        }
    }
}

// Look up the handler associated with eventName and entityID. If found, evalute the argGenerator thunk and call the handler with those args
void ScriptEngine::generalHandler(const EntityItemID& entityID, const QString& eventName, std::function<QScriptValueList()> argGenerator) {
    if (QThread::currentThread() != thread()) {
        qDebug() << "*** ERROR *** ScriptEngine::generalHandler() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "]";
        assert(false);
        return;
    }

    if (!_registeredHandlers.contains(entityID)) {
        return;
    }
    const RegisteredEventHandlers& handlersOnEntity = _registeredHandlers[entityID];
    if (!handlersOnEntity.contains(eventName)) {
        return;
    }
    QScriptValueList handlersForEvent = handlersOnEntity[eventName];
    if (!handlersForEvent.isEmpty()) {
        QScriptValueList args = argGenerator();
        for (int i = 0; i < handlersForEvent.count(); ++i) {
            handlersForEvent[i].call(QScriptValue(), args);
        }
    }
}

// since all of these operations can be asynch we will always do the actual work in the response handler
// for the download
void ScriptEngine::loadEntityScript(const EntityItemID& entityID, const QString& entityScript, bool forceRedownload) {
    if (QThread::currentThread() != thread()) {
        #ifdef THREAD_DEBUGGING
        qDebug() << "*** WARNING *** ScriptEngine::loadEntityScript() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "]  "
            "entityID:" << entityID << "entityScript:" << entityScript <<"forceRedownload:" << forceRedownload;
        #endif

        QMetaObject::invokeMethod(this, "loadEntityScript",
            Q_ARG(const EntityItemID&, entityID),
            Q_ARG(const QString&, entityScript),
            Q_ARG(bool, forceRedownload));
        return;
    }
    #ifdef THREAD_DEBUGGING
    qDebug() << "ScriptEngine::loadEntityScript() called on correct thread [" << thread() << "]  "
        "entityID:" << entityID << "entityScript:" << entityScript << "forceRedownload:" << forceRedownload;
    #endif

    // If we've been called our known entityScripts should not know about us..
    assert(!_entityScripts.contains(entityID));

    #ifdef THREAD_DEBUGGING
    qDebug() << "ScriptEngine::loadEntityScript() calling scriptCache->getScriptContents() on thread [" << QThread::currentThread() << "] expected thread [" << thread() << "]";
    #endif
    DependencyManager::get<ScriptCache>()->getScriptContents(entityScript, [=](const QString& scriptOrURL, const QString& contents, bool isURL, bool success) {
            #ifdef THREAD_DEBUGGING
            qDebug() << "ScriptEngine::entityScriptContentAvailable() IN LAMBDA contentAvailable on thread [" << QThread::currentThread() << "] expected thread [" << thread() << "]";
            #endif

            this->entityScriptContentAvailable(entityID, scriptOrURL, contents, isURL, success);
        }, forceRedownload);
}

// since all of these operations can be asynch we will always do the actual work in the response handler
// for the download
void ScriptEngine::entityScriptContentAvailable(const EntityItemID& entityID, const QString& scriptOrURL, const QString& contents, bool isURL, bool success) {
    if (QThread::currentThread() != thread()) {
        #ifdef THREAD_DEBUGGING
        qDebug() << "*** WARNING *** ScriptEngine::entityScriptContentAvailable() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "]  "
            "entityID:" << entityID << "scriptOrURL:" << scriptOrURL << "contents:" << contents << "isURL:" << isURL << "success:" << success;
        #endif

        QMetaObject::invokeMethod(this, "entityScriptContentAvailable",
            Q_ARG(const EntityItemID&, entityID),
            Q_ARG(const QString&, scriptOrURL),
            Q_ARG(const QString&, contents),
            Q_ARG(bool, isURL),
            Q_ARG(bool, success));
        return;
    }

    #ifdef THREAD_DEBUGGING
    qDebug() << "ScriptEngine::entityScriptContentAvailable() thread [" << QThread::currentThread() << "] expected thread [" << thread() << "]";
    #endif

    auto scriptCache = DependencyManager::get<ScriptCache>();
    bool isFileUrl = isURL && scriptOrURL.startsWith("file://");

    // first check the syntax of the script contents
    QScriptSyntaxCheckResult syntaxCheck = QScriptEngine::checkSyntax(contents);
    if (syntaxCheck.state() != QScriptSyntaxCheckResult::Valid) {
        qCDebug(scriptengine) << "ScriptEngine::loadEntityScript() entity:" << entityID;
        qCDebug(scriptengine) << "   " << syntaxCheck.errorMessage() << ":"
                << syntaxCheck.errorLineNumber() << syntaxCheck.errorColumnNumber();
        qCDebug(scriptengine) << "    SCRIPT:" << scriptOrURL;
        if (!isFileUrl) {
            scriptCache->addScriptToBadScriptList(scriptOrURL);
        }
        return; // done processing script
    }

    if (isURL) {
        setParentURL(scriptOrURL);
    }

    QScriptEngine sandbox;
    QScriptValue testConstructor = sandbox.evaluate(contents);

    if (!testConstructor.isFunction()) {
        qCDebug(scriptengine) << "ScriptEngine::loadEntityScript() entity:" << entityID;
        qCDebug(scriptengine) << "    NOT CONSTRUCTOR";
        qCDebug(scriptengine) << "    SCRIPT:" << scriptOrURL;
        if (!isFileUrl) {
            scriptCache->addScriptToBadScriptList(scriptOrURL);
        }
        return; // done processing script
    }

    int64_t lastModified = 0;
    if (isFileUrl) {
        QString file = QUrl(scriptOrURL).toLocalFile();
        lastModified = (quint64)QFileInfo(file).lastModified().toMSecsSinceEpoch();
    }

    QScriptValue entityScriptConstructor = evaluate(contents);
    QScriptValue entityScriptObject = entityScriptConstructor.construct();
    EntityScriptDetails newDetails = { scriptOrURL, entityScriptObject, lastModified };
    _entityScripts[entityID] = newDetails;
    if (isURL) {
        setParentURL("");
    }

    // if we got this far, then call the preload method
    callEntityScriptMethod(entityID, "preload");
}

void ScriptEngine::unloadEntityScript(const EntityItemID& entityID) {
    if (QThread::currentThread() != thread()) {
        #ifdef THREAD_DEBUGGING
        qDebug() << "*** WARNING *** ScriptEngine::unloadEntityScript() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "]  "
            "entityID:" << entityID;
        #endif

        QMetaObject::invokeMethod(this, "unloadEntityScript",
            Q_ARG(const EntityItemID&, entityID));
        return;
    }
    #ifdef THREAD_DEBUGGING
    qDebug() << "ScriptEngine::unloadEntityScript() called on correct thread [" << thread() << "]  "
        "entityID:" << entityID;
    #endif

    if (_entityScripts.contains(entityID)) {
        callEntityScriptMethod(entityID, "unload");
        _entityScripts.remove(entityID);
    }
}

void ScriptEngine::unloadAllEntityScripts() {
    if (QThread::currentThread() != thread()) {
#ifdef THREAD_DEBUGGING
        qDebug() << "*** WARNING *** ScriptEngine::unloadAllEntityScripts() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "]";
#endif

        QMetaObject::invokeMethod(this, "unloadAllEntityScripts");
        return;
    }
#ifdef THREAD_DEBUGGING
    qDebug() << "ScriptEngine::unloadAllEntityScripts() called on correct thread [" << thread() << "]";
#endif
    foreach(const EntityItemID& entityID, _entityScripts.keys()) {
        callEntityScriptMethod(entityID, "unload");
    }
    _entityScripts.clear();
}

void ScriptEngine::refreshFileScript(const EntityItemID& entityID) {
    if (!_entityScripts.contains(entityID)) {
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
            qDebug() << "Reloading modified script " << details.scriptText;

            QFile file(filePath);
            file.open(QIODevice::ReadOnly);
            QString scriptContents = QTextStream(&file).readAll();
            this->unloadEntityScript(entityID);
            this->entityScriptContentAvailable(entityID, details.scriptText, scriptContents, true, true);
            if (!_entityScripts.contains(entityID)) {
                qWarning() << "Reload script " << details.scriptText << " failed";
            } else {
                details = _entityScripts[entityID];
            }
        }
    }
    recurseGuard = false;
}


void ScriptEngine::callEntityScriptMethod(const EntityItemID& entityID, const QString& methodName) {
    if (QThread::currentThread() != thread()) {
        #ifdef THREAD_DEBUGGING
        qDebug() << "*** WARNING *** ScriptEngine::callEntityScriptMethod() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "]  "
            "entityID:" << entityID << "methodName:" << methodName;
        #endif

        QMetaObject::invokeMethod(this, "callEntityScriptMethod",
            Q_ARG(const EntityItemID&, entityID),
            Q_ARG(const QString&, methodName));
        return;
    }
    #ifdef THREAD_DEBUGGING
    qDebug() << "ScriptEngine::callEntityScriptMethod() called on correct thread [" << thread() << "]  "
        "entityID:" << entityID << "methodName:" << methodName;
    #endif

    refreshFileScript(entityID);
    if (_entityScripts.contains(entityID)) {
        EntityScriptDetails details = _entityScripts[entityID];
        QScriptValue entityScript = details.scriptObject; // previously loaded
        if (entityScript.property(methodName).isFunction()) {
            QScriptValueList args;
            args << entityID.toScriptValue(this);
            entityScript.property(methodName).call(entityScript, args);
        }

    }
}

void ScriptEngine::callEntityScriptMethod(const EntityItemID& entityID, const QString& methodName, const MouseEvent& event) {
    if (QThread::currentThread() != thread()) {
        #ifdef THREAD_DEBUGGING
        qDebug() << "*** WARNING *** ScriptEngine::callEntityScriptMethod() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "]  "
            "entityID:" << entityID << "methodName:" << methodName << "event: mouseEvent";
        #endif

        QMetaObject::invokeMethod(this, "callEntityScriptMethod",
            Q_ARG(const EntityItemID&, entityID),
            Q_ARG(const QString&, methodName),
            Q_ARG(const MouseEvent&, event));
        return;
    }
    #ifdef THREAD_DEBUGGING
    qDebug() << "ScriptEngine::callEntityScriptMethod() called on correct thread [" << thread() << "]  "
        "entityID:" << entityID << "methodName:" << methodName << "event: mouseEvent";
    #endif

    refreshFileScript(entityID);
    if (_entityScripts.contains(entityID)) {
        EntityScriptDetails details = _entityScripts[entityID];
        QScriptValue entityScript = details.scriptObject; // previously loaded
        if (entityScript.property(methodName).isFunction()) {
            QScriptValueList args;
            args << entityID.toScriptValue(this);
            args << event.toScriptValue(this);
            entityScript.property(methodName).call(entityScript, args);
        }
    }
}


void ScriptEngine::callEntityScriptMethod(const EntityItemID& entityID, const QString& methodName, const EntityItemID& otherID, const Collision& collision) {
    if (QThread::currentThread() != thread()) {
        #ifdef THREAD_DEBUGGING
        qDebug() << "*** WARNING *** ScriptEngine::callEntityScriptMethod() called on wrong thread [" << QThread::currentThread() << "], invoking on correct thread [" << thread() << "]  "
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
    qDebug() << "ScriptEngine::callEntityScriptMethod() called on correct thread [" << thread() << "]  "
        "entityID:" << entityID << "methodName:" << methodName << "otherID:" << otherID << "collision: collision";
    #endif

    refreshFileScript(entityID);
    if (_entityScripts.contains(entityID)) {
        EntityScriptDetails details = _entityScripts[entityID];
        QScriptValue entityScript = details.scriptObject; // previously loaded
        if (entityScript.property(methodName).isFunction()) {
            QScriptValueList args;
            args << entityID.toScriptValue(this);
            args << otherID.toScriptValue(this);
            args << collisionToScriptValue(this, collision);
            entityScript.property(methodName).call(entityScript, args);
        }
    }
}
