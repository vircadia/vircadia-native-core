//
//  RefreshRateManager.h
//  interface/src/
//
//  Created by Dante Ruiz on 2019-04-15.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RefreshRateManager_h
#define hifi_RefreshRateManager_h

#include <map>
#include <string>
#include <functional>

#include <QTimer>

#include <SettingHandle.h>
#include <shared/ReadWriteLockable.h>

class RefreshRateManager {
public:
    enum RefreshRateProfile {
        ECO = 0,
        INTERACTIVE,
        REALTIME,
        PROFILE_NUM
    };
    static bool isValidRefreshRateProfile(RefreshRateProfile value) { return (value >= RefreshRateProfile::ECO && value <= RefreshRateProfile::REALTIME); }

    enum RefreshRateRegime {
        FOCUS_ACTIVE = 0,
        FOCUS_INACTIVE,
        UNFOCUS,
        MINIMIZED,
        STARTUP,
        SHUTDOWN,
        REGIME_NUM
    };
    static bool isValidRefreshRateRegime(RefreshRateRegime value) { return (value >= RefreshRateRegime::FOCUS_ACTIVE && value <= RefreshRateRegime::SHUTDOWN); }

    enum UXMode {
        DESKTOP = 0,
        VR,
        UX_NUM
    };
    static bool isValidUXMode(UXMode value) { return (value >= UXMode::DESKTOP && value <= UXMode::VR); }

    RefreshRateManager();
    ~RefreshRateManager() = default;

    void setRefreshRateProfile(RefreshRateProfile refreshRateProfile);
    RefreshRateProfile getRefreshRateProfile() const;

    void setRefreshRateRegime(RefreshRateRegime refreshRateRegime);
    RefreshRateRegime getRefreshRateRegime() const;

    void setUXMode(UXMode uxMode);
    UXMode getUXMode() const { return _uxMode; }

    void setRefreshRateOperator(std::function<void(int)> refreshRateOperator) { _refreshRateOperator = refreshRateOperator; }
    int getActiveRefreshRate() const { return _activeRefreshRate; }
    void updateRefreshRateController() const;

    // query the refresh rate target at the specified combination
    int queryRefreshRateTarget(RefreshRateProfile profile, RefreshRateRegime regime, UXMode uxMode) const;

    void resetInactiveTimer();
    void toggleInactive();

    static std::string refreshRateProfileToString(RefreshRateProfile refreshRateProfile);
    static RefreshRateProfile refreshRateProfileFromString(std::string refreshRateProfile);
    static std::string uxModeToString(UXMode uxMode);
    static std::string refreshRateRegimeToString(RefreshRateRegime refreshRateRegime);

private:
    mutable int _activeRefreshRate { 20 };
    RefreshRateProfile _refreshRateProfile { RefreshRateProfile::INTERACTIVE};
    RefreshRateRegime _refreshRateRegime { RefreshRateRegime::STARTUP };
    UXMode _uxMode;

    mutable ReadWriteLockable _refreshRateProfileSettingLock;
    Setting::Handle<int> _refreshRateProfileSetting { "refreshRateProfile", RefreshRateProfile::INTERACTIVE };

    std::function<void(int)> _refreshRateOperator { nullptr };

    std::shared_ptr<QTimer> _inactiveTimer { std::make_shared<QTimer>() };
};

#endif
