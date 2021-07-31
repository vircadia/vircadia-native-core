//
//  RefreshRateManager.cpp
//  interface/src/
//
//  Created by Dante Ruiz on 2019-04-15.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "RefreshRateManager.h"

#include <array>


static const int VR_TARGET_RATE = 90;

/*@jsdoc
 * <p>Refresh rate profile.</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"Eco"</code></td><td>Low refresh rate, which is reduced when Interface doesn't have focus or is 
 *         minimized.</td></tr>
 *     <tr><td><code>"Interactive"</code></td><td>Medium refresh rate, which is reduced when Interface doesn't have focus or is 
 *         minimized.</td></tr>
 *     <tr><td><code>"Realtime"</code></td><td>High refresh rate, even when Interface doesn't have focus or is minimized.
 *   </tbody>
 * </table>
 * @typedef {string} RefreshRateProfileName
 */
static const std::array<std::string, RefreshRateManager::RefreshRateProfile::PROFILE_NUM> REFRESH_RATE_PROFILE_TO_STRING =
    { { "Eco", "Interactive", "Realtime" } };

/*@jsdoc
 * <p>Interface states that affect the refresh rate.</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"FocusActive"</code></td><td>Interface has focus and the user is active or is in VR.</td></tr>
 *     <tr><td><code>"FocusInactive"</code></td><td>Interface has focus and the user is inactive.</td></tr>
 *     <tr><td><code>"Unfocus"</code></td><td>Interface doesn't have focus.</td></tr>
 *     <tr><td><code>"Minimized"</code></td><td>Interface is minimized.</td></tr>
 *     <tr><td><code>"StartUp"</code></td><td>Interface is starting up.</td></tr>
 *     <tr><td><code>"ShutDown"</code></td><td>Interface is shutting down.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {string} RefreshRateRegimeName
 */
static const std::array<std::string, RefreshRateManager::RefreshRateRegime::REGIME_NUM> REFRESH_RATE_REGIME_TO_STRING =
    { { "FocusActive", "FocusInactive", "Unfocus", "Minimized", "StartUp", "ShutDown" } };

/*@jsdoc
 * <p>User experience (UX) modes.</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"Desktop"</code></td><td>Desktop user experience.</td></tr>
 *     <tr><td><code>"VR"</code></td><td>VR user experience.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {string} UXModeName
 */
static const std::array<std::string, RefreshRateManager::UXMode::UX_NUM> UX_MODE_TO_STRING =
    { { "Desktop", "VR" } };

static const std::map<std::string, RefreshRateManager::RefreshRateProfile> REFRESH_RATE_PROFILE_FROM_STRING =
    { { "Eco", RefreshRateManager::RefreshRateProfile::ECO },
      { "Interactive", RefreshRateManager::RefreshRateProfile::INTERACTIVE },
      { "Realtime", RefreshRateManager::RefreshRateProfile::REALTIME } };


// Porfile regimes are:
//  { { "FocusActive", "FocusInactive", "Unfocus", "Minimized", "StartUp", "ShutDown" } }

static const std::array<int, RefreshRateManager::RefreshRateRegime::REGIME_NUM> ECO_PROFILE =
    { { 20, 10, 5, 2, 30, 30 } };

static const std::array<int, RefreshRateManager::RefreshRateRegime::REGIME_NUM> INTERACTIVE_PROFILE =
    { { 30, 20, 10, 2, 30, 30 } };

static const std::array<int, RefreshRateManager::RefreshRateRegime::REGIME_NUM> REALTIME_PROFILE =
    { { 60, 60, 60, 2, 30, 30} };

static const std::array<std::array<int, RefreshRateManager::RefreshRateRegime::REGIME_NUM>, RefreshRateManager::RefreshRateProfile::PROFILE_NUM> REFRESH_RATE_PROFILES =
    { { ECO_PROFILE, INTERACTIVE_PROFILE, REALTIME_PROFILE } };


static const int INACTIVE_TIMER_LIMIT = 3000;


std::string RefreshRateManager::refreshRateProfileToString(RefreshRateManager::RefreshRateProfile refreshRateProfile) {
    return REFRESH_RATE_PROFILE_TO_STRING.at(refreshRateProfile);
}

RefreshRateManager::RefreshRateProfile RefreshRateManager::refreshRateProfileFromString(std::string refreshRateProfile) {
    return REFRESH_RATE_PROFILE_FROM_STRING.at(refreshRateProfile);
}

