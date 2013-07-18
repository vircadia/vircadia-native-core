//
//  HandData.cpp
//  hifi
//
//  Created by Stephen Birarda on 5/20/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "HandData.h"

HandData::HandData(AvatarData* owningAvatar) :
    _basePosition(0.0f, 0.0f, 0.0f),
    _baseOrientation(0.0f, 0.0f, 0.0f, 1.0f),
    _owningAvatarData(owningAvatar)
{
    for (int i = 0; i < 2; ++i) {
        _palms.push_back(PalmData(this));
    }
    const int standardTrailLength = 30;
    setFingerTrailLength(standardTrailLength);
}

PalmData::PalmData(HandData* owningHandData) :
_rawPosition(0, 0, 0),
_rawNormal(0, 1, 0),
_isActive(false),
_owningHandData(owningHandData)
{
    for (int i = 0; i < NUM_FINGERS_PER_HAND; ++i) {
        _fingers.push_back(FingerData(this, owningHandData));
    }
}

FingerData::FingerData(PalmData* owningPalmData, HandData* owningHandData) :
_tipRawPosition(0, 0, 0),
_rootRawPosition(0, 0, 0),
_isActive(false),
_owningPalmData(owningPalmData),
_owningHandData(owningHandData)
{
}

void HandData::encodeRemoteData(std::vector<glm::vec3>& fingerVectors) {
    fingerVectors.clear();
    for (size_t i = 0; i < getNumPalms(); ++i) {
        PalmData& palm = getPalms()[i];
        fingerVectors.push_back(palm.getRawPosition());
        fingerVectors.push_back(palm.getRawNormal());
        for (size_t f = 0; f < palm.getNumFingers(); ++f) {
            FingerData& finger = palm.getFingers()[f];
            if (finger.isActive()) {
                fingerVectors.push_back(finger.getTipRawPosition());
                fingerVectors.push_back(finger.getRootRawPosition());
            }
            else {
                fingerVectors.push_back(glm::vec3(0,0,0));
                fingerVectors.push_back(glm::vec3(0,0,0));
            }
        }
    }
}

void HandData::decodeRemoteData(const std::vector<glm::vec3>& fingerVectors) {
    size_t vectorIndex = 0;
    for (size_t i = 0; i < getNumPalms(); ++i) {
        PalmData& palm = getPalms()[i];
        // If a palm is active, there will be
        //  1 vector for its position
        //  1 vector for normal
        // 10 vectors for fingers (5 tip/root pairs)
        bool palmActive = fingerVectors.size() >= i * 12;
        palm.setActive(palmActive);
        if (palmActive) {
            palm.setRawPosition(fingerVectors[vectorIndex++]);
            palm.setRawNormal(fingerVectors[vectorIndex++]);
            for (size_t f = 0; f < NUM_FINGERS_PER_HAND; ++f) {
                FingerData& finger = palm.getFingers()[i];
                finger.setRawTipPosition(fingerVectors[vectorIndex++]);
                finger.setRawRootPosition(fingerVectors[vectorIndex++]);
            }
        }
    }
}

void HandData::setFingerTrailLength(unsigned int length) {
    for (size_t i = 0; i < getNumPalms(); ++i) {
        PalmData& palm = getPalms()[i];
        for (size_t f = 0; f < palm.getNumFingers(); ++f) {
            FingerData& finger = palm.getFingers()[f];
            finger.setTrailLength(length);
        }
    }
}

void HandData::updateFingerTrails() {
    for (size_t i = 0; i < getNumPalms(); ++i) {
        PalmData& palm = getPalms()[i];
        for (size_t f = 0; f < palm.getNumFingers(); ++f) {
            FingerData& finger = palm.getFingers()[f];
            finger.updateTrail();
        }
    }
}

void FingerData::setTrailLength(unsigned int length) {
    _tipTrailPositions.resize(length);
    _tipTrailCurrentStartIndex = 0;
    _tipTrailCurrentValidLength = 0;
}

void FingerData::updateTrail() {
    if (_tipTrailPositions.size() == 0)
        return;
    
    if (_isActive) {
        // Add the next point in the trail.
        _tipTrailCurrentStartIndex--;
        if (_tipTrailCurrentStartIndex < 0)
            _tipTrailCurrentStartIndex = _tipTrailPositions.size() - 1;
        
        _tipTrailPositions[_tipTrailCurrentStartIndex] = getTipPosition();
        
        if (_tipTrailCurrentValidLength < _tipTrailPositions.size())
            _tipTrailCurrentValidLength++;
    }
    else {
        // It's not active, so just shorten the trail.
        if (_tipTrailCurrentValidLength > 0)
            _tipTrailCurrentValidLength--;
    }
}

int FingerData::getTrailNumPositions() {
    return _tipTrailCurrentValidLength;
}

const glm::vec3& FingerData::getTrailPosition(int index) {
    if (index >= _tipTrailCurrentValidLength) {
        static glm::vec3 zero(0,0,0);
        return zero;
    }
    int posIndex = (index + _tipTrailCurrentStartIndex) % _tipTrailCurrentValidLength;
    return _tipTrailPositions[posIndex];
}



