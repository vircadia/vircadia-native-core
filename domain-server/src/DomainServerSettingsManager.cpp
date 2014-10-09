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

const QString DESCRIPTION_SETTINGS_KEY = "settings";
const QString SETTING_DEFAULT_KEY = "default";
const QString DESCRIPTION_NAME_KEY = "name";
const QString SETTING_DESCRIPTION_TYPE_KEY = "type";

DomainServerSettingsManager::DomainServerSettingsManager() :
    _descriptionArray(),
    _configMap()
{
    // load the description object from the settings description
    QFile descriptionFile(QCoreApplication::applicationDirPath() + SETTINGS_DESCRIPTION_RELATIVE_PATH);
    descriptionFile.open(QIODevice::ReadOnly);
    
    _descriptionArray = QJsonDocument::fromJson(descriptionFile.readAll()).array();
}

void DomainServerSettingsManager::setupConfigMap(const QStringList& argumentList) {
    _configMap.loadMasterAndUserConfig(argumentList);
    
    // for now we perform a temporary transition from http-username and http-password to http_username and http_password
    const QVariant* oldUsername = valueForKeyPath(_configMap.getUserConfig(), "security.http-username");
    const QVariant* oldPassword = valueForKeyPath(_configMap.getUserConfig(), "security.http-password");
    
    if (oldUsername || oldPassword) {
        QVariantMap& settingsMap = *reinterpret_cast<QVariantMap*>(_configMap.getUserConfig()["security"].data());
        
        // remove old keys, move to new format
        if (oldUsername) {
            settingsMap["http_username"] = oldUsername->toString();
            settingsMap.remove("http-username");
        }
        
        if (oldPassword) {
            settingsMap["http_password"] = oldPassword->toString();
            settingsMap.remove("http-password");
        }
        
        // save the updated settings
        persistToFile();
    }
}

QVariant DomainServerSettingsManager::valueOrDefaultValueForKeyPath(const QString &keyPath) {
    const QVariant* foundValue = valueForKeyPath(_configMap.getMergedConfig(), keyPath);
    
    if (foundValue) {
        return *foundValue;
    } else {
        int dotIndex = keyPath.indexOf('.');
        
        QString groupKey = keyPath.mid(0, dotIndex);
        QString settingKey = keyPath.mid(dotIndex + 1);
        
        foreach(const QVariant& group, _descriptionArray.toVariantList()) {
            QVariantMap groupMap = group.toMap();
            
            if (groupMap[DESCRIPTION_NAME_KEY].toString() == groupKey) {
                foreach(const QVariant& setting, groupMap[DESCRIPTION_SETTINGS_KEY].toList()) {
                    QVariantMap settingMap = setting.toMap();
                    if (settingMap[DESCRIPTION_NAME_KEY].toString() == settingKey) {
                        return settingMap[SETTING_DEFAULT_KEY];
                    }
                }
                
                return QVariant();
            }
        }
    }
    
    return QVariant();
}

const QString SETTINGS_PATH = "/settings.json";

bool DomainServerSettingsManager::handlePublicHTTPRequest(HTTPConnection* connection, const QUrl &url) {
    if (connection->requestOperation() == QNetworkAccessManager::GetOperation && url.path() == SETTINGS_PATH) {
        // this is a GET operation for our settings
        
        // check if there is a query parameter for settings affecting a particular type of assignment
        const QString SETTINGS_TYPE_QUERY_KEY = "type";
        QUrlQuery settingsQuery(url);
        QString typeValue = settingsQuery.queryItemValue(SETTINGS_TYPE_QUERY_KEY);
        
        if (!typeValue.isEmpty()) {
            QJsonObject responseObject = responseObjectForType(typeValue);
            
            connection->respond(HTTPConnection::StatusCode200, QJsonDocument(responseObject).toJson(), "application/json");
            
            return true;
        } else {
            return false;
        }
    }
    
    return false;
}

