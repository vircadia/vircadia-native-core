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

QVariant* valueForKeyPath(QVariantMap& variantMap, const QString& keyPath, bool shouldCreateIfMissing = false);

class HifiConfigVariantMap {
public:
    static QVariantMap mergeCLParametersWithJSONConfig(const QStringList& argumentList);

    void loadConfig();

    const QVariant value(const QString& key) const { return _userConfig.value(key); }
    QVariant* valueForKeyPath(const QString& keyPath, bool shouldCreateIfMissing = false)
        { return ::valueForKeyPath(_userConfig, keyPath, shouldCreateIfMissing); }

    QVariantMap& getConfig() { return _userConfig; }

    const QString& getUserConfigFilename() const { return _userConfigFilename; }
    void setUserConfigFilename(const QString& filename) { _userConfigFilename = filename; }
private:
    QString _userConfigFilename;

    QVariantMap _userConfig;

    void loadMapFromJSONFile(QVariantMap& existingMap, const QString& filename);
    void addMissingValuesToExistingMap(QVariantMap& existingMap, const QVariantMap& newMap);
};

#endif // hifi_HifiConfigVariantMap_h
