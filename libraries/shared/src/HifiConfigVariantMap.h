//
//  HifiConfigVariantMap.h
//  libraries/shared/src
//
//  Created by Stephen Birarda on 2014-04-08.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HifiConfigVariantMap_h
#define hifi_HifiConfigVariantMap_h

#include <QtCore/QStringList>
#include <QtCore/QVariantMap>

class HifiConfigVariantMap {
public:
    static QVariantMap mergeCLParametersWithJSONConfig(const QStringList& argumentList);

    HifiConfigVariantMap();
    void loadMasterAndUserConfig(const QStringList& argumentList);
    void loadConfig(const QStringList& argumentList);

    const QVariantMap& getMasterConfig() const { return _masterConfig; }
    QVariantMap& getUserConfig() { return _userConfig; }
    QVariantMap& getMergedConfig() { return _mergedConfig; }

    void mergeMasterAndUserConfigs();

    const QString& getUserConfigFilename() const { return _userConfigFilename; }
private:
    QString _userConfigFilename;

    QVariantMap _masterConfig;
    QVariantMap _userConfig;
    QVariantMap _mergedConfig;

    void loadMapFromJSONFile(QVariantMap& existingMap, const QString& filename);
    void addMissingValuesToExistingMap(QVariantMap& existingMap, const QVariantMap& newMap);
};

QVariant* valueForKeyPath(QVariantMap& variantMap, const QString& keyPath, bool shouldCreateIfMissing = false);

#endif // hifi_HifiConfigVariantMap_h
