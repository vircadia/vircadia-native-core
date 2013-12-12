//
//  HandData.cpp
//  hifi
//
//  Created by Stephen Birarda on 5/20/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "HandData.h"
#include "AvatarData.h"
#include <SharedUtil.h>


// When converting between fixed and float, use this as the radix.
const int fingerVectorRadix = 4;

HandData::HandData(AvatarData* owningAvatar) :
    _basePosition(0.0f, 0.0f, 0.0f),
    _baseOrientation(0.0f, 0.0f, 0.0f, 1.0f),
    _owningAvatarData(owningAvatar)
{
    // Start with two palms
    addNewPalm();
    addNewPalm();
}

glm::vec3 HandData::worldPositionToLeapPosition(const glm::vec3& worldPosition) const {
    return glm::inverse(_baseOrientation) * (worldPosition - _basePosition) / LEAP_UNIT_SCALE;
}

glm::vec3 HandData::worldVectorToLeapVector(const glm::vec3& worldVector) const {
    return glm::inverse(_baseOrientation) * worldVector / LEAP_UNIT_SCALE;
}

PalmData& HandData::addNewPalm()  {
    _palms.push_back(PalmData(this));
    return _palms.back();
}

void HandData::getLeftRightPalmIndices(int& leftPalmIndex, int& rightPalmIndex) const {
    leftPalmIndex = -1;
    float leftPalmX = FLT_MAX;
    rightPalmIndex = -1;    
    float rightPalmX = -FLT_MAX;
    for (int i = 0; i < _palms.size(); i++) {
        const PalmData& palm = _palms[i];
        if (palm.isActive()) {
            float x = palm.getRawPosition().x;
            if (x < leftPalmX) {
                leftPalmIndex = i;
                leftPalmX = x;
            }
            if (x > rightPalmX) {
                rightPalmIndex = i;
                rightPalmX = x;
            }
        }
    }
}

PalmData::PalmData(HandData* owningHandData) :
_rawRotation(0, 0, 0, 1),
_rawPosition(0, 0, 0),
_rawNormal(0, 1, 0),
_rawVelocity(0, 0, 0),
_rotationalVelocity(0, 0, 0),
_controllerButtons(0),
_isActive(false),
_leapID(LEAPID_INVALID),
_sixenseID(SIXENSEID_INVALID),
_numFramesWithoutData(0),
_owningHandData(owningHandData),
_isCollidingWithVoxel(false)
{
    for (int i = 0; i < NUM_FINGERS_PER_HAND; ++i) {
        _fingers.push_back(FingerData(this, owningHandData));
    }
}

void PalmData::addToPosition(const glm::vec3& delta) {
    // convert to Leap coordinates, then add to palm and finger positions
    glm::vec3 leapDelta = _owningHandData->worldVectorToLeapVector(delta);
    _rawPosition += leapDelta;
    for (int i = 0; i < getNumFingers(); i++) {
        FingerData& finger = _fingers[i];
        if (finger.isActive()) {
            finger.setRawTipPosition(finger.getTipRawPosition() + leapDelta);
            finger.setRawRootPosition(finger.getRootRawPosition() + leapDelta);
        }
    }
}

FingerData::FingerData(PalmData* owningPalmData, HandData* owningHandData) :
_tipRawPosition(0, 0, 0),
_rootRawPosition(0, 0, 0),
_isActive(false),
_leapID(LEAPID_INVALID),
_numFramesWithoutData(0),
_owningPalmData(owningPalmData),
_owningHandData(owningHandData)
{
    const int standardTrailLength = 10;
    setTrailLength(standardTrailLength);
}

