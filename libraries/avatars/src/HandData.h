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

const int NUM_HANDS = 2;
const int NUM_FINGERS_PER_HAND = 5;
const int NUM_FINGERS = NUM_HANDS * NUM_FINGERS_PER_HAND;

const int LEAPID_INVALID = -1;

class HandData {
public:
    HandData(AvatarData* owningAvatar);
    
    // These methods return the positions in Leap-relative space.
    // To convert to world coordinates, use Hand::leapPositionToWorldPosition.
    
    // position conversion
    glm::vec3 leapPositionToWorldPosition(const glm::vec3& leapPosition) {
        const float unitScale = 0.001;            // convert mm to meters
        return _basePosition + _baseOrientation * (leapPosition * unitScale);
    }
    glm::vec3 leapDirectionToWorldDirection(const glm::vec3& leapDirection) {
        return glm::normalize(_baseOrientation * leapDirection);
    }

    std::vector<PalmData>& getPalms()    { return _palms; }
    size_t                 getNumPalms() { return _palms.size(); }
    PalmData&              addNewPalm();

    void setFingerTrailLength(unsigned int length);
    void updateFingerTrails();

    // Use these for sending and receiving hand data
    void encodeRemoteData(std::vector<glm::vec3>& fingerVectors);
    void decodeRemoteData(const std::vector<glm::vec3>& fingerVectors);
    
    friend class AvatarData;
protected:
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

    glm::vec3        getTipPosition()     const { return _owningHandData->leapPositionToWorldPosition(_tipRawPosition); }
    glm::vec3        getRootPosition()    const { return _owningHandData->leapPositionToWorldPosition(_rootRawPosition); }
    const glm::vec3& getTipRawPosition()  const { return _tipRawPosition; }
    const glm::vec3& getRootRawPosition() const { return _rootRawPosition; }
    bool             isActive()           const { return _isActive; }
    int              getLeapID()          const { return _leapID; }

    void setActive(bool active)                   { _isActive = active; }
    void setLeapID(int id)                        { _leapID = id; }
    void setRawTipPosition(const glm::vec3& pos)  { _tipRawPosition = pos; }
    void setRawRootPosition(const glm::vec3& pos) { _rootRawPosition = pos; }
    void setTrailLength(unsigned int length);
    void updateTrail();

    int              getTrailNumPositions();
    const glm::vec3& getTrailPosition(int index);

    void incrementFramesWithoutData()          { _numFramesWithoutData++; }
    void resetFramesWithoutData()              { _numFramesWithoutData = 0; }
    int  getFramesWithoutData()          const { return _numFramesWithoutData; }

private:
    glm::vec3 _tipRawPosition;
    glm::vec3 _rootRawPosition;
    bool      _isActive;            // This has current valid data
    int       _leapID;              // the Leap's serial id for this tracked object
    int       _numFramesWithoutData; // after too many frames without data, this tracked object assumed lost.
    std::vector<glm::vec3> _tipTrailPositions;
    int                    _tipTrailCurrentStartIndex;
    int                    _tipTrailCurrentValidLength;
    PalmData* _owningPalmData;
    HandData* _owningHandData;
};

class PalmData {
public:
    PalmData(HandData* owningHandData);
    glm::vec3 getPosition()           const { return _owningHandData->leapPositionToWorldPosition(_rawPosition); }
    glm::vec3 getNormal()             const { return _owningHandData->leapDirectionToWorldDirection(_rawNormal); }
    const glm::vec3& getRawPosition() const { return _rawPosition; }
    const glm::vec3& getRawNormal()   const { return _rawNormal; }
    bool             isActive()       const { return _isActive; }
    int              getLeapID()      const { return _leapID; }

    std::vector<FingerData>& getFingers()    { return _fingers; }
    size_t                   getNumFingers() { return _fingers.size(); }

    void setActive(bool active)                { _isActive = active; }
    void setLeapID(int id)                     { _leapID = id; }
    void setRawPosition(const glm::vec3& pos)  { _rawPosition = pos; }
    void setRawNormal(const glm::vec3& normal) { _rawNormal = normal; }

    void incrementFramesWithoutData()          { _numFramesWithoutData++; }
    void resetFramesWithoutData()              { _numFramesWithoutData = 0; }
    int  getFramesWithoutData()          const { return _numFramesWithoutData; }

private:
    std::vector<FingerData> _fingers;
    glm::vec3 _rawPosition;
    glm::vec3 _rawNormal;
    bool      _isActive;            // This has current valid data
    int       _leapID;              // the Leap's serial id for this tracked object
    int       _numFramesWithoutData; // after too many frames without data, this tracked object assumed lost.
    HandData* _owningHandData;
};

#endif /* defined(__hifi__HandData__) */
