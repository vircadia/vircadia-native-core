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

    enum RefreshRateRegime {
        RUNNING = 0,
        UNFOCUS,
        MINIMIZED,
        STARTUP,
        SHUTDOWN,
        INACTIVE,
        REGIME_NUM
    };

    enum UXMode {
        DESKTOP = 0,
        HMD,
        UX_NUM
    };

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
    void setInteractiveRefreshRate(int refreshRate);
    int getInteractiveRefreshRate() const;

    void resetInactiveTimer();

    static std::string refreshRateProfileToString(RefreshRateProfile refreshRateProfile);
    static RefreshRateProfile refreshRateProfileFromString(std::string refreshRateProfile);
    static std::string uxModeToString(UXMode uxMode);
    static std::string refreshRateRegimeToString(RefreshRateRegime refreshRateRegime);

private:
    mutable ReadWriteLockable _refreshRateLock;
    mutable ReadWriteLockable _refreshRateModeLock;

    mutable int _activeRefreshRate { 20 };
    RefreshRateProfile _refreshRateProfile { RefreshRateProfile::INTERACTIVE};
    RefreshRateRegime _refreshRateRegime { RefreshRateRegime::STARTUP };
    UXMode _uxMode;

    Setting::Handle<int> _interactiveRefreshRate { "interactiveRefreshRate", 20};
    Setting::Handle<int> _refreshRateMode { "refreshRateProfile", INTERACTIVE };

    std::function<void(int)> _refreshRateOperator { nullptr };


    std::shared_ptr<QTimer> _inactiveTimer { std::make_shared<QTimer>() };
};

#endif
