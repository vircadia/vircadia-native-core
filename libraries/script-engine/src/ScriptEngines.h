//
//  Created by Bradley Austin Davis on 2016/01/08
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptEngines_h
#define hifi_ScriptEngines_h

#include <functional>
#include <atomic>
#include <memory>

#include <QtCore/QObject>
#include <QtCore/QMutex>
#include <QtCore/QReadWriteLock>

#include <SettingHandle.h>
#include <DependencyManager.h>

#include "ScriptsModel.h"
#include "ScriptsModelFilter.h"

class ScriptEngine;

class ScriptEngines : public QObject, public Dependency {
    Q_OBJECT

    Q_PROPERTY(ScriptsModel* scriptsModel READ scriptsModel CONSTANT)
    Q_PROPERTY(ScriptsModelFilter* scriptsModelFilter READ scriptsModelFilter CONSTANT)

public:
    using ScriptInitializer = std::function<void(ScriptEngine*)>;

    ScriptEngines();
    void registerScriptInitializer(ScriptInitializer initializer);

    void loadScripts();
    void saveScripts();
    void clearScripts();

    QString getScriptsLocation() const;
    void loadDefaultScripts();
    void setScriptsLocation(const QString& scriptsLocation);
    QStringList getRunningScripts();
    ScriptEngine* getScriptEngine(const QUrl& scriptHash);

    ScriptsModel* scriptsModel() { return &_scriptsModel; };
    ScriptsModelFilter* scriptsModelFilter() { return &_scriptsModelFilter; };

    QString getDefaultScriptsLocation() const;

    Q_INVOKABLE void loadOneScript(const QString& scriptFilename);
    Q_INVOKABLE ScriptEngine* loadScript(const QUrl& scriptFilename = QString(),
        bool isUserLoaded = true, bool loadScriptFromEditor = false, bool activateMainWindow = false, bool reload = false);
    Q_INVOKABLE bool stopScript(const QString& scriptHash, bool restart = false);

    Q_INVOKABLE void reloadAllScripts();
    Q_INVOKABLE void stopAllScripts(bool restart = false);

    Q_INVOKABLE QVariantList getRunning();
    Q_INVOKABLE QVariantList getPublic();
    Q_INVOKABLE QVariantList getLocal();

    Q_PROPERTY(QString defaultScriptsPath READ getDefaultScriptsLocation)

    // Called at shutdown time
    void shutdownScripting();
    bool isStopped() const { return _isStopped; }

signals:
    void scriptCountChanged();
    void scriptsReloading();
    void scriptLoadError(const QString& filename, const QString& error);

protected slots:
    void onScriptFinished(const QString& fileNameString, ScriptEngine* engine);

protected:
    friend class ScriptEngine;

    void reloadScript(const QString& scriptName) { loadScript(scriptName, true, false, false, true); }
    void addScriptEngine(ScriptEngine* engine);
    void removeScriptEngine(ScriptEngine* engine);
    void onScriptEngineLoaded(const QString& scriptFilename);
    void onScriptEngineError(const QString& scriptFilename);
    void launchScriptEngine(ScriptEngine* engine);

    QReadWriteLock _scriptEnginesHashLock;
    QHash<QUrl, ScriptEngine*> _scriptEnginesHash;
    QSet<ScriptEngine*> _allKnownScriptEngines;
    QMutex _allScriptsMutex;
    std::list<ScriptInitializer> _scriptInitializers;
    mutable Setting::Handle<QString> _scriptsLocationHandle;
    ScriptsModel _scriptsModel;
    ScriptsModelFilter _scriptsModelFilter;
    std::atomic<bool> _isStopped { false };
};

QUrl normalizeScriptURL(const QUrl& rawScriptURL);
QString expandScriptPath(const QString& rawPath);
QUrl expandScriptUrl(const QUrl& rawScriptURL);

#endif // hifi_ScriptEngine_h
