//
//  Created by Bradley Austin Davis on 2016/01/08
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptEngines.h"

#include <QtCore/QStandardPaths>
#include <QtCore/QCoreApplication>

#include <SettingHandle.h>
#include <UserActivityLogger.h>
#include <PathUtils.h>

#include "ScriptEngine.h"
#include "ScriptEngineLogging.h"

#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC__ __FILE__ "(" __STR1__(__LINE__) ") : Warning Msg: "

#ifndef __APPLE__
static const QString DESKTOP_LOCATION = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
#else
// Temporary fix to Qt bug: http://stackoverflow.com/questions/16194475
static const QString DESKTOP_LOCATION = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation).append("/script.js");
#endif

ScriptsModel& getScriptsModel() {
    static ScriptsModel scriptsModel;
    return scriptsModel;
}

ScriptEngines::ScriptEngines()
    : _scriptsLocationHandle("scriptsLocation", DESKTOP_LOCATION)
{
    _scriptsModelFilter.setSourceModel(&_scriptsModel);
    _scriptsModelFilter.sort(0, Qt::AscendingOrder);
    _scriptsModelFilter.setDynamicSortFilter(true);

    static const int SCRIPT_SAVE_COUNTDOWN_INTERVAL_MS = 5000;
    QTimer* scriptSaveTimer = new QTimer(this);
    scriptSaveTimer->setSingleShot(true);
    QMetaObject::Connection timerConnection = connect(scriptSaveTimer, &QTimer::timeout, [] {
        DependencyManager::get<ScriptEngines>()->saveScripts();
    });
    connect(qApp, &QCoreApplication::aboutToQuit, [=] {
        disconnect(timerConnection);
    });
    connect(this, &ScriptEngines::scriptCountChanged, this, [scriptSaveTimer] {
        scriptSaveTimer->start(SCRIPT_SAVE_COUNTDOWN_INTERVAL_MS);
    }, Qt::QueuedConnection);
}

QUrl normalizeScriptURL(const QUrl& rawScriptURL) {
    if (rawScriptURL.scheme() == "file") {
        QUrl fullNormal = rawScriptURL;
        QUrl defaultScriptLoc = defaultScriptsLocation();

        // if this url is something "beneath" the default script url, replace the local path with ~
        if (fullNormal.scheme() == defaultScriptLoc.scheme() &&
            fullNormal.host() == defaultScriptLoc.host() &&
            fullNormal.path().startsWith(defaultScriptLoc.path())) {
            fullNormal.setPath("/~/" + fullNormal.path().mid(defaultScriptLoc.path().size()));
        }
        return fullNormal;
    } else if (rawScriptURL.scheme() == "http" || rawScriptURL.scheme() == "https" || rawScriptURL.scheme() == "atp") {
        return rawScriptURL;
    } else {
        // don't accidently support gopher
        return QUrl("");
    }
}

QUrl expandScriptUrl(const QUrl& rawScriptURL) {
    QUrl normalizedScriptURL = normalizeScriptURL(rawScriptURL);
    if (normalizedScriptURL.scheme() == "http" ||
        normalizedScriptURL.scheme() == "https" ||
        normalizedScriptURL.scheme() == "atp") {
        return normalizedScriptURL;
    } else if (normalizedScriptURL.scheme() == "file") {
        if (normalizedScriptURL.path().startsWith("/~/")) {
            QUrl url = normalizedScriptURL;
            QStringList splitPath = url.path().split("/");
            QUrl defaultScriptsLoc = defaultScriptsLocation();
            url.setPath(defaultScriptsLoc.path() + "/" + splitPath.mid(2).join("/")); // 2 to skip the slashes in /~/
            return url;
        }
        return normalizedScriptURL;
    } else {
        return QUrl("");
    }
}


QObject* scriptsModel();

void ScriptEngines::registerScriptInitializer(ScriptInitializer initializer) {
    _scriptInitializers.push_back(initializer);
}

void ScriptEngines::addScriptEngine(ScriptEngine* engine) {
    _allScriptsMutex.lock();
    _allKnownScriptEngines.insert(engine);
    _allScriptsMutex.unlock();
}

void ScriptEngines::removeScriptEngine(ScriptEngine* engine) {
    // If we're not already in the middle of stopping all scripts, then we should remove ourselves
    // from the list of running scripts. We don't do this if we're in the process of stopping all scripts
    // because that method removes scripts from its list as it iterates them
    if (!_stoppingAllScripts) {
        _allScriptsMutex.lock();
        _allKnownScriptEngines.remove(engine);
        _allScriptsMutex.unlock();
    }
}

