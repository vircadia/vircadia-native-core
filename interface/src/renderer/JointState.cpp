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
    _transform = other._transform;
    _rotation = other._rotation;
    _rotationInParentFrame = other._rotationInParentFrame;
    _animationPriority = other._animationPriority;
    _fbxJoint = other._fbxJoint;
    // DO NOT copy _constraint
}

JointState::~JointState() {
    delete _constraint;
    _constraint = NULL;
    if (_constraint) {
        delete _constraint;
        _constraint = NULL;
    }
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
    _animationPriority = state._animationPriority;
    _transform = state._transform;
    _rotation = extractRotation(_transform);
    _rotationInParentFrame = state._rotationInParentFrame;

    _visibleTransform = state._visibleTransform;
    _visibleRotation = extractRotation(_visibleTransform);
    _visibleRotationInParentFrame = state._visibleRotationInParentFrame;
    // DO NOT copy _fbxJoint or _constraint
}

void JointState::computeTransform(const glm::mat4& parentTransform) {
    glm::quat modifiedRotation = _fbxJoint->preRotation * _rotationInParentFrame * _fbxJoint->postRotation;
    glm::mat4 modifiedTransform = _fbxJoint->preTransform * glm::mat4_cast(modifiedRotation) * _fbxJoint->postTransform;
    _transform = parentTransform * glm::translate(_fbxJoint->translation) * modifiedTransform;
    _rotation = extractRotation(_transform);
}

void JointState::computeVisibleTransform(const glm::mat4& parentTransform) {
    glm::quat modifiedRotation = _fbxJoint->preRotation * _visibleRotationInParentFrame * _fbxJoint->postRotation;
    glm::mat4 modifiedTransform = _fbxJoint->preTransform * glm::mat4_cast(modifiedRotation) * _fbxJoint->postTransform;
    _visibleTransform = parentTransform * glm::translate(_fbxJoint->translation) * modifiedTransform;
    _visibleRotation = extractRotation(_visibleTransform);
}

glm::quat JointState::getRotationFromBindToModelFrame() const {
    return _rotation * _fbxJoint->inverseBindRotation;
}

void JointState::restoreRotation(float fraction, float priority) {
    assert(_fbxJoint != NULL);
    if (priority == _animationPriority || _animationPriority == 0.0f) {
        setRotationInParentFrame(safeMix(_rotationInParentFrame, _fbxJoint->rotation, fraction));
        _animationPriority = 0.0f;
    }
}

void JointState::setRotationFromBindFrame(const glm::quat& rotation, float priority, bool constrain) {
    // rotation is from bind- to model-frame
    assert(_fbxJoint != NULL);
    if (priority >= _animationPriority) {
        glm::quat targetRotation = _rotationInParentFrame * glm::inverse(_rotation) * rotation * glm::inverse(_fbxJoint->inverseBindRotation);
        if (constrain && _constraint) {
            _constraint->softClamp(targetRotation, _rotationInParentFrame, 0.5f);
        }
        setRotationInParentFrame(targetRotation);
        _animationPriority = priority;
    }
}

void JointState::clearTransformTranslation() {
    _transform[3][0] = 0.0f;
    _transform[3][1] = 0.0f;
    _transform[3][2] = 0.0f;
    _visibleTransform[3][0] = 0.0f;
    _visibleTransform[3][1] = 0.0f;
    _visibleTransform[3][2] = 0.0f;
}

void JointState::setRotation(const glm::quat& rotation, bool constrain, float priority) {
    applyRotationDelta(rotation * glm::inverse(_rotation), true, priority);
}

void JointState::applyRotationDelta(const glm::quat& delta, bool constrain, float priority) {
    // NOTE: delta is in model-frame
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
    glm::quat targetRotation = _rotationInParentFrame * glm::inverse(_rotation) * delta * _rotation;
    setRotationInParentFrame(targetRotation);
}

/// Applies delta rotation to joint but mixes a little bit of the default pose as well.
/// This helps keep an IK solution stable.
void JointState::mixRotationDelta(const glm::quat& delta, float mixFactor, float priority) {
    // NOTE: delta is in model-frame
    assert(_fbxJoint != NULL);
    if (priority < _animationPriority) {
        return;
    }
    _animationPriority = priority;
    glm::quat targetRotation = _rotationInParentFrame * glm::inverse(_rotation) * delta * _rotation;
    if (mixFactor > 0.0f && mixFactor <= 1.0f) {
        targetRotation = safeMix(targetRotation, _fbxJoint->rotation, mixFactor);
    }
    if (_constraint) {
        _constraint->softClamp(targetRotation, _rotationInParentFrame, 0.5f);
    }
    setRotationInParentFrame(targetRotation);
}

glm::quat JointState::computeParentRotation() const {
    // R = Rp * Rpre * r * Rpost
    // Rp = R * (Rpre * r * Rpost)^
    return _rotation * glm::inverse(_fbxJoint->preRotation * _rotationInParentFrame * _fbxJoint->postRotation);
}

void JointState::setRotationInParentFrame(const glm::quat& targetRotation) {
    glm::quat parentRotation = computeParentRotation();
    _rotationInParentFrame = targetRotation;
    // R' = Rp * Rpre * r' * Rpost
    _rotation = parentRotation * _fbxJoint->preRotation * _rotationInParentFrame * _fbxJoint->postRotation;
}

const glm::vec3& JointState::getDefaultTranslationInParentFrame() const {
    assert(_fbxJoint != NULL);
    return _fbxJoint->translation;
}

void JointState::slaveVisibleTransform() {
    _visibleTransform = _transform;
    _visibleRotation = _rotation;
    _visibleRotationInParentFrame = _rotationInParentFrame;
}
