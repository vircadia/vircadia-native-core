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

    void centerUI();

private:
    bool headOutsideOverlay() const;
    bool updateAvatarHasDriveInput();
    bool updateAvatarIsAtRest();
    bool userWishesToHide() const;
    bool userWishesToShow() const;

    enum State {
        Enabled = 0,
        DisabledByDrive,
        DisabledByHead,
        DisabledByToggle,
        NumStates
    };

    void setState(State state);
    State getState() const;

    State _state { DisabledByDrive };

    bool _prevOverlayMenuChecked { true };
    bool _enabled { false };
    bool _hmdMode { false };
    bool _disabledFromHead { false };

    // used by updateAvatarHasDriveInput
    quint64 _desiredDrivingTimer { 0 };
    bool _desiredDriving { false };
    bool _currentDriving { false };

    // used by updateAvatarIsAtRest
    quint64 _desiredAtRestTimer { 0 };
    bool _desiredAtRest { true };
    bool _currentAtRest { true };
};

#endif
