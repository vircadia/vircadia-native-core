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
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QStandardPaths>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>

#include <Assignment.h>
#include <HifiConfigVariantMap.h>
#include <HTTPConnection.h>

#include "DomainServerSettingsManager.h"

const QString SETTINGS_DESCRIPTION_RELATIVE_PATH = "/resources/describe-settings.json";

DomainServerSettingsManager::DomainServerSettingsManager() :
    _descriptionArray(),
    _settingsMap()
{
    // load the description object from the settings description
    QFile descriptionFile(QCoreApplication::applicationDirPath() + SETTINGS_DESCRIPTION_RELATIVE_PATH);
    descriptionFile.open(QIODevice::ReadOnly);
    
    _descriptionArray = QJsonDocument::fromJson(descriptionFile.readAll()).array();
}

void DomainServerSettingsManager::loadSettingsMap(const QStringList& argumentList) {
    _settingsMap = HifiConfigVariantMap::mergeMasterConfigWithUserConfig(argumentList);
    
    // figure out where we are supposed to persist our settings to
    _settingsFilepath = HifiConfigVariantMap::userConfigFilepath(argumentList);
}

const QString DESCRIPTION_SETTINGS_KEY = "settings";
const QString SETTING_DEFAULT_KEY = "default";
const QString SETTINGS_GROUP_KEY_NAME = "name";

bool DomainServerSettingsManager::handlePublicHTTPRequest(HTTPConnection* connection, const QUrl &url) {
    if (connection->requestOperation() == QNetworkAccessManager::GetOperation && url.path() == "/settings.json") {
        // this is a GET operation for our settings
        
        // check if there is a query parameter for settings affecting a particular type of assignment
        const QString SETTINGS_TYPE_QUERY_KEY = "type";
        QUrlQuery settingsQuery(url);
        QString typeValue = settingsQuery.queryItemValue(SETTINGS_TYPE_QUERY_KEY);
        
        QJsonObject responseObject;
        
        if (typeValue.isEmpty()) {
            // combine the description object and our current settings map
            responseObject["descriptions"] = _descriptionArray;
            responseObject["values"] = QJsonDocument::fromVariant(_settingsMap).object();
        } else {
            // convert the string type value to a QJsonValue
            QJsonValue queryType = QJsonValue(typeValue.toInt());
            
            const QString AFFECTED_TYPES_JSON_KEY = "assignment-types";
            
            // enumerate the groups in the description object to find which settings to pass
            foreach(const QJsonValue& groupValue, _descriptionArray) {
                QJsonObject groupObject = groupValue.toObject();
                QString groupKey = groupObject[SETTINGS_GROUP_KEY_NAME].toString();
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
                        QVariant settingsMapGroupValue = _settingsMap.value(groupObject[SETTINGS_GROUP_KEY_NAME].toString());
                        
                        if (!settingsMapGroupValue.isNull()) {
                            variantValue = settingsMapGroupValue.toMap().value(settingKey);
                        }
                        
                        if (variantValue.isNull()) {
                            // no value for this setting, pass the default
                            if (settingObject.contains(SETTING_DEFAULT_KEY)) {
                                groupResponseObject[settingKey] = settingObject[SETTING_DEFAULT_KEY];
                            } else {
                                // users are allowed not to provide a default for string values
                                // if so we set to the empty string
                                groupResponseObject[settingKey] = QString("");
                            }
                            
                        } else {
                            groupResponseObject[settingKey] = QJsonValue::fromVariant(variantValue);
                        }
                    }
                }
                
                if (!groupResponseObject.isEmpty()) {
                    // set this group's object to the constructed object
                    responseObject[groupKey] = groupResponseObject;
                }
            }
            
        }
        
        connection->respond(HTTPConnection::StatusCode200, QJsonDocument(responseObject).toJson(), "application/json");
        return true;
    }
    
    return false;
}