bool DomainServerSettingsManager::handleAuthenticatedHTTPRequest(HTTPConnection *connection, const QUrl &url) {
    if (connection->requestOperation() == QNetworkAccessManager::PostOperation && url.path() == SETTINGS_PATH) {
        // this is a POST operation to change one or more settings
        QJsonDocument postedDocument = QJsonDocument::fromJson(connection->requestContent());
        QJsonObject postedObject = postedDocument.object();
        
        // we recurse one level deep below each group for the appropriate setting
        recurseJSONObjectAndOverwriteSettings(postedObject, _configMap.getUserConfig(), _descriptionArray);
        
        // store whatever the current _settingsMap is to file
        persistToFile();
        
        // return success to the caller
        QString jsonSuccess = "{\"status\": \"success\"}";
        connection->respond(HTTPConnection::StatusCode200, jsonSuccess.toUtf8(), "application/json");
        
        // defer a restart to the domain-server, this gives our HTTPConnection enough time to respond
        const int DOMAIN_SERVER_RESTART_TIMER_MSECS = 1000;
        QTimer::singleShot(DOMAIN_SERVER_RESTART_TIMER_MSECS, qApp, SLOT(restart()));
        
        return true;
    } else if (connection->requestOperation() == QNetworkAccessManager::GetOperation && url.path() == SETTINGS_PATH) {
        // setup a JSON Object with descriptions and non-omitted settings
        const QString SETTINGS_RESPONSE_DESCRIPTION_KEY = "descriptions";
        const QString SETTINGS_RESPONSE_VALUE_KEY = "values";
        const QString SETTINGS_RESPONSE_LOCKED_VALUES_KEY = "locked";
        
        QJsonObject rootObject;
        rootObject[SETTINGS_RESPONSE_DESCRIPTION_KEY] = _descriptionArray;
        rootObject[SETTINGS_RESPONSE_VALUE_KEY] = responseObjectForType("", true);
        rootObject[SETTINGS_RESPONSE_LOCKED_VALUES_KEY] = QJsonDocument::fromVariant(_configMap.getMasterConfig()).object();
        
        
        connection->respond(HTTPConnection::StatusCode200, QJsonDocument(rootObject).toJson(), "application/json");
    }
    
    return false;
}

QJsonObject DomainServerSettingsManager::responseObjectForType(const QString& typeValue, bool isAuthenticated) {
    QJsonObject responseObject;
    
    if (!typeValue.isEmpty() || isAuthenticated) {
        // convert the string type value to a QJsonValue
        QJsonValue queryType = typeValue.isEmpty() ? QJsonValue() : QJsonValue(typeValue.toInt());
        
        const QString AFFECTED_TYPES_JSON_KEY = "assignment-types";
        
        // enumerate the groups in the description object to find which settings to pass
        foreach(const QJsonValue& groupValue, _descriptionArray) {
            QJsonObject groupObject = groupValue.toObject();
            QString groupKey = groupObject[DESCRIPTION_NAME_KEY].toString();
            QJsonArray groupSettingsArray = groupObject[DESCRIPTION_SETTINGS_KEY].toArray();
            
            QJsonObject groupResponseObject;
            
            foreach(const QJsonValue& settingValue, groupSettingsArray) {
                const QString VALUE_HIDDEN_FLAG_KEY = "value-hidden";
                
                QJsonObject settingObject = settingValue.toObject();
                
                if (!settingObject[VALUE_HIDDEN_FLAG_KEY].toBool()) {
                    QJsonArray affectedTypesArray = settingObject[AFFECTED_TYPES_JSON_KEY].toArray();
                    if (affectedTypesArray.isEmpty()) {
                        affectedTypesArray = groupObject[AFFECTED_TYPES_JSON_KEY].toArray();
                    }
                    
                    if (affectedTypesArray.contains(queryType) ||
                        (queryType.isNull() && isAuthenticated)) {
                        // this is a setting we should include in the responseObject
                        
                        QString settingName = settingObject[DESCRIPTION_NAME_KEY].toString();
                        
                        // we need to check if the settings map has a value for this setting
                        QVariant variantValue;
                        QVariant settingsMapGroupValue = _configMap.getMergedConfig()
                            .value(groupObject[DESCRIPTION_NAME_KEY].toString());
                        
                        if (!settingsMapGroupValue.isNull()) {
                            variantValue = settingsMapGroupValue.toMap().value(settingName);
                        }
                        
                        if (variantValue.isNull()) {
                            // no value for this setting, pass the default
                            if (settingObject.contains(SETTING_DEFAULT_KEY)) {
                                groupResponseObject[settingName] = settingObject[SETTING_DEFAULT_KEY];
                            } else {
                                // users are allowed not to provide a default for string values
                                // if so we set to the empty string
                                groupResponseObject[settingName] = QString("");
                            }
                            
                        } else {
                            groupResponseObject[settingName] = QJsonValue::fromVariant(variantValue);
                        }
                    }
                }
            }
            
            if (!groupResponseObject.isEmpty()) {
                // set this group's object to the constructed object
                responseObject[groupKey] = groupResponseObject;
            }
        }
    }
    
    
    return responseObject;
}

