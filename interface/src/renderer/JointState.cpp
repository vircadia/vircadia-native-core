//
//  JointState.cpp
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 10/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/norm.hpp>

#include <AngularConstraint.h>
//#include <GeometryUtil.h>
#include <SharedUtil.h>

#include "JointState.h"

JointState::JointState() :
    _animationPriority(0.0f),
    _fbxJoint(NULL),
    _constraint(NULL) {
}

JointState::JointState(const JointState& other) : _constraint(NULL) {
    _rotationInParentFrame = other._rotationInParentFrame;
    _transform = other._transform;
    _rotation = other._rotation;
    _animationPriority = other._animationPriority;
    _fbxJoint = other._fbxJoint;
    // DO NOT copy _constraint
}

JointState::~JointState() {
    delete _constraint;
    _constraint = NULL;
}

void JointState::setFBXJoint(const FBXJoint* joint) {
    assert(joint != NULL);
    _rotationInParentFrame = joint->rotation;
    // NOTE: JointState does not own the FBXJoint to which it points.
    _fbxJoint = joint;
    if (_constraint) {
        delete _constraint;
        _constraint = NULL;
    }
}

void JointState::updateConstraint() {
    if (_constraint) {
        delete _constraint;
        _constraint = NULL;
    }
    if (glm::distance2(glm::vec3(-PI), _fbxJoint->rotationMin) > EPSILON || 
            glm::distance2(glm::vec3(PI), _fbxJoint->rotationMax) > EPSILON ) {
        // this joint has rotation constraints
        _constraint = AngularConstraint::newAngularConstraint(_fbxJoint->rotationMin, _fbxJoint->rotationMax);
    }
}

void JointState::copyState(const JointState& state) {
    _rotationInParentFrame = state._rotationInParentFrame;
    _transform = state._transform;
    _rotation = extractRotation(_transform);
    _animationPriority = state._animationPriority;
    // DO NOT copy _fbxJoint or _constraint
}

void JointState::computeTransform(const glm::mat4& parentTransform) {
    glm::quat modifiedRotation = _fbxJoint->preRotation * _rotationInParentFrame * _fbxJoint->postRotation;
    glm::mat4 modifiedTransform = _fbxJoint->preTransform * glm::mat4_cast(modifiedRotation) * _fbxJoint->postTransform;
    _transform = parentTransform * glm::translate(_fbxJoint->translation) * modifiedTransform;
    _rotation = extractRotation(_transform);
}

glm::quat JointState::getRotationFromBindToModelFrame() const {
    return _rotation * _fbxJoint->inverseBindRotation;
}

void JointState::restoreRotation(float fraction, float priority) {
    assert(_fbxJoint != NULL);
    if (priority == _animationPriority || _animationPriority == 0.0f) {
        _rotationInParentFrame = safeMix(_rotationInParentFrame, _fbxJoint->rotation, fraction);
        _animationPriority = 0.0f;
    }
}

void JointState::setRotationFromBindFrame(const glm::quat& rotation, float priority) {
    assert(_fbxJoint != NULL);
    if (priority >= _animationPriority) {
        // rotation is from bind- to model-frame
        _rotationInParentFrame = _rotationInParentFrame * glm::inverse(_rotation) * rotation * glm::inverse(_fbxJoint->inverseBindRotation);
        _animationPriority = priority;
    }
}

void JointState::clearTransformTranslation() {
    _transform[3][0] = 0.0f;
    _transform[3][1] = 0.0f;
    _transform[3][2] = 0.0f;
}

void JointState::setRotation(const glm::quat& rotation, bool constrain, float priority) {
    applyRotationDelta(rotation * glm::inverse(_rotation), true, priority);
}

void JointState::applyRotationDelta(const glm::quat& delta, bool constrain, float priority) {
    // NOTE: delta is in jointParent-frame
    assert(_fbxJoint != NULL);
    if (priority < _animationPriority) {
        return;
    }
    _animationPriority = priority;
    if (!constrain || _constraint == NULL) {
        // no constraints
        _rotationInParentFrame = _rotationInParentFrame * glm::inverse(_rotation) * delta * _rotation;
        _rotation = delta * _rotation;
        return;
    }
    glm::quat targetRotation = delta * _rotation;
    glm::vec3 eulers = safeEulerAngles(_rotationInParentFrame * glm::inverse(_rotation) * targetRotation);
    glm::quat newRotation = glm::quat(glm::clamp(eulers, _fbxJoint->rotationMin, _fbxJoint->rotationMax));
    _rotation = _rotation * glm::inverse(_rotationInParentFrame) * newRotation;
    _rotationInParentFrame = newRotation;
}

const glm::vec3& JointState::getDefaultTranslationInParentFrame() const {
    assert(_fbxJoint != NULL);
    return _fbxJoint->translation;
}
