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
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>

#include <Assignment.h>
#include <HifiConfigVariantMap.h>
#include <HTTPConnection.h>
#include <NLPacketList.h>

#include "DomainServerSettingsManager.h"

const QString SETTINGS_DESCRIPTION_RELATIVE_PATH = "/resources/describe-settings.json";

const QString DESCRIPTION_SETTINGS_KEY = "settings";
const QString SETTING_DEFAULT_KEY = "default";
const QString DESCRIPTION_NAME_KEY = "name";
const QString SETTING_DESCRIPTION_TYPE_KEY = "type";
const QString DESCRIPTION_COLUMNS_KEY = "columns";

const QString SETTINGS_VIEWPOINT_KEY = "viewpoint";

DomainServerSettingsManager::DomainServerSettingsManager() :
    _descriptionArray(),
    _configMap()
{
    // load the description object from the settings description
    QFile descriptionFile(QCoreApplication::applicationDirPath() + SETTINGS_DESCRIPTION_RELATIVE_PATH);
    descriptionFile.open(QIODevice::ReadOnly);

    QJsonDocument descriptionDocument = QJsonDocument::fromJson(descriptionFile.readAll());

    if (descriptionDocument.isObject()) {
        QJsonObject descriptionObject = descriptionDocument.object();

        const QString DESCRIPTION_VERSION_KEY = "version";

        if (descriptionObject.contains(DESCRIPTION_VERSION_KEY)) {
            // read the version from the settings description
            _descriptionVersion = descriptionObject[DESCRIPTION_VERSION_KEY].toDouble();

            if (descriptionObject.contains(DESCRIPTION_SETTINGS_KEY)) {
                _descriptionArray = descriptionDocument.object()[DESCRIPTION_SETTINGS_KEY].toArray();
                return;
            }
        }
    }

    qCritical() << "Did not find settings decription in JSON at" << SETTINGS_DESCRIPTION_RELATIVE_PATH
            << "- Unable to continue. domain-server will quit.";
    QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);
}

void DomainServerSettingsManager::processSettingsRequestPacket(QSharedPointer<NLPacket> packet) {
    Assignment::Type type;
    packet->readPrimitive(&type);

    QJsonObject responseObject = responseObjectForType(QString::number(type));
    auto json = QJsonDocument(responseObject).toJson();

    auto packetList = std::unique_ptr<NLPacketList>(new NLPacketList(PacketType::DomainSettings, QByteArray(), true, true));

    packetList->write(json);

    auto nodeList = DependencyManager::get<LimitedNodeList>();
    nodeList->sendPacketList(std::move(packetList), packet->getSenderSockAddr());
}

