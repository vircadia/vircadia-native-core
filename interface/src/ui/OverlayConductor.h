//
//  OverlayConductor.h
//  interface/src/ui
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OverlayConductor_h
#define hifi_OverlayConductor_h

class OverlayConductor {
public:
    OverlayConductor();
    ~OverlayConductor();

    void update(float dt);
    void setEnabled(bool enable);
    bool getEnabled() const;

private:
    bool headOutsideOverlay() const;
    bool avatarHasDriveInput() const;
    bool shouldShowOverlay() const;
    bool shouldRecenterOnFadeOut() const;
    void centerUI();

    quint64 _fadeOutTime { 0 };
    bool _enabled { false };
    bool _hmdMode { false };

    mutable quint64 _desiredDrivingTimer { 0 };
    mutable bool _desiredDriving { false };
    mutable bool _currentDriving { false };
};

#endif
