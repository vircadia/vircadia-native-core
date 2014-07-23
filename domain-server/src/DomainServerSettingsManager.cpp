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
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>

#include <Assignment.h>
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
    
    if (configFile.exists()) {
        configFile.open(QIODevice::ReadOnly);
        
        _settingsMap = QJsonDocument::fromJson(configFile.readAll()).toVariant().toMap();
    }
}

const QString DESCRIPTION_SETTINGS_KEY = "settings";
const QString SETTING_DEFAULT_KEY = "default";

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
        
        // check if there is a query parameter for settings affecting a particular type of assignment
        const QString SETTINGS_TYPE_QUERY_KEY = "type";
        QUrlQuery settingsQuery(url);
        QString typeValue = settingsQuery.queryItemValue(SETTINGS_TYPE_QUERY_KEY);
        
        QJsonObject responseObject;
        
        if (typeValue.isEmpty()) {
            // combine the description object and our current settings map
            responseObject["descriptions"] = _descriptionObject;
            responseObject["values"] = QJsonDocument::fromVariant(_settingsMap).object();
        } else {
            // convert the string type value to a QJsonValue
            QJsonValue queryType = QJsonValue(typeValue.toInt());
            
            const QString AFFECTED_TYPES_JSON_KEY = "assignment-types";
            
            // enumerate the groups in the description object to find which settings to pass
            foreach(const QString& group, _descriptionObject.keys()) {
                QJsonObject groupObject = _descriptionObject[group].toObject();
                QJsonObject groupSettingsObject = groupObject[DESCRIPTION_SETTINGS_KEY].toObject();
                
                QJsonObject groupResponseObject;
                
                
                foreach(const QString& settingKey, groupSettingsObject.keys()) {
                    QJsonObject settingObject = groupSettingsObject[settingKey].toObject();
                    
                    QJsonArray affectedTypesArray = settingObject[AFFECTED_TYPES_JSON_KEY].toArray();
                    if (affectedTypesArray.isEmpty()) {
                        affectedTypesArray = groupObject[AFFECTED_TYPES_JSON_KEY].toArray();
                    }
                    
                    if (affectedTypesArray.contains(queryType)) {
                        // this is a setting we should include in the responseObject
                        
                        // we need to check if the settings map has a value for this setting
                        QVariant variantValue;
                        QVariant settingsMapGroupValue = _settingsMap.value(group);
                        
                        if (!settingsMapGroupValue.isNull()) {
                            variantValue = settingsMapGroupValue.toMap().value(settingKey);
                        }
                        
                        if (variantValue.isNull()) {
                            // no value for this setting, pass the default
                            groupResponseObject[settingKey] = settingObject[SETTING_DEFAULT_KEY];
                        } else {
                            groupResponseObject[settingKey] = QJsonValue::fromVariant(variantValue);
                        }
                    }
                }
                
                if (!groupResponseObject.isEmpty()) {
                    // set this group's object to the constructed object
                    responseObject[group] = groupResponseObject;
                }
            }
            
        }
        
        connection->respond(HTTPConnection::StatusCode200, QJsonDocument(responseObject).toJson(), "application/json");
        return true;
    }
    
    return false;
}

void DomainServerSettingsManager::recurseJSONObjectAndOverwriteSettings(const QJsonObject& postedObject,
                                                                        QVariantMap& settingsVariant,
                                                                        QJsonObject descriptionObject) {
    foreach(const QString& key, postedObject.keys()) {

        QJsonValue rootValue = postedObject[key];
        
        // we don't continue if this key is not present in our descriptionObject
        if (descriptionObject.contains(key)) {
            if (rootValue.isString()) {
                if (rootValue.toString().isEmpty()) {
                    // this is an empty value, clear it in settings variant so the default is sent
                    settingsVariant.remove(key);
                } else {
                	settingsVariant[key] = rootValue.toString();
                }
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
                    
                    QVariantMap& thisMap = *reinterpret_cast<QVariantMap*>(settingsVariant[key].data());
                    
                    recurseJSONObjectAndOverwriteSettings(rootValue.toObject(),
                                                          thisMap,
                                                          nextDescriptionObject[DESCRIPTION_SETTINGS_KEY].toObject());
                    
                    if (thisMap.isEmpty()) {
                        // we've cleared all of the settings below this value, so remove this one too
                        settingsVariant.remove(key);
                    }
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