void DomainServerSettingsManager::setupConfigMap(const QStringList& argumentList) {
    _configMap.loadMasterAndUserConfig(argumentList);

    // What settings version were we before and what are we using now?
    // Do we need to do any re-mapping?
    QSettings appSettings;
    const QString JSON_SETTINGS_VERSION_KEY = "json-settings/version";
    double oldVersion = appSettings.value(JSON_SETTINGS_VERSION_KEY, 0.0).toDouble();

    if (oldVersion != _descriptionVersion) {
        qDebug() << "Previous domain-server settings version was"
            << QString::number(oldVersion, 'g', 8) << "and the new version is"
            << QString::number(_descriptionVersion, 'g', 8) << "- checking if any re-mapping is required";

        // we have a version mismatch - for now handle custom behaviour here since there are not many remappings
        if (oldVersion < 1.0) {
            // This was prior to the introduction of security.restricted_access
            // If the user has a list of allowed users then set their value for security.restricted_access to true

            QVariant* allowedUsers = valueForKeyPath(_configMap.getMergedConfig(), ALLOWED_USERS_SETTINGS_KEYPATH);

            if (allowedUsers
                && allowedUsers->canConvert(QMetaType::QVariantList)
                && reinterpret_cast<QVariantList*>(allowedUsers)->size() > 0) {

                qDebug() << "Forcing security.restricted_access to TRUE since there was an"
                    << "existing list of allowed users.";

                // In the pre-toggle system the user had a list of allowed users, so
                // we need to set security.restricted_access to true
                QVariant* restrictedAccess = valueForKeyPath(_configMap.getUserConfig(),
                                                             RESTRICTED_ACCESS_SETTINGS_KEYPATH,
                                                             true);

                *restrictedAccess = QVariant(true);

                // write the new settings to the json file
                persistToFile();

                // reload the master and user config so that the merged config is right
                _configMap.loadMasterAndUserConfig(argumentList);
            }
        }
    }

    // write the current description version to our settings
    appSettings.setValue(JSON_SETTINGS_VERSION_KEY, _descriptionVersion);
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

bool DomainServerSettingsManager::handlePublicHTTPRequest(HTTPConnection* connection, const QUrl &url) {
    if (connection->requestOperation() == QNetworkAccessManager::GetOperation && url.path() == SETTINGS_PATH_JSON) {
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
    if (connection->requestOperation() == QNetworkAccessManager::PostOperation && url.path() == SETTINGS_PATH_JSON) {
        // this is a POST operation to change one or more settings
        QJsonDocument postedDocument = QJsonDocument::fromJson(connection->requestContent());
        QJsonObject postedObject = postedDocument.object();

        qDebug() << "DomainServerSettingsManager postedObject -" << postedObject;

        // we recurse one level deep below each group for the appropriate setting
        recurseJSONObjectAndOverwriteSettings(postedObject, _configMap.getUserConfig());

        // store whatever the current _settingsMap is to file
        persistToFile();

        // return success to the caller
        QString jsonSuccess = "{\"status\": \"success\"}";
        connection->respond(HTTPConnection::StatusCode200, jsonSuccess.toUtf8(), "application/json");

        // defer a restart to the domain-server, this gives our HTTPConnection enough time to respond
        const int DOMAIN_SERVER_RESTART_TIMER_MSECS = 1000;
        QTimer::singleShot(DOMAIN_SERVER_RESTART_TIMER_MSECS, qApp, SLOT(restart()));

        return true;
    } else if (connection->requestOperation() == QNetworkAccessManager::GetOperation && url.path() == SETTINGS_PATH_JSON) {
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

                        if (!groupKey.isEmpty()) {
                             QVariant settingsMapGroupValue = _configMap.getMergedConfig().value(groupKey);

                            if (!settingsMapGroupValue.isNull()) {
                                variantValue = settingsMapGroupValue.toMap().value(settingName);
                            }
                        } else {
                            variantValue = _configMap.getMergedConfig().value(settingName);
                        }

                        QJsonValue result;

                        if (variantValue.isNull()) {
                            // no value for this setting, pass the default
                            if (settingObject.contains(SETTING_DEFAULT_KEY)) {
                                result = settingObject[SETTING_DEFAULT_KEY];
                            } else {
                                // users are allowed not to provide a default for string values
                                // if so we set to the empty string
                                result = QString("");
                            }

                        } else {
                            result = QJsonValue::fromVariant(variantValue);
                        }

                        if (!groupKey.isEmpty()) {
                            // this belongs in the group object
                            groupResponseObject[settingName] = result;
                        } else {
                            // this is a value that should be at the root
                            responseObject[settingName] = result;
                        }
                    }
                }
            }

            if (!groupKey.isEmpty() && !groupResponseObject.isEmpty()) {
                // set this group's object to the constructed object
                responseObject[groupKey] = groupResponseObject;
            }
        }
    }


    return responseObject;
}

void DomainServerSettingsManager::updateSetting(const QString& key, const QJsonValue& newValue, QVariantMap& settingMap,
                                                const QJsonObject& settingDescription) {
    if (newValue.isString()) {
        if (newValue.toString().isEmpty()) {
            // this is an empty value, clear it in settings variant so the default is sent
            settingMap.remove(key);
        } else {
            // make sure the resulting json value has the right type
            QString settingType = settingDescription[SETTING_DESCRIPTION_TYPE_KEY].toString();
            const QString INPUT_DOUBLE_TYPE = "double";
            const QString INPUT_INTEGER_TYPE = "int";

            if (settingType == INPUT_DOUBLE_TYPE) {
                settingMap[key] = newValue.toString().toDouble();
            } else if (settingType == INPUT_INTEGER_TYPE) {
                settingMap[key] = newValue.toString().toInt();
            } else {
                QString sanitizedValue = newValue.toString();

                // we perform special handling for viewpoints here
                // we do not want them to be prepended with a slash
                if (key == SETTINGS_VIEWPOINT_KEY && !sanitizedValue.startsWith('/')) {
                    sanitizedValue.prepend('/');
                }

                settingMap[key] = sanitizedValue;
            }
        }
    } else if (newValue.isBool()) {
        settingMap[key] = newValue.toBool();
    } else if (newValue.isObject()) {
        if (!settingMap.contains(key)) {
            // we don't have a map below this key yet, so set it up now
            settingMap[key] = QVariantMap();
        }

        QVariant& possibleMap = settingMap[key];

        if (!possibleMap.canConvert(QMetaType::QVariantMap)) {
            // if this isn't a map then we need to make it one, otherwise we're about to crash
            qDebug() << "Value at" << key << "was not the expected QVariantMap while updating DS settings"
                << "- removing existing value and making it a QVariantMap";
            possibleMap = QVariantMap();
        }

        QVariantMap& thisMap = *reinterpret_cast<QVariantMap*>(possibleMap.data());
        foreach(const QString childKey, newValue.toObject().keys()) {

            QJsonObject childDescriptionObject = settingDescription;

            // is this the key? if so we have the description already
            if (key != settingDescription[DESCRIPTION_NAME_KEY].toString()) {
                // otherwise find the description object for this childKey under columns
                foreach(const QJsonValue& column, settingDescription[DESCRIPTION_COLUMNS_KEY].toArray()) {
                    if (column.isObject()) {
                        QJsonObject thisDescription = column.toObject();
                        if (thisDescription[DESCRIPTION_NAME_KEY] == childKey) {
                            childDescriptionObject = column.toObject();
                            break;
                        }
                    }
                }
            }

            QString sanitizedKey = childKey;

            if (key == SETTINGS_PATHS_KEY && !sanitizedKey.startsWith('/')) {
                // We perform special handling for paths here.
                // If we got sent a path without a leading slash then we add it.
                sanitizedKey.prepend("/");
            }

            updateSetting(sanitizedKey, newValue.toObject()[childKey], thisMap, childDescriptionObject);
        }

        if (settingMap[key].toMap().isEmpty()) {
            // we've cleared all of the settings below this value, so remove this one too
            settingMap.remove(key);
        }
    } else if (newValue.isArray()) {
        // we just assume array is replacement
        // TODO: we still need to recurse here with the description in case values in the array have special types
        settingMap[key] = newValue.toArray().toVariantList();
    }
}

