//
//  PerformanceManager.cpp
//  interface/src/
//
//  Created by Sam Gateau on 2019-05-29.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "PerformanceManager.h"

PerformanceManager::PerformanceManager()
{
}

void PerformanceManager::setPerformanceProfile(PerformanceManager::PerformanceProfile performanceProfile) {
    _performanceProfileSettingLock.withWriteLock([&] {
        _performanceProfileSetting.set((int)performanceProfile);
    });
}

PerformanceManager::PerformanceProfile PerformanceManager::getPerformanceProfile() const {
    PerformanceProfile profile = PerformanceProfile::MID;

    profile = (PerformanceProfile) _performanceProfileSettingLock.resultWithReadLock<int>([&] {
        return _performanceProfileSetting.get();
    });

    return profile;
}