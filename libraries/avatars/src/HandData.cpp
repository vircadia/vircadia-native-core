//
//  HandData.cpp
//  libraries/avatars/src
//
//  Created by Stephen Birarda on 5/20/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QDataStream>

#include <GeometryUtil.h>
#include <SharedUtil.h>

#include "AvatarData.h" 
#include "HandData.h"


HandData::HandData(AvatarData* owningAvatar) :
    _owningAvatarData(owningAvatar)
{
    addNewPalm(LeftHand);
    addNewPalm(RightHand);
}

glm::vec3 HandData::worldToLocalVector(const glm::vec3& worldVector) const {
    return glm::inverse(getBaseOrientation()) * worldVector / getBaseScale();
}

PalmData& HandData::addNewPalm(Hand whichHand)  {
    QWriteLocker locker(&_palmsLock);
    _palms.push_back(PalmData(this, whichHand));
    return _palms.back();
}

PalmData HandData::getCopyOfPalmData(Hand hand) const {
    QReadLocker locker(&_palmsLock);

    // the palms are not necessarily added in left-right order, 
    // so we have to search for the correct hand
    for (const auto& palm : _palms) {
        if (palm.whichHand() == hand && palm.isActive()) {
            return palm;
        }
    }
    PalmData noData;
    return noData; // invalid hand
}

void HandData::getLeftRightPalmIndices(int& leftPalmIndex, int& rightPalmIndex) const {
    QReadLocker locker(&_palmsLock);
    leftPalmIndex = -1;
    rightPalmIndex = -1;
    for (size_t i = 0; i < _palms.size(); i++) {
        const PalmData& palm = _palms[i];
        if (palm.isActive()) {
            if (palm.whichHand() == LeftHand) {
                leftPalmIndex = i;
            }
            if (palm.whichHand() == RightHand) {
                rightPalmIndex = i;
            }
        }
    }
}

PalmData::PalmData(HandData* owningHandData, HandData::Hand hand) :
_rawRotation(0.0f, 0.0f, 0.0f, 1.0f),
_rawPosition(0.0f),
_rawVelocity(0.0f),
_rawAngularVelocity(0.0f),
_totalPenetration(0.0f),
_isActive(false),
_numFramesWithoutData(0),
_owningHandData(owningHandData),
_hand(hand) {
}

void PalmData::addToPosition(const glm::vec3& delta) {
    _rawPosition += _owningHandData->worldToLocalVector(delta);
}

bool HandData::findSpherePenetration(const glm::vec3& penetratorCenter, float penetratorRadius, glm::vec3& penetration, 
                                        const PalmData*& collidingPalm) const {
    QReadLocker locker(&_palmsLock);

    for (size_t i = 0; i < _palms.size(); ++i) {
        const PalmData& palm = _palms[i];
        if (!palm.isActive()) {
            continue;
        }
        glm::vec3 palmPosition = palm.getPosition();
        const float PALM_RADIUS = 0.05f; // in world (not voxel) coordinates
        if (findSphereSpherePenetration(penetratorCenter, penetratorRadius, palmPosition, PALM_RADIUS, penetration)) {
            collidingPalm = &palm;
            return true;
        }
    }
    return false;
}

glm::quat HandData::getBaseOrientation() const {
    return _owningAvatarData->getOrientation();
}

glm::vec3 HandData::getBasePosition() const {
    return _owningAvatarData->getPosition();
}

float HandData::getBaseScale() const {
    return _owningAvatarData->getTargetScale();
}
        
glm::vec3 PalmData::getFingerDirection() const {
    // finger points along yAxis in hand-frame
    const glm::vec3 LOCAL_FINGER_DIRECTION(0.0f, 1.0f, 0.0f);
    return glm::normalize(_owningHandData->localToWorldDirection(_rawRotation * LOCAL_FINGER_DIRECTION));
}

glm::vec3 PalmData::getNormal() const {
    // palm normal points along zAxis in hand-frame
    const glm::vec3 LOCAL_PALM_DIRECTION(0.0f, 0.0f, 1.0f);
    return glm::normalize(_owningHandData->localToWorldDirection(_rawRotation * LOCAL_PALM_DIRECTION));
}



