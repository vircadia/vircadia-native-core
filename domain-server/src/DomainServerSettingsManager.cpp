//
//  DomainServerSettingsManager.cpp
//  domain-server/src
//
//  Created by Stephen Birarda on 2014-06-24.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QUrl>

#include <HTTPConnection.h>

#include "DomainServerSettingsManager.h"

const QString SETTINGS_DESCRIPTION_RELATIVE_PATH = "/resources/web/settings/describe.json";
const QString SETTINGS_CONFIG_FILE_RELATIVE_PATH = "/resources/config.json";

DomainServerSettingsManager::DomainServerSettingsManager() :
    _descriptionObject(),
    _settingsMap()
{
    // load the description object from the settings description
    QFile descriptionFile(QCoreApplication::applicationDirPath() + SETTINGS_DESCRIPTION_RELATIVE_PATH);
    descriptionFile.open(QIODevice::ReadOnly);
    
    _descriptionObject = QJsonDocument::fromJson(descriptionFile.readAll()).object();
    
    // load the existing config file to get the current values
    QFile configFile(QCoreApplication::applicationDirPath() + SETTINGS_CONFIG_FILE_RELATIVE_PATH);
    configFile.open(QIODevice::ReadOnly);
    
    _settingsMap = QJsonDocument::fromJson(configFile.readAll()).toVariant().toMap();
}

bool DomainServerSettingsManager::handleHTTPRequest(HTTPConnection* connection, const QUrl &url) {
    if (connection->requestOperation() == QNetworkAccessManager::PostOperation && url.path() == "/settings.json") {
        // this is a POST operation to change one or more settings
        QJsonDocument postedDocument = QJsonDocument::fromJson(connection->requestContent());
        QJsonObject postedObject = postedDocument.object();
        
        // we recurse one level deep below each group for the appropriate setting
        recurseJSONObjectAndOverwriteSettings(postedObject, _settingsMap, _descriptionObject);
        
        // store whatever the current _settingsMap is to file
        persistToFile();
        
        // return success to the caller
        QString jsonSuccess = "{\"status\": \"success\"}";
        connection->respond(HTTPConnection::StatusCode200, jsonSuccess.toUtf8(), "application/json");
        
        return true;
    } else if (connection->requestOperation() == QNetworkAccessManager::GetOperation && url.path() == "/settings.json") {
        // this is a GET operation for our settings
        // combine the description object and our current settings map
        QJsonObject combinedObject;
        combinedObject["descriptions"] = _descriptionObject;
        combinedObject["values"] = QJsonDocument::fromVariant(_settingsMap).object();
        
        connection->respond(HTTPConnection::StatusCode200, QJsonDocument(combinedObject).toJson(), "application/json");
        return true;
    }
    
    return false;
}

const QString DESCRIPTION_SETTINGS_KEY = "settings";

void DomainServerSettingsManager::recurseJSONObjectAndOverwriteSettings(const QJsonObject& postedObject,
                                                                        QVariantMap& settingsVariant,
                                                                        QJsonObject descriptionObject) {
    foreach(const QString& key, postedObject.keys()) {

        QJsonValue rootValue = postedObject[key];
        
        // we don't continue if this key is not present in our descriptionObject
        if (descriptionObject.contains(key)) {
            if (rootValue.isString()) {
                settingsVariant[key] = rootValue.toString();
            } else if (rootValue.isBool()) {
                settingsVariant[key] = rootValue.toBool();
            } else if (rootValue.isObject()) {
                // there's a JSON Object to explore, so attempt to recurse into it
                QJsonObject nextDescriptionObject = descriptionObject[key].toObject();
                
                if (nextDescriptionObject.contains(DESCRIPTION_SETTINGS_KEY)) {
                    if (!settingsVariant.contains(key)) {
                        // we don't have a map below this key yet, so set it up now
                        settingsVariant[key] = QVariantMap();
                    }
                    
                    
                    recurseJSONObjectAndOverwriteSettings(rootValue.toObject(),
                                                          *reinterpret_cast<QVariantMap*>(settingsVariant[key].data()),
                                                          nextDescriptionObject[DESCRIPTION_SETTINGS_KEY].toObject());
                }
            }
        }
    }
}

QByteArray DomainServerSettingsManager::getJSONSettingsMap() const {
    return QJsonDocument::fromVariant(_settingsMap).toJson();
}

void DomainServerSettingsManager::persistToFile() {
    QFile settingsFile(QCoreApplication::applicationDirPath() + SETTINGS_CONFIG_FILE_RELATIVE_PATH);
    
    if (settingsFile.open(QIODevice::WriteOnly)) {
        settingsFile.write(getJSONSettingsMap());
    } else {
        qCritical("Could not write to JSON settings file. Unable to persist settings.");
    }
}