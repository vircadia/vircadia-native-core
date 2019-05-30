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

#include "scripting/RenderScriptingInterface.h"

PerformanceManager::PerformanceManager()
{
}

void PerformanceManager::setPerformanceProfile(PerformanceManager::PerformanceProfile performanceProfile) {
    if (getPerformanceProfile() != performanceProfile) {
        _performanceProfileSettingLock.withWriteLock([&] {
            _performanceProfileSetting.set((int)performanceProfile);
        });

        applyPerformanceProfile(performanceProfile);
    }
}

PerformanceManager::PerformanceProfile PerformanceManager::getPerformanceProfile() const {
    PerformanceProfile profile = PerformanceProfile::MID;

    profile = (PerformanceProfile) _performanceProfileSettingLock.resultWithReadLock<int>([&] {
        return _performanceProfileSetting.get();
    });

    return profile;
}

void PerformanceManager::applyPerformanceProfile(PerformanceManager::PerformanceProfile performanceProfile) {

    switch (performanceProfile) {
        case PerformanceProfile::HIGH:
            RenderScriptingInterface::getInstance()->setRenderMethod(RenderScriptingInterface::RenderMethod::DEFERRED);
            RenderScriptingInterface::getInstance()->setShadowsEnabled(true);
            qApp->getRefreshRateManager().setRefreshRateProfile(RefreshRateManager::RefreshRateProfile::INTERACTIVE);

        break;
        case PerformanceProfile::MID:
            RenderScriptingInterface::getInstance()->setRenderMethod(RenderScriptingInterface::RenderMethod::DEFERRED);
            RenderScriptingInterface::getInstance()->setShadowsEnabled(false);
            qApp->getRefreshRateManager().setRefreshRateProfile(RefreshRateManager::RefreshRateProfile::INTERACTIVE);

        break;
        case PerformanceProfile::LOW:
            RenderScriptingInterface::getInstance()->setRenderMethod(RenderScriptingInterface::RenderMethod::FORWARD);
            RenderScriptingInterface::getInstance()->setShadowsEnabled(false);
            qApp->getRefreshRateManager().setRefreshRateProfile(RefreshRateManager::RefreshRateProfile::ECO);

        break;
        default:
        break;
    }
}
