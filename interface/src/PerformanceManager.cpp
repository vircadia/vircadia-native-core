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

void PerformanceManager::setPerformancePreset(PerformanceManager::PerformancePreset preset) {
    if (getPerformancePreset() != preset) {
        _performancePresetSettingLock.withWriteLock([&] {
            _performancePresetSetting.set((int)preset);
        });

        applyPerformancePreset(preset);
    }
}

PerformanceManager::PerformancePreset PerformanceManager::getPerformancePreset() const {
    PerformancePreset preset = PerformancePreset::MID;

    preset = (PerformancePreset) _performancePresetSettingLock.resultWithReadLock<int>([&] {
        return _performancePresetSetting.get();
    });

    return preset;
}

void PerformanceManager::applyPerformancePreset(PerformanceManager::PerformancePreset preset) {

    switch (preset) {
        case PerformancePreset::HIGH:
            RenderScriptingInterface::getInstance()->setRenderMethod(RenderScriptingInterface::RenderMethod::DEFERRED);
            RenderScriptingInterface::getInstance()->setShadowsEnabled(true);
            qApp->getRefreshRateManager().setRefreshRateProfile(RefreshRateManager::RefreshRateProfile::INTERACTIVE);

        break;
        case PerformancePreset::MID:
            RenderScriptingInterface::getInstance()->setRenderMethod(RenderScriptingInterface::RenderMethod::DEFERRED);
            RenderScriptingInterface::getInstance()->setShadowsEnabled(false);
            qApp->getRefreshRateManager().setRefreshRateProfile(RefreshRateManager::RefreshRateProfile::INTERACTIVE);

        break;
        case PerformancePreset::LOW:
            RenderScriptingInterface::getInstance()->setRenderMethod(RenderScriptingInterface::RenderMethod::FORWARD);
            RenderScriptingInterface::getInstance()->setShadowsEnabled(false);
            qApp->getRefreshRateManager().setRefreshRateProfile(RefreshRateManager::RefreshRateProfile::ECO);

        break;
        default:
        break;
    }
}
