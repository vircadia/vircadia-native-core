//
//  KinematicController.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2015.01.13
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_KinematicController_h
#define hifi_KinematicController_h

#include <stdint.h>

/// KinematicController defines an API for derived classes.

class KinematicController {
public:
    KinematicController();

    virtual ~KinematicController() {}

    virtual void stepForward() = 0;

    void start();
    void stop() { _enabled = false; }
    bool isRunning() const { return _enabled; }

protected:
    bool _enabled = false;
    uint32_t _lastFrame;
};

#endif // hifi_KinematicController_h