int HandData::encodeRemoteData(unsigned char* destinationBuffer) {
    const unsigned char* startPosition = destinationBuffer;

    unsigned int numHands = 0;
    for (unsigned int handIndex = 0; handIndex < getNumPalms(); ++handIndex) {
        PalmData& palm = getPalms()[handIndex];
        if (palm.isActive()) {
            numHands++;
        }
    }
    *destinationBuffer++ = numHands;

    for (unsigned int handIndex = 0; handIndex < getNumPalms(); ++handIndex) {
        PalmData& palm = getPalms()[handIndex];
        if (palm.isActive()) {
            destinationBuffer += packFloatVec3ToSignedTwoByteFixed(destinationBuffer, palm.getRawPosition(), fingerVectorRadix);
            destinationBuffer += packFloatVec3ToSignedTwoByteFixed(destinationBuffer, palm.getRawNormal(), fingerVectorRadix);

            unsigned int numFingers = 0;
            for (unsigned int fingerIndex = 0; fingerIndex < palm.getNumFingers(); ++fingerIndex) {
                FingerData& finger = palm.getFingers()[fingerIndex];
                if (finger.isActive()) {
                    numFingers++;
                }
            }
            *destinationBuffer++ = numFingers;

            for (unsigned int fingerIndex = 0; fingerIndex < palm.getNumFingers(); ++fingerIndex) {
                FingerData& finger = palm.getFingers()[fingerIndex];
                if (finger.isActive()) {
                    destinationBuffer += packFloatVec3ToSignedTwoByteFixed(destinationBuffer, finger.getTipRawPosition(), fingerVectorRadix);
                    destinationBuffer += packFloatVec3ToSignedTwoByteFixed(destinationBuffer, finger.getRootRawPosition(), fingerVectorRadix);
                }
            }
        }
    }
    // One byte for error checking safety.
    size_t checkLength = destinationBuffer - startPosition;
    *destinationBuffer++ = (unsigned char)checkLength;

    // just a double-check, while tracing a crash.
//    decodeRemoteData(destinationBuffer - (destinationBuffer - startPosition));
    
    return destinationBuffer - startPosition;
}

int HandData::decodeRemoteData(unsigned char* sourceBuffer) {
    const unsigned char* startPosition = sourceBuffer;
        
    unsigned int numHands = *sourceBuffer++;
    
    for (unsigned int handIndex = 0; handIndex < numHands; ++handIndex) {
        if (handIndex >= getNumPalms())
            addNewPalm();
        PalmData& palm = getPalms()[handIndex];

        glm::vec3 handPosition;
        glm::vec3 handNormal;
        sourceBuffer += unpackFloatVec3FromSignedTwoByteFixed(sourceBuffer, handPosition, fingerVectorRadix);
        sourceBuffer += unpackFloatVec3FromSignedTwoByteFixed(sourceBuffer, handNormal, fingerVectorRadix);
        unsigned int numFingers = *sourceBuffer++;

        palm.setRawPosition(handPosition);
        palm.setRawNormal(handNormal);
        palm.setActive(true);
        
        for (unsigned int fingerIndex = 0; fingerIndex < numFingers; ++fingerIndex) {
            if (fingerIndex < palm.getNumFingers()) {
                FingerData& finger = palm.getFingers()[fingerIndex];

                glm::vec3 tipPosition;
                glm::vec3 rootPosition;
                sourceBuffer += unpackFloatVec3FromSignedTwoByteFixed(sourceBuffer, tipPosition, fingerVectorRadix);
                sourceBuffer += unpackFloatVec3FromSignedTwoByteFixed(sourceBuffer, rootPosition, fingerVectorRadix);

                finger.setRawTipPosition(tipPosition);
                finger.setRawRootPosition(rootPosition);
                finger.setActive(true);
            }
        }
        // Turn off any fingers which weren't used.
        for (unsigned int fingerIndex = numFingers; fingerIndex < palm.getNumFingers(); ++fingerIndex) {
            FingerData& finger = palm.getFingers()[fingerIndex];
            finger.setActive(false);
        }
    }
    // Turn off any hands which weren't used.
    for (unsigned int handIndex = numHands; handIndex < getNumPalms(); ++handIndex) {
        PalmData& palm = getPalms()[handIndex];
        palm.setActive(false);
    }
    
    // One byte for error checking safety.
    unsigned char requiredLength = (unsigned char)(sourceBuffer - startPosition);
    unsigned char checkLength = *sourceBuffer++;
    assert(checkLength == requiredLength);

    return sourceBuffer - startPosition;
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
        // It's not active, so just kill the trail.
        _tipTrailCurrentValidLength = 0;
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



