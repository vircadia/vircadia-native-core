//
//  Settings.cpp
//
//
//  Created by Clement on 1/18/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QSettings>
#include <QThread>
#include <QThreadStorage>

#include "Settings.h"

namespace SettingHandles {
    
static QThreadStorage<QSettings*> storage;
    
QSettings* getSettings() {
    if (!storage.hasLocalData()) {
        storage.setLocalData(new QSettings());
        QObject::connect(QThread::currentThread(), &QThread::destroyed,
                         storage.localData(), &QSettings::deleteLater);
    }
    return storage.localData();
}
    
QVariant SettingsBridge::getFromSettings(const QString& key, const QVariant& defaultValue) {
    return getSettings()->value(key, defaultValue);
}

void SettingsBridge::setInSettings(const QString& key, const QVariant& value) {
    getSettings()->setValue(key, value);
}
    
void SettingsBridge::removeFromSettings(const QString& key) {
    getSettings()->remove(key);
}

}
