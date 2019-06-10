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

#include <platform/Profiler.h>

#include "scripting/RenderScriptingInterface.h"
#include "LODManager.h"

PerformanceManager::PerformanceManager()
{
    setPerformancePreset((PerformancePreset) _performancePresetSetting.get());
}

void PerformanceManager::setupPerformancePresetSettings(bool evaluatePlatformTier) {
    if (evaluatePlatformTier || (getPerformancePreset() == UNKNOWN)) {
        // If evaluatePlatformTier, evalute the Platform Tier and assign the matching Performance profile by default.
        // A bunch of Performance, Simulation and Render settings will be set to a matching default value from this

        // Here is the mapping between pelatformTIer and performance profile
        const std::array<PerformanceManager::PerformancePreset, platform::Profiler::NumTiers> platformToPerformancePresetMap = { {
            PerformanceManager::PerformancePreset::MID,  // platform::Profiler::UNKNOWN
            PerformanceManager::PerformancePreset::LOW,  // platform::Profiler::LOW
            PerformanceManager::PerformancePreset::MID,  // platform::Profiler::MID
            PerformanceManager::PerformancePreset::HIGH  // platform::Profiler::HIGH
        } };

        // What is our profile?
        auto platformTier = platform::Profiler::profilePlatform();

        // Then let's assign the performance preset setting from it
        setPerformancePreset(platformToPerformancePresetMap[platformTier]);
    }
}

void PerformanceManager::setPerformancePreset(PerformanceManager::PerformancePreset preset) {
    if (isValidPerformancePreset(preset) && (getPerformancePreset() != preset)) {
        _performancePresetSettingLock.withWriteLock([&] {
            _performancePresetSetting.set((int)preset);
        });

        applyPerformancePreset(preset);
    }
}

PerformanceManager::PerformancePreset PerformanceManager::getPerformancePreset() const {
    PerformancePreset preset = PerformancePreset::UNKNOWN;

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
            qApp->getRefreshRateManager().setRefreshRateProfile(RefreshRateManager::RefreshRateProfile::REALTIME);

            DependencyManager::get<LODManager>()->setWorldDetailQuality(0.5f);

        break;
        case PerformancePreset::MID:
            RenderScriptingInterface::getInstance()->setRenderMethod(RenderScriptingInterface::RenderMethod::DEFERRED);
            RenderScriptingInterface::getInstance()->setShadowsEnabled(false);
            qApp->getRefreshRateManager().setRefreshRateProfile(RefreshRateManager::RefreshRateProfile::INTERACTIVE);
            DependencyManager::get<LODManager>()->setWorldDetailQuality(0.5f);

        break;
        case PerformancePreset::LOW:
            RenderScriptingInterface::getInstance()->setRenderMethod(RenderScriptingInterface::RenderMethod::FORWARD);
            RenderScriptingInterface::getInstance()->setShadowsEnabled(false);
            qApp->getRefreshRateManager().setRefreshRateProfile(RefreshRateManager::RefreshRateProfile::ECO);

            DependencyManager::get<LODManager>()->setWorldDetailQuality(0.75f);

        break;
        case PerformancePreset::UNKNOWN:
        default:
            // Do nothing anymore
        break;
    }
}