void ScriptEngines::shutdownScripting() {
    _allScriptsMutex.lock();
    _stoppingAllScripts = true;
    ScriptEngine::_stoppingAllScripts = true;
    qCDebug(scriptengine) << "Stopping all scripts.... currently known scripts:" << _allKnownScriptEngines.size();

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
            scriptEngine->disconnect(this);

            // Calling stop on the script engine will set it's internal _isFinished state to true, and result
            // in the ScriptEngine gracefully ending it's run() method.
            scriptEngine->stop();

            // We need to wait for the engine to be done running before we proceed, because we don't
            // want any of the scripts final "scriptEnding()" or pending "update()" methods from accessing
            // any application state after we leave this stopAllScripts() method
            qCDebug(scriptengine) << "waiting on script:" << scriptName;
            scriptEngine->waitTillDoneRunning();
            qCDebug(scriptengine) << "done waiting on script:" << scriptName;

            scriptEngine->deleteLater();

            // If the script is stopped, we can remove it from our set
            i.remove();
        }
    }
    _stoppingAllScripts = false;
    _allScriptsMutex.unlock();
    qCDebug(scriptengine) << "DONE Stopping all scripts....";
}

QVariantList getPublicChildNodes(TreeNodeFolder* parent) {
    QVariantList result;
    QList<TreeNodeBase*> treeNodes = getScriptsModel().getFolderNodes(parent);
    for (int i = 0; i < treeNodes.size(); i++) {
        TreeNodeBase* node = treeNodes.at(i);
        if (node->getType() == TREE_NODE_TYPE_FOLDER) {
            TreeNodeFolder* folder = static_cast<TreeNodeFolder*>(node);
            QVariantMap resultNode;
            resultNode.insert("name", node->getName());
            resultNode.insert("type", "folder");
            resultNode.insert("children", getPublicChildNodes(folder));
            result.append(resultNode);
            continue;
        }
        TreeNodeScript* script = static_cast<TreeNodeScript*>(node);
        if (script->getOrigin() == ScriptOrigin::SCRIPT_ORIGIN_LOCAL) {
            continue;
        }
        QVariantMap resultNode;
        resultNode.insert("name", node->getName());
        resultNode.insert("type", "script");
        resultNode.insert("url", script->getFullPath());
        result.append(resultNode);
    }
    return result;
}

QVariantList ScriptEngines::getPublic() {
    return getPublicChildNodes(NULL);
}

QVariantList ScriptEngines::getLocal() {
    QVariantList result;
    QList<TreeNodeBase*> treeNodes = getScriptsModel().getFolderNodes(NULL);
    for (int i = 0; i < treeNodes.size(); i++) {
        TreeNodeBase* node = treeNodes.at(i);
        if (node->getType() != TREE_NODE_TYPE_SCRIPT) {
            continue;
        }
        TreeNodeScript* script = static_cast<TreeNodeScript*>(node);
        if (script->getOrigin() != ScriptOrigin::SCRIPT_ORIGIN_LOCAL) {
            continue;
        }
        QVariantMap resultNode;
        resultNode.insert("name", node->getName());
        resultNode.insert("path", script->getFullPath());
        result.append(resultNode);
    }
    return result;
}

QVariantList ScriptEngines::getRunning() {
    QVariantList result;
    auto runningScripts = getRunningScripts();
    foreach(const QString& runningScript, runningScripts) {
        QUrl runningScriptURL = QUrl(runningScript);
        if (!runningScriptURL.isValid()) {
            runningScriptURL = QUrl::fromLocalFile(runningScriptURL.toDisplayString(QUrl::FormattingOptions(QUrl::FullyEncoded)));
        }
        QVariantMap resultNode;
        resultNode.insert("name", runningScriptURL.fileName());
        QUrl displayURL = expandScriptUrl(runningScriptURL);
        QString displayURLString;
        if (displayURL.isLocalFile()) {
            displayURLString = displayURL.toLocalFile();
        } else {
            displayURLString = displayURL.toDisplayString(QUrl::FormattingOptions(QUrl::FullyEncoded));
        }
        resultNode.insert("url", displayURLString);
        // The path contains the exact path/URL of the script, which also is used in the stopScript function.
        resultNode.insert("path", normalizeScriptURL(runningScript).toString());
        resultNode.insert("local", runningScriptURL.isLocalFile());
        result.append(resultNode);
    }
    return result;
}


static const QString SETTINGS_KEY = "Settings";

void ScriptEngines::loadDefaultScripts() {
    QUrl defaultScriptsLoc = defaultScriptsLocation();
    defaultScriptsLoc.setPath(defaultScriptsLoc.path() + "/defaultScripts.js");
    loadScript(defaultScriptsLoc.toString());
}

void ScriptEngines::loadOneScript(const QString& scriptFilename) {
    loadScript(scriptFilename);
}

