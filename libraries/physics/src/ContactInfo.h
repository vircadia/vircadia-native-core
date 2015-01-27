//
//  ContactEvent.h
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2015.01.20
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ContactEvent_h
#define hifi_ContactEvent_h

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

#include "RegisteredMetaTypes.h"

enum ContactEventType {
    CONTACT_EVENT_TYPE_START, 
    CONTACT_EVENT_TYPE_CONTINUE,
    CONTACT_EVENT_TYPE_END
};

class ContactInfo : public Collision
{       
public: 
    void update(uint32_t currentStep, btManifoldPoint& p, const glm::vec3& worldOffset);
    ContactEventType computeType(uint32_t thisStep);
private:
    uint32_t _lastStep = 0;
    uint32_t _numSteps = 0;
};  


#endif // hifi_ContactEvent_h
