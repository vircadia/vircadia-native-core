//
//  DomainServerSettingsManager.h
//  domain-server/src
//
//  Created by Stephen Birarda on 2014-06-24.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DomainServerSettingsManager_h
#define hifi_DomainServerSettingsManager_h

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>

#include <HifiConfigVariantMap.h>
#include <HTTPManager.h>

#include <NLPacket.h>

const QString SETTINGS_PATHS_KEY = "paths";

const QString SETTINGS_PATH = "/settings";
const QString SETTINGS_PATH_JSON = SETTINGS_PATH + ".json";

const QString ALLOWED_USERS_SETTINGS_KEYPATH = "security.allowed_users";
const QString RESTRICTED_ACCESS_SETTINGS_KEYPATH = "security.restricted_access";

class DomainServerSettingsManager : public QObject {
    Q_OBJECT
public:
    DomainServerSettingsManager();
    bool handlePublicHTTPRequest(HTTPConnection* connection, const QUrl& url);
    bool handleAuthenticatedHTTPRequest(HTTPConnection* connection, const QUrl& url);

    void setupConfigMap(const QStringList& argumentList);
    QVariant valueOrDefaultValueForKeyPath(const QString& keyPath);

    QVariantMap& getUserSettingsMap() { return _configMap.getUserConfig(); }
    QVariantMap& getSettingsMap() { return _configMap.getMergedConfig(); }

private slots:
    void processSettingsRequestPacket(QSharedPointer<NLPacket> packet);

private:
    QJsonObject responseObjectForType(const QString& typeValue, bool isAuthenticated = false);
    void recurseJSONObjectAndOverwriteSettings(const QJsonObject& postedObject, QVariantMap& settingsVariant);

    void updateSetting(const QString& key, const QJsonValue& newValue, QVariantMap& settingMap,
                       const QJsonObject& settingDescription);
    QJsonObject settingDescriptionFromGroup(const QJsonObject& groupObject, const QString& settingName);
    void persistToFile();

    double _descriptionVersion;
    QJsonArray _descriptionArray;
    HifiConfigVariantMap _configMap;
};

#endif // hifi_DomainServerSettingsManager_h
