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

#include <HTTPManager.h>

class DomainServerSettingsManager : public QObject {
    Q_OBJECT
public:
    DomainServerSettingsManager();
    bool handlePublicHTTPRequest(HTTPConnection* connection, const QUrl& url);
    bool handleAuthenticatedHTTPRequest(HTTPConnection* connection, const QUrl& url);
    
    void loadSettingsMap(const QStringList& argumentList);
    
    QByteArray getJSONSettingsMap() const;
    QVariantMap& getSettingsMap() { return _settingsMap; }
private:
    void recurseJSONObjectAndOverwriteSettings(const QJsonObject& postedObject, QVariantMap& settingsVariant,
                                               QJsonArray descriptionArray);
    void persistToFile();
    
    QJsonArray _descriptionArray;
    QVariantMap _settingsMap;
    QString _settingsFilepath;
};

#endif // hifi_DomainServerSettingsManager_h