QJsonObject DomainServerSettingsManager::settingDescriptionFromGroup(const QJsonObject& groupObject, const QString& settingName) {
    foreach(const QJsonValue& settingValue, groupObject[DESCRIPTION_SETTINGS_KEY].toArray()) {
        QJsonObject settingObject = settingValue.toObject();
        if (settingObject[DESCRIPTION_NAME_KEY].toString() == settingName) {
            return settingObject;
        }
    }

    return QJsonObject();
}

void DomainServerSettingsManager::recurseJSONObjectAndOverwriteSettings(const QJsonObject& postedObject,
                                                                        QVariantMap& settingsVariant) {
    // Iterate on the setting groups
    foreach(const QString& rootKey, postedObject.keys()) {
        QJsonValue rootValue = postedObject[rootKey];

        if (!settingsVariant.contains(rootKey)) {
            // we don't have a map below this key yet, so set it up now
            settingsVariant[rootKey] = QVariantMap();
        }

        QVariantMap* thisMap = &settingsVariant;

        QJsonObject groupDescriptionObject;

        // we need to check the description array to see if this is a root setting or a group setting
        foreach(const QJsonValue& groupValue, _descriptionArray) {
            if (groupValue.toObject()[DESCRIPTION_NAME_KEY] == rootKey) {
                // we matched a group - keep this since we'll use it below to update the settings
                groupDescriptionObject = groupValue.toObject();

                // change the map we will update to be the map for this group
                thisMap = reinterpret_cast<QVariantMap*>(settingsVariant[rootKey].data());

                break;
            }
        }

        if (groupDescriptionObject.isEmpty()) {
            // this is a root value, so we can call updateSetting for it directly
            // first we need to find our description value for it

            QJsonObject matchingDescriptionObject;

            foreach(const QJsonValue& groupValue, _descriptionArray) {
                // find groups with root values (they don't have a group name)
                QJsonObject groupObject = groupValue.toObject();
                if (!groupObject.contains(DESCRIPTION_NAME_KEY)) {
                    // this is a group with root values - check if our setting is in here
                    matchingDescriptionObject = settingDescriptionFromGroup(groupObject, rootKey);

                    if (!matchingDescriptionObject.isEmpty()) {
                        break;
                    }
                }
            }

            if (!matchingDescriptionObject.isEmpty()) {
                updateSetting(rootKey, rootValue, *thisMap, matchingDescriptionObject);
            } else {
                qDebug() << "Setting for root key" << rootKey << "does not exist - cannot update setting.";
            }
        } else {
            // this is a group - iterate on the settings in the group
            foreach(const QString& settingKey, rootValue.toObject().keys()) {
                // make sure this particular setting exists and we have a description object for it
                QJsonObject matchingDescriptionObject = settingDescriptionFromGroup(groupDescriptionObject, settingKey);

                // if we matched the setting then update the value
                if (!matchingDescriptionObject.isEmpty()) {
                    QJsonValue settingValue = rootValue.toObject()[settingKey];
                    updateSetting(settingKey, settingValue, *thisMap, matchingDescriptionObject);
                } else {
                    qDebug() << "Could not find description for setting" << settingKey << "in group" << rootKey <<
                        "- cannot update setting.";
                }
            }
        }

        if (settingsVariant[rootKey].toMap().empty()) {
            // we've cleared all of the settings below this value, so remove this one too
            settingsVariant.remove(rootKey);
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
