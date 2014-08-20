//
//  JointState.h
//  interface/src/renderer
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

#include <GLMHelpers.h>
#include <FBXReader.h>

class AngularConstraint;

class JointState {
public:
    JointState();
    JointState(const JointState& other);
    ~JointState();

    void setFBXJoint(const FBXJoint* joint); 
    const FBXJoint& getFBXJoint() const { return *_fbxJoint; }

    void updateConstraint();
    void copyState(const JointState& state);

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

    int getParentIndex() const { return _fbxJoint->parentIndex; }

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

    /// \param rotation is from bind- to model-frame
    /// computes and sets new _rotationInConstrainedFrame
    /// NOTE: the JointState's model-frame transform/rotation are NOT updated!
    void setRotationInBindFrame(const glm::quat& rotation, float priority, bool constrain = false);

    void setRotationInConstrainedFrame(const glm::quat& targetRotation);
    void setVisibleRotationInConstrainedFrame(const glm::quat& targetRotation);
    const glm::quat& getRotationInConstrainedFrame() const { return _rotationInConstrainedFrame; }
    const glm::quat& getVisibleRotationInConstrainedFrame() const { return _visibleRotationInConstrainedFrame; }

    const bool rotationIsDefault(const glm::quat& rotation, float tolerance = EPSILON) const;

    glm::quat getDefaultRotationInParentFrame() const;
    const glm::vec3& getDefaultTranslationInConstrainedFrame() const;


    void clearTransformTranslation();

    void slaveVisibleTransform();

    float _animationPriority; // the priority of the animation affecting this joint

    /// \return parent model-frame rotation 
    // (used to keep _rotation consistent when modifying _rotationInWorldFrame directly)
    glm::quat computeParentRotation() const;
    glm::quat computeVisibleParentRotation() const;

private:
    /// debug helper function
    void loadBindRotation();

    bool _transformChanged;
    glm::mat4 _transform; // joint- to model-frame
    bool _rotationIsValid;
    glm::quat _rotation;  // joint- to model-frame
    glm::quat _rotationInConstrainedFrame; // rotation in frame where angular constraints would be applied
    glm::vec3 _positionInParentFrame; // only changes when the Model is scaled
    float _distanceToParent;

    glm::mat4 _visibleTransform;
    glm::quat _visibleRotation;
    glm::quat _visibleRotationInConstrainedFrame;

    const FBXJoint* _fbxJoint; // JointState does NOT own its FBXJoint
    AngularConstraint* _constraint; // JointState owns its AngularConstraint
};

#endif // hifi_JointState_h
