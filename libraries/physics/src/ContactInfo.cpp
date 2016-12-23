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

#include "ContactInfo.h"

void ContactInfo::update(uint32_t currentStep, const btManifoldPoint& p) {
    _lastStep = currentStep;
    positionWorldOnB = p.m_positionWorldOnB;
    normalWorldOnB = p.m_normalWorldOnB;
    distance = p.m_distance1;
}

ContactEventType ContactInfo::computeType(uint32_t thisStep) {
    ++_numChecks;
    if (_numChecks == 1) {
        return CONTACT_EVENT_TYPE_START;
    }
    return (_lastStep == thisStep) ? CONTACT_EVENT_TYPE_CONTINUE : CONTACT_EVENT_TYPE_END;
}
