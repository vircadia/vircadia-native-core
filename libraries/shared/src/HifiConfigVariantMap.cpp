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
        // no config file - try to read a file at resources/config.json
        configFilePath = QCoreApplication::applicationDirPath() + "/resources/config.json";
    }
    
    QFile configFile(configFilePath);
    
    if (configFile.exists()) {
        qDebug() << "Reading JSON config file at" << configFilePath;
        configFile.open(QIODevice::ReadOnly);
        
        QJsonDocument configDocument = QJsonDocument::fromJson(configFile.readAll());
        QJsonObject rootObject = configDocument.object();
        
        // enumerate the keys of the configDocument object
        foreach(const QString& key, rootObject.keys()) {
            
            if (!mergedMap.contains(key)) {
                // no match in existing list, add it
                mergedMap.insert(key, QVariant(rootObject[key]));
            }
        }
    } else {
        qDebug() << "Could not find JSON config file at" << configFilePath;
    }

    return mergedMap;
}