std::string RefreshRateManager::refreshRateRegimeToString(RefreshRateManager::RefreshRateRegime refreshRateRegime) {
    return REFRESH_RATE_REGIME_TO_STRING.at(refreshRateRegime);
}

std::string RefreshRateManager::uxModeToString(RefreshRateManager::RefreshRateManager::UXMode uxMode) {
    return UX_MODE_TO_STRING.at(uxMode);
}

RefreshRateManager::RefreshRateManager() {
    _refreshRateProfile = (RefreshRateManager::RefreshRateProfile) _refreshRateProfileSetting.get();
    _inactiveTimer->setInterval(INACTIVE_TIMER_LIMIT);
    _inactiveTimer->setSingleShot(true);
    QObject::connect(_inactiveTimer.get(), &QTimer::timeout, [&] {
        toggleInactive();
    });
}

void RefreshRateManager::resetInactiveTimer() {
    if (_uxMode == RefreshRateManager::UXMode::DESKTOP) {
        auto regime = getRefreshRateRegime();
        if (regime == RefreshRateRegime::FOCUS_ACTIVE || regime == RefreshRateRegime::FOCUS_INACTIVE) {
            _inactiveTimer->start();
            setRefreshRateRegime(RefreshRateManager::RefreshRateRegime::FOCUS_ACTIVE);
        }
    }
}

void RefreshRateManager::toggleInactive() {
    if (_uxMode == RefreshRateManager::UXMode::DESKTOP &&
        getRefreshRateRegime() == RefreshRateManager::RefreshRateRegime::FOCUS_ACTIVE) {
        setRefreshRateRegime(RefreshRateManager::RefreshRateRegime::FOCUS_INACTIVE);
    }
}

void RefreshRateManager::setRefreshRateProfile(RefreshRateManager::RefreshRateProfile refreshRateProfile) {
    if (isValidRefreshRateProfile(refreshRateProfile) && (_refreshRateProfile != refreshRateProfile)) {
        _refreshRateProfileSettingLock.withWriteLock([&] {
            _refreshRateProfile = refreshRateProfile;
            _refreshRateProfileSetting.set((int) refreshRateProfile);
        });
        updateRefreshRateController();
    }
}

RefreshRateManager::RefreshRateProfile RefreshRateManager::getRefreshRateProfile() const {
    RefreshRateManager::RefreshRateProfile profile = RefreshRateManager::RefreshRateProfile::REALTIME;

    if (getUXMode() != RefreshRateManager::UXMode::VR) {
        return _refreshRateProfile;
    }

    return profile;
}

RefreshRateManager::RefreshRateRegime RefreshRateManager::getRefreshRateRegime() const {
    if (getUXMode() == RefreshRateManager::UXMode::VR) {
        return RefreshRateManager::RefreshRateRegime::FOCUS_ACTIVE;
    } else {
        return _refreshRateRegime;
    }
}

void RefreshRateManager::setRefreshRateRegime(RefreshRateManager::RefreshRateRegime refreshRateRegime) {
    if (isValidRefreshRateRegime(refreshRateRegime) && (_refreshRateRegime != refreshRateRegime)) {
        _refreshRateRegime = refreshRateRegime;
        updateRefreshRateController();
    }

}

void RefreshRateManager::setUXMode(RefreshRateManager::UXMode uxMode) {
    if (isValidUXMode(uxMode) && (_uxMode != uxMode)) {
        _uxMode = uxMode;
        updateRefreshRateController();
    }
}

int RefreshRateManager::queryRefreshRateTarget(RefreshRateProfile profile, RefreshRateRegime regime, UXMode uxMode) const {
    int targetRefreshRate = VR_TARGET_RATE;
    if (uxMode == RefreshRateManager::UXMode::DESKTOP) {
        targetRefreshRate = REFRESH_RATE_PROFILES[profile][regime];
    }
    return targetRefreshRate;
}

void RefreshRateManager::updateRefreshRateController() const {
    if (_refreshRateOperator) {
        int targetRefreshRate = queryRefreshRateTarget(_refreshRateProfile, _refreshRateRegime, _uxMode);
        _refreshRateOperator(targetRefreshRate);
        _activeRefreshRate = targetRefreshRate;
    }
}
