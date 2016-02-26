//
//  HandData.h
//  libraries/avatars/src
//
//  Created by Eric Johnston on 6/26/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HandData_h
#define hifi_HandData_h

#include <functional>
#include <iostream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QReadWriteLock>

#include <NumericalConstants.h>
#include <SharedUtil.h>

class AvatarData;
class PalmData;

class HandData {
public:
    enum Hand {
        LeftHand,
        RightHand,
        UnknownHand,
        NUMBER_OF_HANDS
    };

    HandData(AvatarData* owningAvatar);
    virtual ~HandData() {}

    // position conversion
    glm::vec3 localToWorldPosition(const glm::vec3& localPosition) {
        return getBasePosition() + getBaseOrientation() * localPosition * getBaseScale();
    }

    glm::vec3 localToWorldDirection(const glm::vec3& localVector) {
        return getBaseOrientation() * localVector * getBaseScale();
    }

    glm::vec3 worldToLocalVector(const glm::vec3& worldVector) const;

    PalmData getCopyOfPalmData(Hand hand) const;

    std::vector<PalmData> getCopyOfPalms() const { QReadLocker locker(&_palmsLock); return _palms; }

    /// Checks for penetration between the described sphere and the hand.
    /// \param penetratorCenter the center of the penetration test sphere
    /// \param penetratorRadius the radius of the penetration test sphere
    /// \param penetration[out] the vector in which to store the penetration
    /// \param collidingPalm[out] a const PalmData* to the palm that was collided with
    /// \return whether or not the sphere penetrated
    bool findSpherePenetration(const glm::vec3& penetratorCenter, float penetratorRadius, glm::vec3& penetration,
        const PalmData*& collidingPalm) const;

    glm::quat getBaseOrientation() const;

    /// Allows a lamda function write access to the specific palm for this Hand, this might
    /// modify the _palms vector
    template<typename PalmModifierFunction> void modifyPalm(Hand whichHand, PalmModifierFunction callback);

    friend class AvatarData;
protected:
    AvatarData* _owningAvatarData;
    std::vector<PalmData> _palms;
    mutable QReadWriteLock _palmsLock{ QReadWriteLock::Recursive };

    glm::vec3 getBasePosition() const;
    float getBaseScale() const;

    PalmData& addNewPalm(Hand whichHand);
    PalmData& getPalmData(Hand hand);

private:
    // privatize copy ctor and assignment operator so copies of this object cannot be made
    HandData(const HandData&);
    HandData& operator= (const HandData&);
};


class PalmData {
public:
    PalmData(HandData* owningHandData = nullptr, HandData::Hand hand = HandData::UnknownHand);
    glm::vec3 getPosition() const { return _owningHandData->localToWorldPosition(_rawPosition); }
    glm::vec3 getVelocity() const { return _owningHandData->localToWorldDirection(_rawVelocity); }
    glm::vec3 getAngularVelocity() const { return _owningHandData->localToWorldDirection(_rawAngularVelocity); }

    const glm::vec3& getRawPosition() const { return _rawPosition; }
    bool isActive() const { return _isActive; }
    bool isValid() const { return _owningHandData; }

    void setActive(bool active) { _isActive = active; }

    HandData::Hand whichHand() const { return _hand; }
    void setHand(HandData::Hand hand) { _hand = hand; }

    void setRawRotation(const glm::quat& rawRotation) { _rawRotation = rawRotation; };
    glm::quat getRawRotation() const { return _rawRotation; }
    glm::quat getRotation() const { return _owningHandData->getBaseOrientation() * _rawRotation; }
    void setRawPosition(const glm::vec3& pos)  { _rawPosition = pos; }
    void setRawVelocity(const glm::vec3& velocity) { _rawVelocity = velocity; }
    const glm::vec3& getRawVelocity()  const { return _rawVelocity; }

    void setRawAngularVelocity(const glm::vec3& angularVelocity) { _rawAngularVelocity = angularVelocity; }
    const glm::vec3& getRawAngularVelocity() const { return _rawAngularVelocity; }

    void addToPosition(const glm::vec3& delta);

    void addToPenetration(const glm::vec3& penetration) { _totalPenetration += penetration; }
    void resolvePenetrations() { addToPosition(-_totalPenetration); _totalPenetration = glm::vec3(0.0f); }

    void setTipPosition(const glm::vec3& position) { _tipPosition = position; }
    const glm::vec3 getTipPosition() const { return _owningHandData->localToWorldPosition(_tipPosition); }
    const glm::vec3& getTipRawPosition() const { return _tipPosition; }

    void setTipVelocity(const glm::vec3& velocity) { _tipVelocity = velocity; }
    const glm::vec3 getTipVelocity() const { return _owningHandData->localToWorldDirection(_tipVelocity); }
    const glm::vec3& getTipRawVelocity() const { return _tipVelocity; }

    void incrementFramesWithoutData() { _numFramesWithoutData++; }
    void resetFramesWithoutData() { _numFramesWithoutData = 0; }
    int  getFramesWithoutData() const { return _numFramesWithoutData; }

    // FIXME - these are used in SkeletonModel::updateRig() the skeleton/rig should probably get this information
    // from an action and/or the UserInputMapper instead of piping it through here.
    void setTrigger(float trigger) { _trigger = trigger; }
    float getTrigger() const { return _trigger; }

    // return world-frame:
    glm::vec3 getFingerDirection() const;
    glm::vec3 getNormal() const;

private:
    // unless marked otherwise, these are all in the model-frame
    glm::quat _rawRotation;
    glm::vec3 _rawPosition;
    glm::vec3 _rawVelocity;
    glm::vec3 _rawAngularVelocity;
    glm::quat _rawDeltaRotation;
    glm::quat _lastRotation;

    glm::vec3 _tipPosition;
    glm::vec3 _tipVelocity;
    glm::vec3 _totalPenetration; /// accumulator for per-frame penetrations

    float _trigger;

    bool _isActive; /// This has current valid data
    int _numFramesWithoutData; /// after too many frames without data, this tracked object assumed lost.
    HandData* _owningHandData;
    HandData::Hand _hand;
};

template<typename PalmModifierFunction> void HandData::modifyPalm(Hand whichHand, PalmModifierFunction callback) {
    QReadLocker locker(&_palmsLock);
    for (auto& palm : _palms) {
        if (palm.whichHand() == whichHand && palm.isValid()) {
            callback(palm);
            return;
        }
    }
}

#endif // hifi_HandData_h
