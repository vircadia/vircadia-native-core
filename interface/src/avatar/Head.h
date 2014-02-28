//
//  Head.h
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef hifi_Head_h
#define hifi_Head_h

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <SharedUtil.h>

#include <HeadData.h>

#include <VoxelConstants.h>

#include "FaceModel.h"
#include "InterfaceConfig.h"
#include "world.h"

enum eyeContactTargets {
    LEFT_EYE, 
    RIGHT_EYE, 
    MOUTH
};

class Avatar;
class ProgramObject;

class Head : public HeadData {
public:
    Head(Avatar* owningAvatar);
    
    void init();
    void reset();
    void simulate(float deltaTime, bool isMine, bool delayLoad = false);
    void render(float alpha);
    void setScale(float scale);
    void setPosition(glm::vec3 position) { _position = position; }
    void setGravity(glm::vec3 gravity) { _gravity = gravity; }
    void setAverageLoudness(float averageLoudness) { _averageLoudness = averageLoudness; }
    void setReturnToCenter (bool returnHeadToCenter) { _returnHeadToCenter = returnHeadToCenter; }
    void setRenderLookatVectors(bool onOff) { _renderLookatVectors = onOff; }
    
    glm::quat getTweakedOrientation() const;
    glm::quat getCameraOrientation () const;
    const glm::vec3& getAngularVelocity() const { return _angularVelocity; }
    void setAngularVelocity(glm::vec3 angularVelocity) { _angularVelocity = angularVelocity; }
    
    float getScale() const { return _scale; }
    glm::vec3 getPosition() const { return _position; }
    const glm::vec3& getEyePosition() const { return _eyePosition; }
    const glm::vec3& getSaccade() const { return _saccade; }
    glm::vec3 getRightDirection() const { return getOrientation() * IDENTITY_RIGHT; }
    glm::vec3 getUpDirection() const { return getOrientation() * IDENTITY_UP; }
    glm::vec3 getFrontDirection() const { return getOrientation() * IDENTITY_FRONT; }
    
    glm::quat getEyeRotation(const glm::vec3& eyePosition) const;
    
    FaceModel& getFaceModel() { return _faceModel; }
    const FaceModel& getFaceModel() const { return _faceModel; }
    
    const bool getReturnToCenter() const { return _returnHeadToCenter; } // Do you want head to try to return to center (depends on interface detected)
    float getAverageLoudness() const { return _averageLoudness; }
    glm::vec3 calculateAverageEyePosition() { return _leftEyePosition + (_rightEyePosition - _leftEyePosition ) * ONE_HALF; }
    
    /// Returns the point about which scaling occurs.
    glm::vec3 getScalePivot() const;

    void tweakPitch(float pitch) { _tweakedPitch = pitch; }
    void tweakYaw(float yaw) { _tweakedYaw = yaw; }
    void tweakRoll(float roll) { _tweakedRoll = roll; }

    float getTweakedPitch() const;
    float getTweakedYaw() const;
    float getTweakedRoll() const;

    void  applyCollision(CollisionInfo& collisionInfo);
    
private:
    // disallow copies of the Head, copy of owning Avatar is disallowed too
    Head(const Head&);
    Head& operator= (const Head&);

    bool _returnHeadToCenter;
    glm::vec3 _position;
    glm::vec3 _rotation;
    glm::vec3 _leftEyePosition;
    glm::vec3 _rightEyePosition;
    glm::vec3 _eyePosition;
    float _scale;
    glm::vec3 _gravity;
    float _lastLoudness;
    float _audioAttack;
    glm::vec3 _angularVelocity;
    bool _renderLookatVectors;
    glm::vec3 _saccade;
    glm::vec3 _saccadeTarget;
    float _leftEyeBlinkVelocity;
    float _rightEyeBlinkVelocity;
    float _timeWithoutTalking;

    // tweaked angles affect the rendered head, but not the camera
    float _tweakedPitch;
    float _tweakedYaw;
    float _tweakedRoll;

    bool _isCameraMoving;
    FaceModel _faceModel;
    
    // private methods
    void renderLookatVectors(glm::vec3 leftEyePosition, glm::vec3 rightEyePosition, glm::vec3 lookatPosition);

    friend class FaceModel;
};

#endif
