//
//  ContactEvent.cpp
//  libraries/physics/src
//
//  Created by Andrew Meadows 2015.01.20
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ContactInfo.h"

void ContactInfo::update(uint32_t currentStep, const btManifoldPoint& p) {
    _lastStep = currentStep;
    ++_numSteps;
    positionWorldOnB = p.m_positionWorldOnB;
    normalWorldOnB = p.m_normalWorldOnB;
    distance = p.m_distance1;
}   

ContactEventType ContactInfo::computeType(uint32_t thisStep) {
    if (_lastStep != thisStep) {
        return CONTACT_EVENT_TYPE_END;
    }
    return (_numSteps == 1) ? CONTACT_EVENT_TYPE_START : CONTACT_EVENT_TYPE_CONTINUE;
}