bool DomainServerSettingsManager::handleAuthenticatedHTTPRequest(HTTPConnection *connection, const QUrl &url) {
    if (connection->requestOperation() == QNetworkAccessManager::PostOperation && url.path() == "/settings.json") {
        // this is a POST operation to change one or more settings
        QJsonDocument postedDocument = QJsonDocument::fromJson(connection->requestContent());
        QJsonObject postedObject = postedDocument.object();
        
        // we recurse one level deep below each group for the appropriate setting
        recurseJSONObjectAndOverwriteSettings(postedObject, _settingsMap, _descriptionArray);
        
        // store whatever the current _settingsMap is to file
        persistToFile();
        
        // return success to the caller
        QString jsonSuccess = "{\"status\": \"success\"}";
        connection->respond(HTTPConnection::StatusCode200, jsonSuccess.toUtf8(), "application/json");
        
        // defer a restart to the domain-server, this gives our HTTPConnection enough time to respond
        const int DOMAIN_SERVER_RESTART_TIMER_MSECS = 1000;
        QTimer::singleShot(DOMAIN_SERVER_RESTART_TIMER_MSECS, qApp, SLOT(restart()));
        
        return true;
    }
    
    return false;
}

const QString SETTING_DESCRIPTION_TYPE_KEY = "type";

void DomainServerSettingsManager::recurseJSONObjectAndOverwriteSettings(const QJsonObject& postedObject,
                                                                        QVariantMap& settingsVariant,
                                                                        QJsonArray descriptionArray) {
    foreach(const QString& key, postedObject.keys()) {

        QJsonValue rootValue = postedObject[key];
        
        // we don't continue if this key is not present in our descriptionObject
        foreach(const QJsonValue& groupValue, descriptionArray) {
            if (groupValue.toObject()[SETTINGS_GROUP_KEY_NAME].toString() == key) {
                QJsonObject groupObject = groupValue.toObject();
                if (rootValue.isString()) {
                    if (rootValue.toString().isEmpty()) {
                        // this is an empty value, clear it in settings variant so the default is sent
                        settingsVariant.remove(key);
                    } else {
                        QString settingType = groupObject[SETTING_DESCRIPTION_TYPE_KEY].toString();
                        
                        const QString INPUT_DOUBLE_TYPE = "double";
                        
                        // make sure the resulting json value has the right type
                        
                        if (settingType == INPUT_DOUBLE_TYPE) {
                            settingsVariant[key] = rootValue.toString().toDouble();
                        } else {
                            settingsVariant[key] = rootValue.toString();
                        }
                    }
                } else if (rootValue.isBool()) {
                    settingsVariant[key] = rootValue.toBool();
                } else if (rootValue.isObject()) {
                    // there's a JSON Object to explore, so attempt to recurse into it
                    QJsonObject nextDescriptionObject = groupObject;
                    
                    if (nextDescriptionObject.contains(DESCRIPTION_SETTINGS_KEY)) {
                        if (!settingsVariant.contains(key)) {
                            // we don't have a map below this key yet, so set it up now
                            settingsVariant[key] = QVariantMap();
                        }
                        
                        QVariantMap& thisMap = *reinterpret_cast<QVariantMap*>(settingsVariant[key].data());
                        
                        recurseJSONObjectAndOverwriteSettings(rootValue.toObject(),
                                                              thisMap,
                                                              nextDescriptionObject[DESCRIPTION_SETTINGS_KEY].toArray());
                        
                        if (thisMap.isEmpty()) {
                            // we've cleared all of the settings below this value, so remove this one too
                            settingsVariant.remove(key);
                        }
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
    
    // make sure we have the dir the settings file is supposed to live in
    QFileInfo settingsFileInfo(_settingsFilepath);
    
    if (!settingsFileInfo.dir().exists()) {
        settingsFileInfo.dir().mkpath(".");
    }
    
    QFile settingsFile(_settingsFilepath);
    
    if (settingsFile.open(QIODevice::WriteOnly)) {
        settingsFile.write(getJSONSettingsMap());
    } else {
        qCritical("Could not write to JSON settings file. Unable to persist settings.");
    }
}