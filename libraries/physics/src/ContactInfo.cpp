//
//  ContactEvent.cpp
//  libraries/physcis/src
//
//  Created by Andrew Meadows 2015.01.20
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "BulletUtil.h"
#include "ContactInfo.h"

void ContactInfo::update(uint32_t currentStep, btManifoldPoint& p, const glm::vec3& worldOffset) {
    _lastStep = currentStep;
    ++_numSteps;
    contactPoint = bulletToGLM(p.m_positionWorldOnB) + worldOffset;
    penetration = bulletToGLM(p.m_distance1 * p.m_normalWorldOnB);
    // TODO: also report normal
    //_normal = bulletToGLM(p.m_normalWorldOnB);
}   

ContactEventType ContactInfo::computeType(uint32_t thisStep) {
    if (_lastStep != thisStep) {
        return CONTACT_EVENT_TYPE_END;
    }
    return (_numSteps == 1) ? CONTACT_EVENT_TYPE_START : CONTACT_EVENT_TYPE_CONTINUE;
}
