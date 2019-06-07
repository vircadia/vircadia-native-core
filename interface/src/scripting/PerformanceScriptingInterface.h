//
//  Created by Bradley Austin Davis on 2019/05/14
//  Copyright 2013-2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_PerformanceScriptingInterface_h
#define hifi_PerformanceScriptingInterface_h

#include <mutex>

#include <QObject>

#include "../PerformanceManager.h"
#include "../RefreshRateManager.h"


class PerformanceScriptingInterface : public QObject {
    Q_OBJECT
    Q_PROPERTY(PerformancePreset performancePreset READ getPerformancePreset WRITE setPerformancePreset NOTIFY settingsChanged)
    Q_PROPERTY(RefreshRateProfile refreshRateProfile READ getRefreshRateProfile WRITE setRefreshRateProfile NOTIFY settingsChanged)

public:

    // PerformanceManager PerformancePreset tri state level enums
    enum PerformancePreset {
        UNKNOWN = PerformanceManager::PerformancePreset::UNKNOWN,
        LOW = PerformanceManager::PerformancePreset::LOW,
        MID = PerformanceManager::PerformancePreset::MID,
        HIGH = PerformanceManager::PerformancePreset::HIGH,
    };
    Q_ENUM(PerformancePreset)

    // Must match RefreshRateManager enums
    enum RefreshRateProfile {
        ECO = RefreshRateManager::RefreshRateProfile::ECO,
        INTERACTIVE = RefreshRateManager::RefreshRateProfile::INTERACTIVE,
        REALTIME = RefreshRateManager::RefreshRateProfile::REALTIME,
    };
    Q_ENUM(RefreshRateProfile)

    PerformanceScriptingInterface();
    ~PerformanceScriptingInterface() = default;

public slots:

    void setPerformancePreset(PerformancePreset performancePreset);
    PerformancePreset getPerformancePreset() const;
    QStringList getPerformancePresetNames() const;

    void setRefreshRateProfile(RefreshRateProfile refreshRateProfile);
    RefreshRateProfile getRefreshRateProfile() const;
    QStringList getRefreshRateProfileNames() const;

    int getActiveRefreshRate() const;
    RefreshRateManager::UXMode getUXMode() const;
    RefreshRateManager::RefreshRateRegime getRefreshRateRegime() const;

signals:
    void settingsChanged();

private:
    static std::once_flag registry_flag;
};

#endif // header guard
