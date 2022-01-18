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

#include <cstdint>

class OverlayConductor {
public:
    OverlayConductor();
    ~OverlayConductor();

    void update(float dt);
    void centerUI();

private:
    bool headNotCenteredInOverlay() const;
    bool updateAvatarIsAtRest();

#if !defined(DISABLE_QML)
    bool _suppressedByHead { false };
    bool _hmdMode { false };
#endif

    // used by updateAvatarIsAtRest
    uint64_t _desiredAtRestTimer { 0 };
    bool _desiredAtRest { true };
    bool _currentAtRest { true };
};

#endif
