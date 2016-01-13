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
    Q_PROPERTY(QString previousScriptLocation READ getPreviousScriptLocation WRITE setPreviousScriptLocation NOTIFY previousScriptLocationChanged)

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
    ScriptEngine* getScriptEngine(const QString& scriptHash);

    QString getPreviousScriptLocation() const;
    void setPreviousScriptLocation(const QString& previousScriptLocation);

    ScriptsModel* scriptsModel() { return &_scriptsModel; };
    ScriptsModelFilter* scriptsModelFilter() { return &_scriptsModelFilter; };

    Q_INVOKABLE void loadOneScript(const QString& scriptFilename);
    Q_INVOKABLE ScriptEngine* loadScript(const QString& scriptFilename = QString(),
        bool isUserLoaded = true, bool loadScriptFromEditor = false, bool activateMainWindow = false, bool reload = false);
    Q_INVOKABLE bool stopScript(const QString& scriptHash, bool restart = false);

    Q_INVOKABLE void reloadAllScripts();
    Q_INVOKABLE void stopAllScripts(bool restart = false);

    Q_INVOKABLE QVariantList getRunning();
    Q_INVOKABLE QVariantList getPublic();
    Q_INVOKABLE QVariantList getLocal();

    // Called at shutdown time
    void shutdownScripting();

signals: 
    void scriptCountChanged();
    void scriptsReloading();
    void scriptLoadError(const QString& filename, const QString& error);
    void previousScriptLocationChanged();

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


    Setting::Handle<bool> _firstRun { "firstRun", true };
    QReadWriteLock _scriptEnginesHashLock;
    QHash<QString, ScriptEngine*> _scriptEnginesHash;
    QSet<ScriptEngine*> _allKnownScriptEngines;
    QMutex _allScriptsMutex;
    std::atomic<bool> _stoppingAllScripts { false };
    std::list<ScriptInitializer> _scriptInitializers;
    mutable Setting::Handle<QString> _scriptsLocationHandle;
    mutable Setting::Handle<QString> _previousScriptLocation;
    ScriptsModel _scriptsModel;
    ScriptsModelFilter _scriptsModelFilter;
};

#endif // hifi_ScriptEngine_h