bool DomainServerSettingsManager::settingExists(const QString& groupName, const QString& settingName,
                                                const QJsonArray& descriptionArray, QJsonValue& settingDescription) {
    foreach(const QJsonValue& groupValue, descriptionArray) {
        QJsonObject groupObject = groupValue.toObject();
        if (groupObject[DESCRIPTION_NAME_KEY].toString() == groupName) {
            
            foreach(const QJsonValue& settingValue, groupObject[DESCRIPTION_SETTINGS_KEY].toArray()) {
                QJsonObject settingObject = settingValue.toObject();
                if (settingObject[DESCRIPTION_NAME_KEY].toString() == settingName) {
                    settingDescription = settingObject[SETTING_DEFAULT_KEY];
                    return true;
                }
            }
        }
    }
    settingDescription = QJsonValue::Undefined;
    return false;
}

void DomainServerSettingsManager::updateSetting(const QString& key, const QJsonValue& newValue, QVariantMap& settingMap,
                                                const QJsonValue& settingDescription) {
    if (newValue.isString()) {
        if (newValue.toString().isEmpty()) {
            // this is an empty value, clear it in settings variant so the default is sent
            settingMap.remove(key);
        } else {
            // make sure the resulting json value has the right type
            const QString settingType = settingDescription.toObject()[SETTING_DESCRIPTION_TYPE_KEY].toString();
            const QString INPUT_DOUBLE_TYPE = "double";
            const QString INPUT_INTEGER_TYPE = "int";
            
            if (settingType == INPUT_DOUBLE_TYPE) {
                settingMap[key] = newValue.toString().toDouble();
            } else if (settingType == INPUT_INTEGER_TYPE) {
                settingMap[key] = newValue.toString().toInt();
            } else {
                settingMap[key] = newValue.toString();
            }
        }
    } else if (newValue.isBool()) {
        settingMap[key] = newValue.toBool();
    } else if (newValue.isObject()) {
        if (!settingMap.contains(key)) {
            // we don't have a map below this key yet, so set it up now
            settingMap[key] = QVariantMap();
        }
        
        QVariantMap& thisMap = *reinterpret_cast<QVariantMap*>(settingMap[key].data());
        foreach(const QString childKey, newValue.toObject().keys()) {
            updateSetting(childKey, newValue.toObject()[childKey], thisMap, settingDescription.toObject()[key]);
        }
        
        if (settingMap[key].toMap().isEmpty()) {
            // we've cleared all of the settings below this value, so remove this one too
            settingMap.remove(key);
        }
    }
}

void DomainServerSettingsManager::recurseJSONObjectAndOverwriteSettings(const QJsonObject& postedObject,
                                                                        QVariantMap& settingsVariant,
                                                                        const QJsonArray& descriptionArray) {
    
    qDebug() << postedObject;
    // Iterate on the setting groups
    foreach(const QString& groupKey, postedObject.keys()) {
        QJsonValue groupValue = postedObject[groupKey];

        if (!settingsVariant.contains(groupKey)) {
            // we don't have a map below this key yet, so set it up now
            settingsVariant[groupKey] = QVariantMap();
        }
        
        // Iterate on the settings
        foreach(const QString& settingKey, groupValue.toObject().keys()) {
            QJsonValue settingValue = groupValue.toObject()[settingKey];
            
            QJsonValue thisDescription;
            if (settingExists(groupKey, settingKey, descriptionArray, thisDescription)) {
                QVariantMap& thisMap = *reinterpret_cast<QVariantMap*>(settingsVariant[groupKey].data());
                updateSetting(settingKey, settingValue, thisMap, thisDescription);
            }
        }
        
        if (settingsVariant[groupKey].toMap().empty()) {
            // we've cleared all of the settings below this value, so remove this one too
            settingsVariant.remove(groupKey);
        }
    }
}

void DomainServerSettingsManager::persistToFile() {
    
    // make sure we have the dir the settings file is supposed to live in
    QFileInfo settingsFileInfo(_configMap.getUserConfigFilename());
    
    if (!settingsFileInfo.dir().exists()) {
        settingsFileInfo.dir().mkpath(".");
    }
    
    QFile settingsFile(_configMap.getUserConfigFilename());
    
    if (settingsFile.open(QIODevice::WriteOnly)) {
        settingsFile.write(QJsonDocument::fromVariant(_configMap.getUserConfig()).toJson());
    } else {
        qCritical("Could not write to JSON settings file. Unable to persist settings.");
    }
}