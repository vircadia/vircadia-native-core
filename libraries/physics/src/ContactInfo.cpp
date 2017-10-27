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
    positionWorldOnB = p.m_positionWorldOnB;
    normalWorldOnB = p.m_normalWorldOnB;
    distance = p.m_distance1;
}

const uint32_t STEPS_BETWEEN_CONTINUE_EVENTS = 9;

ContactEventType ContactInfo::computeType(uint32_t thisStep) {
    if (_continueExpiry == 0) {
        _continueExpiry = thisStep + STEPS_BETWEEN_CONTINUE_EVENTS;
        return CONTACT_EVENT_TYPE_START;
    }
    return (_lastStep == thisStep) ? CONTACT_EVENT_TYPE_CONTINUE : CONTACT_EVENT_TYPE_END;
}

bool ContactInfo::readyForContinue(uint32_t thisStep) {
    if (thisStep > _continueExpiry) {
        _continueExpiry = thisStep + STEPS_BETWEEN_CONTINUE_EVENTS;
        return true;
    }
    return false;
}
