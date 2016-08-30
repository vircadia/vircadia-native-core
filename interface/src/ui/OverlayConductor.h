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
    void centerUI();

private:
    bool headOutsideOverlay() const;
    bool updateAvatarHasDriveInput();
    bool updateAvatarIsAtRest();

    enum SupressionFlags {
        SuppressedByDrive = 0x01,
        SuppressedByHead = 0x02,
        SuppressMask = 0x03,
    };

    uint8_t _flags { SuppressedByDrive };
    bool _hmdMode { false };

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
