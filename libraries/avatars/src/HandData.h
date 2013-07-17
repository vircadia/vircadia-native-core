//
//  HandData.h
//  hifi
//
//  Created by Eric Johnston on 6/26/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__HandData__
#define __hifi__HandData__

#include <iostream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

class AvatarData;
class FingerData;
class PalmData;

const int NUM_FINGERS_PER_HAND = 5;

class HandData {
public:
    HandData(AvatarData* owningAvatar);
    
    // These methods return the positions in Leap-relative space.
    // To convert to world coordinates, use Hand::leapPositionToWorldPosition.
    const std::vector<glm::vec3>& getFingerTips() const { return _fingerTips; }
    const std::vector<glm::vec3>& getFingerRoots() const { return _fingerRoots; }
    const std::vector<glm::vec3>& getHandPositions() const { return _handPositions; }
    const std::vector<glm::vec3>& getHandNormals() const { return _handNormals; }
    void setFingerTips(const std::vector<glm::vec3>& fingerTips) { _fingerTips = fingerTips; }
    void setFingerRoots(const std::vector<glm::vec3>& fingerRoots) { _fingerRoots = fingerRoots; }
    void setHandPositions(const std::vector<glm::vec3>& handPositons) { _handPositions = handPositons; }
    void setHandNormals(const std::vector<glm::vec3>& handNormals) { _handNormals = handNormals; }
    
    // position conversion
    glm::vec3 leapPositionToWorldPosition(const glm::vec3& leapPosition) {
        float unitScale = 0.001;            // convert mm to meters
        return _basePosition + _baseOrientation * (leapPosition * unitScale);
    }
    glm::vec3 leapDirectionToWorldDirection(const glm::vec3& leapDirection) {
        return glm::normalize(_baseOrientation * leapDirection);
    }

    friend class AvatarData;
protected:
    std::vector<glm::vec3> _fingerTips;
    std::vector<glm::vec3> _fingerRoots;
    std::vector<glm::vec3> _handPositions;
    std::vector<glm::vec3> _handNormals;
    glm::vec3              _basePosition;      // Hands are placed relative to this
    glm::quat              _baseOrientation;   // Hands are placed relative to this
    AvatarData* _owningAvatarData;
    std::vector<PalmData>  _palms;
private:
    // privatize copy ctor and assignment operator so copies of this object cannot be made
    HandData(const HandData&);
    HandData& operator= (const HandData&);
};

class FingerData {
public:
    FingerData(PalmData* owningPalmData, HandData* owningHandData);
private:
    glm::vec3 _tipRawPosition;
    glm::vec3 _rootRawPosition;
    bool      _isActive;        // This has current valid data
    PalmData* _owningPalmData;
    HandData* _owningHandData;
};

class PalmData {
public:
    PalmData(HandData* owningHandData);
    glm::vec3 getPosition() const { return _owningHandData->leapPositionToWorldPosition(_rawPosition); }
    glm::vec3 getNormal()   const { return _owningHandData->leapDirectionToWorldDirection(_rawNormal); }
    const glm::vec3& getRawPosition() const { return _rawPosition; }
    const glm::vec3& getRawNormal()   const { return _rawNormal; }
private:
    std::vector<FingerData> _fingers;
    glm::vec3 _rawPosition;
    glm::vec3 _rawNormal;
    bool      _isActive;        // This has current valid data
    HandData* _owningHandData;
};

#endif /* defined(__hifi__HandData__) */
