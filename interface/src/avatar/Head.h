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

#include <AvatarData.h>

#include <VoxelConstants.h>

#include "BendyLine.h"
#include "FaceModel.h"
#include "InterfaceConfig.h"
#include "VideoFace.h"
#include "world.h"
#include "devices/SerialInterface.h"
#include "renderer/TextureCache.h"

enum eyeContactTargets {
    LEFT_EYE, 
    RIGHT_EYE, 
    MOUTH
};

const int MOHAWK_TRIANGLES = 50;
const int NUM_HAIR_TUFTS = 4;

class Avatar;
class ProgramObject;

class Head : public HeadData {
public:
    Head(Avatar* owningAvatar);
    
    void init();
    void reset();
    void simulate(float deltaTime, bool isMine);
    void render(float alpha, bool isMine);
    void renderMohawk();

    void setScale(float scale);
    void setPosition(glm::vec3 position) { _position = position; }
    void setBodyRotation(glm::vec3 bodyRotation) { _bodyRotation = bodyRotation; }
    void setGravity(glm::vec3 gravity) { _gravity = gravity; }
    void setSkinColor(glm::vec3 skinColor) { _skinColor = skinColor; }
    void setSpringScale(float returnSpringScale) { _returnSpringScale = returnSpringScale; }
    void setAverageLoudness(float averageLoudness) { _averageLoudness = averageLoudness; }
    void setReturnToCenter (bool returnHeadToCenter) { _returnHeadToCenter = returnHeadToCenter; }
    void setRenderLookatVectors(bool onOff) { _renderLookatVectors = onOff; }
    
    float getMousePitch() const { return _mousePitch; }
    void  setMousePitch(float mousePitch) { _mousePitch = mousePitch; }

    glm::quat getOrientation() const;
    glm::quat getCameraOrientation () const;
    const glm::vec3& getAngularVelocity() const { return _angularVelocity; }
    void setAngularVelocity(glm::vec3 angularVelocity) { _angularVelocity = angularVelocity; }
    
    float getScale() const { return _scale; }
    glm::vec3 getPosition() const { return _position; }
    const glm::vec3& getSkinColor() const { return _skinColor; }
    const glm::vec3& getEyePosition() const { return _eyePosition; }
    const glm::vec3& getSaccade() const { return _saccade; }
    glm::vec3 getRightDirection() const { return getOrientation() * IDENTITY_RIGHT; }
    glm::vec3 getUpDirection() const { return getOrientation() * IDENTITY_UP; }
    glm::vec3 getFrontDirection() const { return getOrientation() * IDENTITY_FRONT; }
    
    glm::quat getEyeRotation(const glm::vec3& eyePosition) const;
    
    VideoFace& getVideoFace() { return _videoFace; }
    FaceModel& getFaceModel() { return _faceModel; }
    
    const bool getReturnToCenter() const { return _returnHeadToCenter; } // Do you want head to try to return to center (depends on interface detected)
    float getAverageLoudness() const { return _averageLoudness; }
    glm::vec3 calculateAverageEyePosition() { return _leftEyePosition + (_rightEyePosition - _leftEyePosition ) * ONE_HALF; }
    
    /// Returns the point about which scaling occurs.
    glm::vec3 getScalePivot() const;
    
    float yawRate;

private:
    // disallow copies of the Head, copy of owning Avatar is disallowed too
    Head(const Head&);
    Head& operator= (const Head&);

    struct Nose {
        glm::vec3 top;
        glm::vec3 left;
        glm::vec3 right;
        glm::vec3 front;
    };

    float _renderAlpha;
    bool _returnHeadToCenter;
    glm::vec3 _skinColor;
    glm::vec3 _position;
    glm::vec3 _rotation;
    glm::vec3 _leftEyePosition;
    glm::vec3 _rightEyePosition;
    glm::vec3 _eyePosition;
    glm::vec3 _leftEyeBrowPosition;
    glm::vec3 _rightEyeBrowPosition;
    glm::vec3 _leftEarPosition;
    glm::vec3 _rightEarPosition;
    glm::vec3 _mouthPosition;
    Nose _nose;
    float _scale;
    glm::vec3 _gravity;
    float _lastLoudness;
    float _audioAttack;
    float _returnSpringScale; //strength of return springs
    glm::vec3 _bodyRotation;
    glm::vec3 _angularVelocity;
    bool _renderLookatVectors;
    BendyLine _hairTuft[NUM_HAIR_TUFTS];
    bool _mohawkInitialized;
    glm::vec3 _mohawkTriangleFan[MOHAWK_TRIANGLES];
    glm::vec3 _mohawkColors[MOHAWK_TRIANGLES];
    glm::vec3 _saccade;
    glm::vec3 _saccadeTarget;
    float _leftEyeBlinkVelocity;
    float _rightEyeBlinkVelocity;
    float _timeWithoutTalking;
    float _cameraPitch; //  Used to position the camera differently from the head
    float _mousePitch;
    float _cameraYaw;
    bool _isCameraMoving;
    VideoFace _videoFace;
    FaceModel _faceModel;

    QSharedPointer<Texture> _dilatedIrisTexture;

    static ProgramObject _irisProgram;
    static QSharedPointer<DilatableNetworkTexture> _irisTexture;
    static int _eyePositionLocation;
    
    // private methods
    void createMohawk();
    void renderHeadSphere();
    void renderEyeBalls();
    void renderEyeBrows();
    void renderEars();
    void renderNose();
    void renderMouth();
    void renderLookatVectors(glm::vec3 leftEyePosition, glm::vec3 rightEyePosition, glm::vec3 lookatPosition);
    void calculateGeometry();
    void resetHairPhysics();
    void updateHairPhysics(float deltaTime);

    friend class FaceModel;
};

#endif
