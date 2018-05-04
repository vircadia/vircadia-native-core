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
#include <shared/ScriptInitializerMixin.h>

#include "ScriptEngine.h"
#include "ScriptsModel.h"
#include "ScriptsModelFilter.h"

class ScriptEngine;

/**jsdoc
 * @namespace ScriptDiscoveryService
 *
 * @hifi-interface
 * @hifi-client-entity
 *
 * @property {string} debugScriptUrl
 * @property {string} defaultScriptsPath
 * @property {ScriptsModel} scriptsModel
 * @property {ScriptsModelFilter} scriptsModelFilter
 */

class NativeScriptInitializers : public ScriptInitializerMixin {
public:
    bool registerNativeScriptInitializer(NativeScriptInitializer initializer) override;
    bool registerScriptInitializer(ScriptInitializer initializer) override;
};

class ScriptEngines : public QObject, public Dependency {
    Q_OBJECT

    Q_PROPERTY(ScriptsModel* scriptsModel READ scriptsModel CONSTANT)
    Q_PROPERTY(ScriptsModelFilter* scriptsModelFilter READ scriptsModelFilter CONSTANT)
    Q_PROPERTY(QString debugScriptUrl READ getDebugScriptUrl WRITE setDebugScriptUrl)

public:
    using ScriptInitializer = ScriptInitializerMixin::ScriptInitializer;

    ScriptEngines(ScriptEngine::Context context);
    void registerScriptInitializer(ScriptInitializer initializer);
    int runScriptInitializers(ScriptEnginePointer engine);
    void loadScripts();
    void saveScripts();

    QString getDebugScriptUrl() { return _debugScriptUrl; };
    void setDebugScriptUrl(const QString& url) { _debugScriptUrl = url; };

    void loadDefaultScripts();
    void reloadLocalFiles();

    QStringList getRunningScripts();
    ScriptEnginePointer getScriptEngine(const QUrl& scriptHash);

    ScriptsModel* scriptsModel() { return &_scriptsModel; };
    ScriptsModelFilter* scriptsModelFilter() { return &_scriptsModelFilter; };

    QString getDefaultScriptsLocation() const;

    /**jsdoc
     * @function ScriptDiscoveryService.loadOneScript
     * @param {string} filename
     */
    Q_INVOKABLE void loadOneScript(const QString& scriptFilename);

    /**jsdoc
     * @function ScriptDiscoveryService.loadScript
     * @param {string} [filename=""]
     * @param {boolean} [isUserLoaded=true]
     * @param {boolean} [loadScriptFromEditor=false]
     * @param {boolean} [activateMainWindow=false]
     * @param {boolean} [reload=false]
     * @returns {boolean}
     */
    Q_INVOKABLE ScriptEnginePointer loadScript(const QUrl& scriptFilename = QString(),
        bool isUserLoaded = true, bool loadScriptFromEditor = false, bool activateMainWindow = false, bool reload = false);

    /**jsdoc
     * @function ScriptDiscoveryService.stopScript
     * @param {string} scriptHash
     * @param {boolean} [restart=false]
     * @returns {boolean}
     */
    Q_INVOKABLE bool stopScript(const QString& scriptHash, bool restart = false);


    /**jsdoc
    * @function ScriptDiscoveryService.reloadAllScripts
    */
    Q_INVOKABLE void reloadAllScripts();

    /**jsdoc
     * @function ScriptDiscoveryService.stopAllScripts
     * @param {boolean} [restart=false]
     */
    Q_INVOKABLE void stopAllScripts(bool restart = false);


    /**jsdoc
     * @function ScriptDiscoveryService.getRunning
     * @returns {object[]}
     */
    Q_INVOKABLE QVariantList getRunning();

    /**jsdoc
     * @function ScriptDiscoveryService.getPublic
     * @returns {object[]}
     */
    Q_INVOKABLE QVariantList getPublic();

    /**jsdoc
     * @function ScriptDiscoveryService.getLocal
     * @returns {object[]}
     */
    Q_INVOKABLE QVariantList getLocal();

    // FIXME: Move to other Q_PROPERTY declarations.
    Q_PROPERTY(QString defaultScriptsPath READ getDefaultScriptsLocation)

    void defaultScriptsLocationOverridden(bool overridden) { _defaultScriptsLocationOverridden = overridden; };

    // Called at shutdown time
    void shutdownScripting();
    bool isStopped() const { return _isStopped; }

    void addScriptEngine(ScriptEnginePointer);

signals:

    /**jsdoc
     * @function ScriptDiscoveryService.scriptCountChanged
     * @returns {Signal}
     */
    void scriptCountChanged();

    /**jsdoc
     * @function ScriptDiscoveryService.scriptsReloading
     * @returns {Signal}
     */
    void scriptsReloading();

    /**jsdoc
     * @function ScriptDiscoveryService.scriptLoadError
     * @param {string} filename
     * @param {string} error
     * @returns {Signal}
     */
    void scriptLoadError(const QString& filename, const QString& error);

    /**jsdoc
     * @function ScriptDiscoveryService.printedMessage
     * @param {string} message
     * @param {string} engineName
     * @returns {Signal}
     */
    void printedMessage(const QString& message, const QString& engineName);

    /**jsdoc
     * @function ScriptDiscoveryService.errorMessage
     * @param {string} message
     * @param {string} engineName
     * @returns {Signal}
     */
    void errorMessage(const QString& message, const QString& engineName);

    /**jsdoc
     * @function ScriptDiscoveryService.warningMessage
     * @param {string} message
     * @param {string} engineName
     * @returns {Signal}
     */
    void warningMessage(const QString& message, const QString& engineName);

    /**jsdoc
     * @function ScriptDiscoveryService.infoMessage
     * @param {string} message
     * @param {string} engineName
     * @returns {Signal}
     */
    void infoMessage(const QString& message, const QString& engineName);

    /**jsdoc
     * @function ScriptDiscoveryService.errorLoadingScript
     * @param {string} url
     * @returns {Signal}
     */
    void errorLoadingScript(const QString& url);

    /**jsdoc
     * @function ScriptDiscoveryService.clearDebugWindow
     * @returns {Signal}
     */
    void clearDebugWindow();

public slots:

    /**jsdoc
     * @function ScriptDiscoveryService.onPrintedMessage
     * @param {string} message
     * @param {string} scriptName
     */
    void onPrintedMessage(const QString& message, const QString& scriptName);

    /**jsdoc
     * @function ScriptDiscoveryService.onErrorMessage
     * @param {string} message
     * @param {string} scriptName
     */
    void onErrorMessage(const QString& message, const QString& scriptName);

    /**jsdoc
     * @function ScriptDiscoveryService.onWarningMessage
     * @param {string} message
     * @param {string} scriptName
     */
    void onWarningMessage(const QString& message, const QString& scriptName);

    /**jsdoc
     * @function ScriptDiscoveryService.onInfoMessage
     * @param {string} message
     * @param {string} scriptName
     */
    void onInfoMessage(const QString& message, const QString& scriptName);

    /**jsdoc
     * @function ScriptDiscoveryService.onErrorLoadingScript
     * @param {string} url
     */
    void onErrorLoadingScript(const QString& url);

    /**jsdoc
     * @function ScriptDiscoveryService.onClearDebugWindow
     */
    void onClearDebugWindow();

protected slots:

    /**jsdoc
     * @function ScriptDiscoveryService.onScriptFinished
     * @param {string} filename
     * @param {object} engine
     */
    void onScriptFinished(const QString& fileNameString, ScriptEnginePointer engine);

protected:
    friend class ScriptEngine;

    ScriptEnginePointer reloadScript(const QString& scriptName, bool isUserLoaded = true) { return loadScript(scriptName, isUserLoaded, false, false, true); }
    void removeScriptEngine(ScriptEnginePointer);
    void onScriptEngineLoaded(const QString& scriptFilename);
    void onScriptEngineError(const QString& scriptFilename);
    void launchScriptEngine(ScriptEnginePointer);

    ScriptEngine::Context _context;
    QReadWriteLock _scriptEnginesHashLock;
    QHash<QUrl, ScriptEnginePointer> _scriptEnginesHash;
    QSet<ScriptEnginePointer> _allKnownScriptEngines;
    QMutex _allScriptsMutex;
    std::list<ScriptInitializer> _scriptInitializers;
    ScriptsModel _scriptsModel;
    ScriptsModelFilter _scriptsModelFilter;
    std::atomic<bool> _isStopped { false };
    std::atomic<bool> _isReloading { false };
    bool _defaultScriptsLocationOverridden { false };
    QString _debugScriptUrl;
};

QUrl normalizeScriptURL(const QUrl& rawScriptURL);
QString expandScriptPath(const QString& rawPath);
QUrl expandScriptUrl(const QUrl& rawScriptURL);

#endif // hifi_ScriptEngine_h