void ScriptEngines::loadScripts() {
    // check first run...
    if (_firstRun.get()) {
        qCDebug(scriptengine) << "This is a first run...";
        // clear the scripts, and set out script to our default scripts
        clearScripts();
        loadDefaultScripts();
        _firstRun.set(false);
        return;
    }

    // loads all saved scripts
    Settings settings;
    int size = settings.beginReadArray(SETTINGS_KEY);
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        QString string = settings.value("script").toString();
        if (!string.isEmpty()) {
            loadScript(string);
        }
    }
    settings.endArray();
}

void ScriptEngines::clearScripts() {
    // clears all scripts from the settingsSettings settings;
    Settings settings;
    settings.beginWriteArray(SETTINGS_KEY);
    settings.remove("");
}

void ScriptEngines::saveScripts() {
    // Saves all currently running user-loaded scripts
    Settings settings;
    settings.beginWriteArray(SETTINGS_KEY);
    settings.remove("");

    QStringList runningScripts = getRunningScripts();
    int i = 0;
    for (auto it = runningScripts.begin(); it != runningScripts.end(); ++it) {
        if (getScriptEngine(*it)->isUserLoaded()) {
            settings.setArrayIndex(i);
            settings.setValue("script", normalizeScriptURL(*it).toString());
            ++i;
        }
    }
    settings.endArray();
}

QStringList ScriptEngines::getRunningScripts() {
    QReadLocker lock(&_scriptEnginesHashLock);
    QList<QUrl> urls = _scriptEnginesHash.keys();
    QStringList result;
    for (auto url : urls) {
        result.append(url.toString());
    }
    return result;
}

void ScriptEngines::stopAllScripts(bool restart) {
    QReadLocker lock(&_scriptEnginesHashLock);
    if (restart) {
        // Delete all running scripts from cache so that they are re-downloaded when they are restarted
        auto scriptCache = DependencyManager::get<ScriptCache>();
        for (QHash<QUrl, ScriptEngine*>::const_iterator it = _scriptEnginesHash.constBegin();
            it != _scriptEnginesHash.constEnd(); it++) {
            if (!it.value()->isFinished()) {
                scriptCache->deleteScript(it.key());
            }
        }
    }

    // Stop and possibly restart all currently running scripts
    for (QHash<QUrl, ScriptEngine*>::const_iterator it = _scriptEnginesHash.constBegin();
        it != _scriptEnginesHash.constEnd(); it++) {
        if (it.value()->isFinished()) {
            continue;
        }
        if (restart && it.value()->isUserLoaded()) {
            connect(it.value(), &ScriptEngine::finished, this, [this](QString scriptName, ScriptEngine* engine) {
                reloadScript(scriptName);
            });
        }
        QMetaObject::invokeMethod(it.value(), "stop");
        //it.value()->stop();
        qCDebug(scriptengine) << "stopping script..." << it.key();
    }
}

bool ScriptEngines::stopScript(const QString& rawScriptURL, bool restart) {
    bool stoppedScript = false;
    {
        QUrl scriptURL = normalizeScriptURL(QUrl(rawScriptURL));
        if (!scriptURL.isValid()) {
            scriptURL = normalizeScriptURL(QUrl::fromLocalFile(rawScriptURL));
        }

        QReadLocker lock(&_scriptEnginesHashLock);
        if (_scriptEnginesHash.contains(scriptURL)) {
            ScriptEngine* scriptEngine = _scriptEnginesHash[scriptURL];
            if (restart) {
                auto scriptCache = DependencyManager::get<ScriptCache>();
                scriptCache->deleteScript(scriptURL);
                connect(scriptEngine, &ScriptEngine::finished, this, [this](QString scriptName, ScriptEngine* engine) {
                    reloadScript(scriptName);
                });
            }
            scriptEngine->stop();
            stoppedScript = true;
            qCDebug(scriptengine) << "stopping script..." << scriptURL;
        }
    }
    return stoppedScript;
}

QString ScriptEngines::getScriptsLocation() const {
    return _scriptsLocationHandle.get();
}

void ScriptEngines::setScriptsLocation(const QString& scriptsLocation) {
    _scriptsLocationHandle.set(scriptsLocation);
    _scriptsModel.updateScriptsLocation(scriptsLocation);
}

void ScriptEngines::reloadAllScripts() {
    DependencyManager::get<ScriptCache>()->clearCache();
    emit scriptsReloading();
    stopAllScripts(true);
}

