//
//  HifiConfigVariantMap.cpp
//  libraries/shared/src
//
//  Created by Stephen Birarda on 2014-04-08.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QStandardPaths>
#include <QtCore/QVariant>

#include "HifiConfigVariantMap.h"

QVariantMap HifiConfigVariantMap::mergeCLParametersWithJSONConfig(const QStringList& argumentList) {

    QVariantMap mergedMap;

    // Add anything in the CL parameter list to the variant map.
    // Take anything with a dash in it as a key, and the values after it as the value.

    const QString DASHED_KEY_REGEX_STRING = "(^-{1,2})([\\w-]+)";
    QRegExp dashedKeyRegex(DASHED_KEY_REGEX_STRING);

    int keyIndex = argumentList.indexOf(dashedKeyRegex);
    int nextKeyIndex = 0;

    // check if there is a config file to read where we can pull config info not passed on command line
    const QString CONFIG_FILE_OPTION = "--config";

    while (keyIndex != -1) {
        if (argumentList[keyIndex] != CONFIG_FILE_OPTION) {
            // we have a key - look forward to see how many values associate to it
            QString key = dashedKeyRegex.cap(2);

            nextKeyIndex = argumentList.indexOf(dashedKeyRegex, keyIndex + 1);

            if (nextKeyIndex == keyIndex + 1 || keyIndex == argumentList.size() - 1) {
                // this option is simply a switch, so add it to the map with a value of `true`
                mergedMap.insertMulti(key, QVariant(true));
            } else {
                int maxIndex = (nextKeyIndex == -1) ? argumentList.size() : nextKeyIndex;

                // there's at least one value associated with the option
                // pull the first value to start
                QString value = argumentList[keyIndex + 1];

                // for any extra values, append them, with a space, to the value string
                for (int i = keyIndex + 2; i < maxIndex; i++) {
                    value +=  " " + argumentList[i];
                }

                // add the finalized value to the merged map
                mergedMap.insert(key, value);
            }

            keyIndex = nextKeyIndex;
        } else {
            keyIndex = argumentList.indexOf(dashedKeyRegex, keyIndex + 1);
        }
    }

    int configIndex = argumentList.indexOf(CONFIG_FILE_OPTION);

    QString configFilePath;
    
    if (configIndex != -1) {
        // we have a config file - try and read it
        configFilePath = argumentList[configIndex + 1];
    } else {
        // no config file - try to read a file config.json at the system config path
        configFilePath = QString("%1/%2/%3/config.json").arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation),
                                                             QCoreApplication::organizationName(),
                                                             QCoreApplication::applicationName());
    }
    
    

    return mergedMap;
}

HifiConfigVariantMap::HifiConfigVariantMap(const QStringList& argumentList) :
    _masterConfigPath(),
    _userConfigPath(),
    _masterConfig(),
    _userConfig(),
    _mergedConfig()
{
    // check if there is a master config file
    const QString MASTER_CONFIG_FILE_OPTION = "--master-config";
    
    int masterConfigIndex = argumentList.indexOf(MASTER_CONFIG_FILE_OPTION);
    if (masterConfigIndex != -1) {
        QString masterConfigFilepath = argumentList[masterConfigIndex + 1];
        
        loadMapFromJSONFile(_masterConfig, masterConfigFilepath);
    }
    
    // load the user config
    const QString USER_CONFIG_FILE_OPTION = "--user-config";
    
    int userConfigIndex = argumentList.indexOf(USER_CONFIG_FILE_OPTION);
    QString userConfigFilepath;
    if (userConfigIndex != -1) {
        _userConfigPath = argumentList[userConfigIndex + 1];
    } else {
        _userConfigPath = QString("%1%2/%3/config.json").arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation),
                                                              QCoreApplication::organizationName(),
                                                              QCoreApplication::applicationName());
    }
    
    loadMapFromJSONFile(_userConfig, _userConfigPath);
    
    // the merged config is initially matched to the master config
    _mergedConfig = _masterConfig;
    
    // then we merge in anything missing from the user config
    addMissingValuesToExistingMap(_mergedConfig, _userConfig);
}

void HifiConfigVariantMap::loadMapFromJSONFile(QVariantMap& existingMap, const QString& filename) {
    QFile configFile(filename);
    
    if (configFile.exists()) {
        qDebug() << "Reading JSON config file at" << filename;
        configFile.open(QIODevice::ReadOnly);
        
        QJsonDocument configDocument = QJsonDocument::fromJson(configFile.readAll());
        existingMap = configDocument.toVariant().toMap();
        
    } else {
        qDebug() << "Could not find JSON config file at" << filename;
    }
}

void HifiConfigVariantMap::addMissingValuesToExistingMap(QVariantMap& existingMap, const QVariantMap& newMap) {
    foreach(const QString& key, newMap.keys()) {
        if (existingMap.contains(key)) {
            // if this is just a regular value, we're done - we don't ovveride
            
            if (newMap[key].canConvert(QMetaType::QVariantMap) && existingMap[key].canConvert(QMetaType::QVariantMap)) {
                // there's a variant map below and the existing map has one too, so we need to keep recursing
                addMissingValuesToExistingMap(*static_cast<QVariantMap*>(existingMap[key].data()), newMap[key].toMap());
            }
        } else {
            existingMap[key] = newMap[key];
        }
    }
}

const QVariant* valueForKeyPath(QVariantMap& variantMap, const QString& keyPath) {
    int dotIndex = keyPath.indexOf('.');
    
    QString firstKey = (dotIndex == -1) ? keyPath : keyPath.mid(0, dotIndex);
    
    if (variantMap.contains(firstKey)) {
        if (dotIndex == -1) {
            return &variantMap[firstKey];
        } else if (variantMap[firstKey].canConvert(QMetaType::QVariantMap)) {
            return valueForKeyPath(*static_cast<QVariantMap*>(variantMap[firstKey].data()), keyPath.mid(dotIndex + 1));
        }
    }
    
    return NULL;
}
