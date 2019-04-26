//
//  RefreshRateScriptingInterface.h
//  interface/src/scrfipting
//
//  Created by Dante Ruiz on 2019-04-15.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RefreshRateScriptingInterface_h
#define hifi_RefreshRateScriptingInterface_h

#include <QtCore/QObject>

#include <Application.h>

class RefreshRateScriptingInterface : public QObject {
    Q_OBJECT
public:
    RefreshRateScriptingInterface() = default;
    ~RefreshRateScriptingInterface() = default;

public:
    Q_INVOKABLE QString getRefreshRateProfile() {
        RefreshRateManager& refreshRateManager = qApp->getRefreshRateManager();
        return QString::fromStdString(RefreshRateManager::refreshRateProfileToString(refreshRateManager.getRefreshRateProfile()));
    }

    Q_INVOKABLE QString getRefreshRateRegime() {
        RefreshRateManager& refreshRateManager = qApp->getRefreshRateManager();
        return QString::fromStdString(RefreshRateManager::refreshRateRegimeToString(refreshRateManager.getRefreshRateRegime()));
    }

    Q_INVOKABLE QString getUXMode() {
        RefreshRateManager& refreshRateManager = qApp->getRefreshRateManager();
        return QString::fromStdString(RefreshRateManager::uxModeToString(refreshRateManager.getUXMode()));
    }

    Q_INVOKABLE int getActiveRefreshRate() {
        return qApp->getRefreshRateManager().getActiveRefreshRate();
    }
};

#endif
