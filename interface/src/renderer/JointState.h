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

#include <FBXReader.h>

class JointState {
public:
    JointState();

    void copyState(const JointState& state);

    void setFBXJoint(const FBXJoint* joint); 
    const FBXJoint& getFBXJoint() const { return *_fbxJoint; }

    void computeTransform(const glm::mat4& parentTransform);


    const glm::mat4& getTransform() const { return _transform; }

    glm::quat getRotation() const { return _rotation; }
    glm::vec3 getPosition() const { return extractTranslation(_transform); }

    /// \return rotation from bind to model frame
    glm::quat getRotationFromBindToModelFrame() const;

    /// \param rotation rotation of joint in model-frame
    void setRotation(const glm::quat& rotation, bool constrain, float priority);

    /// \param delta is in the jointParent-frame
    void applyRotationDelta(const glm::quat& delta, bool constrain = true, float priority = 1.0f);

    void restoreRotation(float fraction, float priority);

    /// \param rotation is from bind- to model-frame
    /// computes and sets new _rotationInParentFrame
    /// NOTE: the JointState's model-frame transform/rotation are NOT updated!
    void setRotationFromBindFrame(const glm::quat& rotation, float priority);

    void setRotationInParentFrame(const glm::quat& rotation) { _rotationInParentFrame = rotation; }
    const glm::quat& getRotationInParentFrame() const { return _rotationInParentFrame; }

    const glm::vec3& getDefaultTranslationInParentFrame() const;


    void clearTransformTranslation();

    float _animationPriority; // the priority of the animation affecting this joint

private:
    glm::mat4 _transform; // joint- to model-frame
    glm::quat _rotation;  // joint- to model-frame
    glm::quat _rotationInParentFrame; // joint- to parentJoint-frame

    const FBXJoint* _fbxJoint; // JointState does NOT own its FBXJoint
};

#endif // hifi_JointState_h
