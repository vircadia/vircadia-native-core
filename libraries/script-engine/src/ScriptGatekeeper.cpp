//
//  ScriptGatekeeper.cpp
//  libraries/script-engine/src
//
//  Created by Kalila L. on Mar 7 2021
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptGatekeeper.h"

#include "BuildInfo.h"
#include "SettingHandle.h"

void ScriptGatekeeper::initialize() {
    if (_initialized) {
        return;
    }

    QVariant rawCurrentWhitelistValues = Setting::Handle<QVariant>(SCRIPT_WHITELIST_ENTRIES_KEY).get();
    QString settingsSafeValues = rawCurrentWhitelistValues.toString();

    Setting::Handle<bool> whitelistEnabled { SCRIPT_WHITELIST_ENABLED_KEY, false };
    Setting::Handle<bool> isFirstRun { Settings::firstRun, true };

    QString preloadedVal = BuildInfo::PRELOADED_SCRIPT_WHITELIST;

    if (settingsSafeValues.isEmpty() && !preloadedVal.isEmpty() && isFirstRun.get()) {
        // We assume that the whitelist should be enabled if a preloaded whitelist is attached, so we activate it if it's not already active.
        if (!whitelistEnabled.get()) {
            whitelistEnabled.set(true);
        }

        Setting::Handle<QVariant>(SCRIPT_WHITELIST_ENTRIES_KEY).set(preloadedVal);
    }
    
    _initialized = true;
}