ScriptEngine* ScriptEngines::loadScript(const QUrl& scriptFilename, bool isUserLoaded, bool loadScriptFromEditor,
                                        bool activateMainWindow, bool reload) {
    if (thread() != QThread::currentThread()) {
        ScriptEngine* result { nullptr };
        QMetaObject::invokeMethod(this, "loadScript", Qt::BlockingQueuedConnection, Q_RETURN_ARG(ScriptEngine*, result),
            Q_ARG(QUrl, scriptFilename),
            Q_ARG(bool, isUserLoaded),
            Q_ARG(bool, loadScriptFromEditor),
            Q_ARG(bool, activateMainWindow),
            Q_ARG(bool, reload));
        return result;
    }
    QUrl scriptUrl;
    if (!scriptFilename.isValid() ||
        (scriptFilename.scheme() != "http" &&
         scriptFilename.scheme() != "https" &&
         scriptFilename.scheme() != "atp" &&
         scriptFilename.scheme() != "file")) {
        // deal with a "url" like c:/something
        scriptUrl = normalizeScriptURL(QUrl::fromLocalFile(scriptFilename.toString()));
    } else {
        scriptUrl = normalizeScriptURL(scriptFilename);
    }

    auto scriptEngine = getScriptEngine(scriptUrl);
    if (scriptEngine) {
        return scriptEngine;
    }

    scriptEngine = new ScriptEngine(NO_SCRIPT, "", true);
    scriptEngine->setUserLoaded(isUserLoaded);
    connect(scriptEngine, &ScriptEngine::doneRunning, this, [scriptEngine] {
        scriptEngine->deleteLater();
    }, Qt::QueuedConnection);


    if (scriptFilename.isEmpty()) {
        launchScriptEngine(scriptEngine);
    } else {
        // connect to the appropriate signals of this script engine
        connect(scriptEngine, &ScriptEngine::scriptLoaded, this, &ScriptEngines::onScriptEngineLoaded);
        connect(scriptEngine, &ScriptEngine::errorLoadingScript, this, &ScriptEngines::onScriptEngineError);

        // get the script engine object to load the script at the designated script URL
        scriptEngine->loadURL(scriptUrl, reload);
    }

    return scriptEngine;
}

ScriptEngine* ScriptEngines::getScriptEngine(const QUrl& rawScriptURL) {
    ScriptEngine* result = nullptr;
    {
        QReadLocker lock(&_scriptEnginesHashLock);
        const QUrl scriptURL = normalizeScriptURL(rawScriptURL);
        auto it = _scriptEnginesHash.find(scriptURL);
        if (it != _scriptEnginesHash.end()) {
            result = it.value();
        }
    }
    return result;
}

// FIXME - change to new version of ScriptCache loading notification
void ScriptEngines::onScriptEngineLoaded(const QString& rawScriptURL) {
    UserActivityLogger::getInstance().loadedScript(rawScriptURL);
    ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(sender());

    launchScriptEngine(scriptEngine);

    {
        QWriteLocker lock(&_scriptEnginesHashLock);
        QUrl url = QUrl(rawScriptURL);
        QUrl normalized = normalizeScriptURL(url);
        _scriptEnginesHash.insertMulti(normalized, scriptEngine);
    }
    emit scriptCountChanged();
}

void ScriptEngines::launchScriptEngine(ScriptEngine* scriptEngine) {
    connect(scriptEngine, &ScriptEngine::finished, this, &ScriptEngines::onScriptFinished, Qt::DirectConnection);
    connect(scriptEngine, &ScriptEngine::loadScript, [&](const QString& scriptName, bool userLoaded) {
        loadScript(scriptName, userLoaded);
    });
    connect(scriptEngine, &ScriptEngine::reloadScript, [&](const QString& scriptName, bool userLoaded) {
        loadScript(scriptName, userLoaded, false, false, true);
    });

    // register our application services and set it off on its own thread
    for (auto initializer : _scriptInitializers) {
        initializer(scriptEngine);
    }
    scriptEngine->runInThread();
}


void ScriptEngines::onScriptFinished(const QString& rawScriptURL, ScriptEngine* engine) {
    bool removed = false;
    {
        QWriteLocker lock(&_scriptEnginesHashLock);
        const QUrl scriptURL = normalizeScriptURL(QUrl(rawScriptURL));
        for (auto it = _scriptEnginesHash.find(scriptURL); it != _scriptEnginesHash.end(); ++it) {
            if (it.value() == engine) {
                _scriptEnginesHash.erase(it);
                removed = true;
                break;
            }
        }
    }

    if (removed) {
        emit scriptCountChanged();
    }
}

// FIXME - change to new version of ScriptCache loading notification
void ScriptEngines::onScriptEngineError(const QString& scriptFilename) {
    qCDebug(scriptengine) << "Application::loadScript(), script failed to load...";
    emit scriptLoadError(scriptFilename, "");
}
