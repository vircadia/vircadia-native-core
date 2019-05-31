//
//  PerformanceManager.h
//  interface/src/
//
//  Created by Sam Gateau on 2019-05-29.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PerformanceManager_h
#define hifi_PerformanceManager_h

#include <string>

#include <SettingHandle.h>
#include <shared/ReadWriteLockable.h>

class PerformanceManager {
public:
    enum PerformancePreset {
        UNKNOWN = 0, // Matching the platform Tier profiles enumeration for coherence
        LOW,
        MID,
        HIGH,
        PROFILE_COUNT
    };

    PerformanceManager();
    ~PerformanceManager() = default;

    void setPerformancePreset(PerformancePreset performancePreset);
    PerformancePreset getPerformancePreset() const;

private:
    mutable ReadWriteLockable _performancePresetSettingLock;
    Setting::Handle<int> _performancePresetSetting { "performancePreset", PerformanceManager::PerformancePreset::MID };

    // The concrete performance preset changes
    void applyPerformancePreset(PerformanceManager::PerformancePreset performancePreset);
};

#endif
