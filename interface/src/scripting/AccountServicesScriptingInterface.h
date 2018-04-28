//
//  AccountServicesScriptingInterface.h
//  interface/src/scripting
//
//  Created by Thijs Wenker on 9/10/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AccountServicesScriptingInterface_h
#define hifi_AccountServicesScriptingInterface_h

#include <QObject>
#include <QScriptContext>
#include <QScriptEngine>
#include <QScriptValue>
#include <QString>
#include <QStringList>
#include <DiscoverabilityManager.h>

class DownloadInfoResult {
public:
    DownloadInfoResult();
    QList<float> downloading;  // List of percentages
    float pending;
};

Q_DECLARE_METATYPE(DownloadInfoResult)

QScriptValue DownloadInfoResultToScriptValue(QScriptEngine* engine, const DownloadInfoResult& result);
void DownloadInfoResultFromScriptValue(const QScriptValue& object, DownloadInfoResult& result);

class AccountServicesScriptingInterface : public QObject {
    Q_OBJECT

    /**jsdoc
     * The AccountServices API contains helper functions related to user connectivity
     * 
     * @hifi-interface
     * @hifi-client-entity
     *
     * @namespace AccountServices
     * @property {string} username <em>Read-only.</em>
     * @property {boolean} loggedIn <em>Read-only.</em>
     * @property {string} findableBy
     * @property {string} metaverseServerURL <em>Read-only.</em>
     */

    Q_PROPERTY(QString username READ getUsername NOTIFY myUsernameChanged)
    Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY loggedInChanged)
    Q_PROPERTY(QString findableBy READ getFindableBy WRITE setFindableBy NOTIFY findableByChanged)
    Q_PROPERTY(QUrl metaverseServerURL READ getMetaverseServerURL CONSTANT)
    
public:
    static AccountServicesScriptingInterface* getInstance();

    const QString getUsername() const;
    bool loggedIn() const { return _loggedIn; }
    QUrl getMetaverseServerURL() { return DependencyManager::get<AccountManager>()->getMetaverseServerURL(); }
    
public slots:

    /**jsdoc
     * @function AccountServices.getDownloadInfo
     * @returns {DownloadInfoResult}
     */
    DownloadInfoResult getDownloadInfo();

    /**jsdoc
     * @function AccountServices.updateDownloadInfo
     */
    void updateDownloadInfo();

    /**jsdoc
     * @function AccountServices.isLoggedIn
     * @returns {boolean}
     */
    bool isLoggedIn();

    /**jsdoc
     * @function AccountServices.checkAndSignalForAccessToken
     * @returns {boolean}
     */
    bool checkAndSignalForAccessToken();

    /**jsdoc
     * @function AccountServices.logOut
     */
    void logOut();
    
private slots:
    void loggedOut();
    void checkDownloadInfo();
    
    QString getFindableBy() const;
    void setFindableBy(const QString& discoverabilityMode);
    void discoverabilityModeChanged(Discoverability::Mode discoverabilityMode);

    void onUsernameChanged(const QString& username);

signals:

    /**jsdoc
     * @function AccountServices.connected
     * @returns {Signal}
     */
    void connected();

    /**jsdoc
     * @function AccountServices.disconnected
     * @param {string} reason
     * @returns {Signal}
     */
    void disconnected(const QString& reason);

    /**jsdoc
     * @function AccountServices.myUsernameChanged
     * @param {string} username
     * @returns {Signal}
     */
    void myUsernameChanged(const QString& username);

    /**jsdoc
     * @function AccountServices.downloadInfoChanged
     * @param {} info
     * @returns {Signal}
     */
    void downloadInfoChanged(DownloadInfoResult info);

    /**jsdoc
     * @function AccountServices.findableByChanged
     * @param {string} discoverabilityMode
     * @returns {Signal}
     */
    void findableByChanged(const QString& discoverabilityMode);

    /**jsdoc
     * @function AccountServices.loggedInChanged
     * @param {boolean} loggedIn
     * @returns {Signal}
     */
    void loggedInChanged(bool loggedIn);

private:
    AccountServicesScriptingInterface();
    ~AccountServicesScriptingInterface();
    
    bool _downloading;
    bool _loggedIn{ false };
};

#endif // hifi_AccountServicesScriptingInterface_h
