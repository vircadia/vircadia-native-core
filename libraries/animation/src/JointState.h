//
//  JointState.h
//  libraries/animation/src/
//
//  Created by Andrzej Kapolka on 10/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_JointState_h
#define hifi_JointState_h

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

#include <FBXReader.h>
#include <GLMHelpers.h>
#include <NumericalConstants.h>

const float DEFAULT_PRIORITY = 3.0f;

class AngularConstraint;

class JointState {
public:
    JointState() {}
    JointState(const JointState& other) { copyState(other); }
    JointState(const FBXJoint& joint);
    ~JointState();
    JointState& operator=(const JointState& other) { copyState(other); return *this; }
    void copyState(const JointState& state);
    void buildConstraint();
 
    void initTransform(const glm::mat4& parentTransform);
    // if synchronousRotationCompute is true, then _transform is still computed synchronously,
    // but _rotation will be asynchronously extracted
    void computeTransform(const glm::mat4& parentTransform, bool parentTransformChanged = true, bool synchronousRotationCompute = false);

    void computeVisibleTransform(const glm::mat4& parentTransform);
    const glm::mat4& getVisibleTransform() const { return _visibleTransform; }
    glm::quat getVisibleRotation() const { return _visibleRotation; }
    glm::vec3 getVisiblePosition() const { return extractTranslation(_visibleTransform); }

    const glm::mat4& getTransform() const { return _transform; }
    void resetTransformChanged() { _transformChanged = false; }
    bool getTransformChanged() const { return _transformChanged; }

    glm::quat getRotation() const;
    glm::vec3 getPosition() const { return extractTranslation(_transform); }

    /// \return rotation from bind to model frame
    glm::quat getRotationInBindFrame() const;

    glm::quat getRotationInParentFrame() const;
    glm::quat getVisibleRotationInParentFrame() const;
    const glm::vec3& getPositionInParentFrame() const { return _positionInParentFrame; }
    float getDistanceToParent() const { return _distanceToParent; }

    int getParentIndex() const { return _parentIndex; }

    /// \param delta is in the model-frame
    void applyRotationDelta(const glm::quat& delta, bool constrain = true, float priority = 1.0f);

    /// Applies delta rotation to joint but mixes a little bit of the default pose as well.
    /// This helps keep an IK solution stable.
    /// \param delta rotation change in model-frame
    /// \param mixFactor fraction in range [0,1] of how much default pose to blend in (0 is none, 1 is all)
    /// \param priority priority level of this animation blend
    void mixRotationDelta(const glm::quat& delta, float mixFactor, float priority = 1.0f);
    void mixVisibleRotationDelta(const glm::quat& delta, float mixFactor);

    /// Blends a fraciton of default pose into joint rotation.
    /// \param fraction fraction in range [0,1] of how much default pose to blend in (0 is none, 1 is all)
    /// \param priority priority level of this animation blend
    void restoreRotation(float fraction, float priority);

    void restoreTranslation(float fraction, float priority);

    /// \param rotation is from bind- to model-frame
    /// computes and sets new _rotationInConstrainedFrame
    /// NOTE: the JointState's model-frame transform/rotation are NOT updated!
    void setRotationInBindFrame(const glm::quat& rotation, float priority, bool constrain = false);

    /// \param rotationInModelRame is in model-frame
    /// computes and sets new _rotationInConstrainedFrame to match rotationInModelFrame
    /// NOTE: the JointState's model-frame transform/rotation are NOT updated!
    void setRotationInModelFrame(const glm::quat& rotationInModelFrame, float priority, bool constrain);

    void setRotationInConstrainedFrame(glm::quat targetRotation, float priority, bool constrain = false, float mix = 1.0f);

    void setTranslation(const glm::vec3& translation, float priority);

    void setVisibleRotationInConstrainedFrame(const glm::quat& targetRotation);
    const glm::quat& getRotationInConstrainedFrame() const { return _rotationInConstrainedFrame; }
    const glm::quat& getVisibleRotationInConstrainedFrame() const { return _visibleRotationInConstrainedFrame; }

    bool rotationIsDefault(const glm::quat& rotation, float tolerance = EPSILON) const;
    bool translationIsDefault(const glm::vec3& translation, float tolerance = EPSILON) const;

    glm::quat getDefaultRotationInParentFrame() const;
    glm::vec3 getDefaultTranslationInConstrainedFrame() const;


    void clearTransformTranslation();

    void slaveVisibleTransform();

    /// \return parent model-frame rotation
    // (used to keep _rotation consistent when modifying _rotationInWorldFrame directly)
    glm::quat computeParentRotation() const;
    glm::quat computeVisibleParentRotation() const;

    void setTransform(const glm::mat4& transform) { _transform = transform; }
    void setVisibleTransform(const glm::mat4& transform) { _visibleTransform = transform; }
    
    glm::vec3 getTranslation() const { return _translation * _unitsScale; }
    const glm::mat4& getPreTransform() const { return _preTransform; }
    const glm::mat4& getPostTransform() const { return _postTransform; }
    const glm::quat& getPreRotation() const { return _preRotation; }
    const glm::quat& getPostRotation() const { return _postRotation; }
    const glm::quat& getDefaultRotation() const { return _defaultRotation; }
    glm::vec3 getDefaultTranslation() const { return _defaultTranslation * _unitsScale; }
    const glm::quat& getInverseDefaultRotation() const { return _inverseDefaultRotation; }
    const QString& getName() const { return _name; }
    bool getIsFree() const { return _isFree; }
    float getAnimationPriority() const { return _animationPriority; }
    void setAnimationPriority(float priority) { _animationPriority = priority; }

private:
    void setRotationInConstrainedFrameInternal(const glm::quat& targetRotation);
    /// debug helper function
    void loadBindRotation();

    bool _transformChanged {true};
    bool _rotationIsValid {false};
    glm::vec3 _positionInParentFrame {0.0f}; // only changes when the Model is scaled
    float _animationPriority {0.0f}; // the priority of the animation affecting this joint
    float _distanceToParent {0.0f};
    AngularConstraint* _constraint{nullptr}; // JointState owns its AngularConstraint
    
    glm::mat4 _transform; // joint- to model-frame
    glm::quat _rotation;  // joint- to model-frame
    glm::quat _rotationInConstrainedFrame; // rotation in frame where angular constraints would be applied

    glm::mat4 _visibleTransform;
    glm::quat _visibleRotation;
    glm::quat _visibleRotationInConstrainedFrame;

    glm::quat _defaultRotation; // Not necessarilly bind rotation. See FBXJoint transform/bindTransform
    glm::quat _inverseDefaultRotation;
    glm::vec3 _defaultTranslation;
    glm::vec3 _translation;
    QString _name;
    int _parentIndex;
    bool _isFree;
    glm::vec3 _rotationMin;
    glm::vec3 _rotationMax;
    glm::quat _preRotation;
    glm::quat _postRotation;
    glm::mat4 _preTransform;
    glm::mat4 _postTransform;
    glm::quat _inverseBindRotation;

    glm::vec3 _unitsScale{1.0f, 1.0f, 1.0f};
};

#endif // hifi_JointState_h
