//
//  SimpleEntityKinematicController.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2015.01.13
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SimpleEntityKinematicController_h
#define hifi_SimpleEntityKinematicController_h

/// SimpleKinematicConstroller performs simple exrapolation of velocities.

#include <assert.h>
#include <glm/glm.hpp>

#include "KinematicController.h"

class SimpleEntityKinematicController : public KinematicController {
public:
    SimpleEntityKinematicController() = delete; // prevent compiler from making a default ctor

    SimpleEntityKinematicController(EntityItem* entity) : KinematicController(), _entity(entity) { assert(entity); }

    ~SimpleEntityKinematicController() { _entity = NULL; }

    void stepForward();

private:
    EntityItem* _entity;
};

#endif // hifi_SimpleEntityKinematicController_h
