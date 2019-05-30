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
    enum PerformanceProfile {
        LOW = 0,
        MID,
        HIGH,
        PROFILE_COUNT
    };

    PerformanceManager();
    ~PerformanceManager() = default;

    void setPerformanceProfile(PerformanceProfile performanceProfile);
    PerformanceProfile getPerformanceProfile() const;

private:
    mutable ReadWriteLockable _performanceProfileSettingLock;
    Setting::Handle<int> _performanceProfileSetting { "performanceProfile", PerformanceManager::PerformanceProfile::MID };

    // The concrete performance profile changes
    void applyPerformanceProfile(PerformanceManager::PerformanceProfile performanceProfile);
};

#endif
