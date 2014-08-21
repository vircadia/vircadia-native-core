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

#include <QThreadPool>

#include <AngularConstraint.h>
#include <SharedUtil.h>

#include "JointState.h"

JointState::JointState() :
    _animationPriority(0.0f),
    _transformChanged(true),
    _rotationIsValid(false),
    _positionInParentFrame(0.0f),
    _distanceToParent(0.0f),
    _fbxJoint(NULL),
    _constraint(NULL) {
}

JointState::JointState(const JointState& other) : _constraint(NULL) {
    _transformChanged = other._transformChanged;
    _transform = other._transform;
    _rotationIsValid = other._rotationIsValid;
    _rotation = other._rotation;
    _rotationInConstrainedFrame = other._rotationInConstrainedFrame;
    _positionInParentFrame = other._positionInParentFrame;
    _distanceToParent = other._distanceToParent;
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

glm::quat JointState::getRotation() const {
    if (!_rotationIsValid) {
        const_cast<JointState*>(this)->_rotation = extractRotation(_transform);
        const_cast<JointState*>(this)->_rotationIsValid = true;
    }
    
    return _rotation;
}

void JointState::setFBXJoint(const FBXJoint* joint) {
    assert(joint != NULL);
    _rotationInConstrainedFrame = joint->rotation;
    _transformChanged = true;
    _rotationIsValid = false;
    
    // NOTE: JointState does not own the FBXJoint to which it points.
    _fbxJoint = joint;
    if (_constraint) {
        delete _constraint;
        _constraint = NULL;
    }
}

void JointState::buildConstraint() {
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
    _transformChanged = state._transformChanged;
    _transform = state._transform;
    _rotationIsValid = state._rotationIsValid;
    _rotation = state._rotation;
    _rotationInConstrainedFrame = state._rotationInConstrainedFrame;
    _positionInParentFrame = state._positionInParentFrame;
    _distanceToParent = state._distanceToParent;

    _visibleTransform = state._visibleTransform;
    _visibleRotation = extractRotation(_visibleTransform);
    _visibleRotationInConstrainedFrame = state._visibleRotationInConstrainedFrame;
    // DO NOT copy _fbxJoint or _constraint
}

void JointState::initTransform(const glm::mat4& parentTransform) {
    computeTransform(parentTransform);
    _positionInParentFrame = glm::inverse(extractRotation(parentTransform)) * (extractTranslation(_transform) - extractTranslation(parentTransform));
    _distanceToParent = glm::length(_positionInParentFrame);
}

void JointState::computeTransform(const glm::mat4& parentTransform, bool parentTransformChanged, bool synchronousRotationCompute) {
    if (!parentTransformChanged && !_transformChanged) {
        return;
    }
    
    glm::quat rotationInParentFrame = _fbxJoint->preRotation * _rotationInConstrainedFrame * _fbxJoint->postRotation;
    glm::mat4 transformInParentFrame = _fbxJoint->preTransform * glm::mat4_cast(rotationInParentFrame) * _fbxJoint->postTransform;
    glm::mat4 newTransform = parentTransform * glm::translate(_fbxJoint->translation) * transformInParentFrame;
    
    if (newTransform != _transform) {
        _transform = newTransform;
        _transformChanged = true;
        _rotationIsValid = false;
    }
}

void JointState::computeVisibleTransform(const glm::mat4& parentTransform) {
    glm::quat rotationInParentFrame = _fbxJoint->preRotation * _visibleRotationInConstrainedFrame * _fbxJoint->postRotation;
    glm::mat4 transformInParentFrame = _fbxJoint->preTransform * glm::mat4_cast(rotationInParentFrame) * _fbxJoint->postTransform;
    _visibleTransform = parentTransform * glm::translate(_fbxJoint->translation) * transformInParentFrame;
    _visibleRotation = extractRotation(_visibleTransform);
}

glm::quat JointState::getRotationInBindFrame() const {
    return getRotation() * _fbxJoint->inverseBindRotation;
}

glm::quat JointState::getRotationInParentFrame() const {
    return _fbxJoint->preRotation * _rotationInConstrainedFrame * _fbxJoint->postRotation;
}

glm::quat JointState::getVisibleRotationInParentFrame() const {
    return _fbxJoint->preRotation * _visibleRotationInConstrainedFrame * _fbxJoint->postRotation;
}

void JointState::restoreRotation(float fraction, float priority) {
    assert(_fbxJoint != NULL);
    if (priority == _animationPriority || _animationPriority == 0.0f) {
        setRotationInConstrainedFrame(safeMix(_rotationInConstrainedFrame, _fbxJoint->rotation, fraction));
        _animationPriority = 0.0f;
    }
}

void JointState::setRotationInBindFrame(const glm::quat& rotation, float priority, bool constrain) {
    // rotation is from bind- to model-frame
    assert(_fbxJoint != NULL);
    if (priority >= _animationPriority) {
        glm::quat targetRotation = _rotationInConstrainedFrame * glm::inverse(getRotation()) * rotation * glm::inverse(_fbxJoint->inverseBindRotation);
        if (constrain && _constraint) {
            _constraint->softClamp(targetRotation, _rotationInConstrainedFrame, 0.5f);
        }
        setRotationInConstrainedFrame(targetRotation);
        _animationPriority = priority;
    }
}

void JointState::clearTransformTranslation() {
    _transform[3][0] = 0.0f;
    _transform[3][1] = 0.0f;
    _transform[3][2] = 0.0f;
    _transformChanged = true;
    _visibleTransform[3][0] = 0.0f;
    _visibleTransform[3][1] = 0.0f;
    _visibleTransform[3][2] = 0.0f;
}

void JointState::applyRotationDelta(const glm::quat& delta, bool constrain, float priority) {
    // NOTE: delta is in model-frame
    assert(_fbxJoint != NULL);
    if (priority < _animationPriority || delta.null) {
        return;
    }
    _animationPriority = priority;
    glm::quat targetRotation = _rotationInConstrainedFrame * glm::inverse(getRotation()) * delta * getRotation();
    if (!constrain || _constraint == NULL) {
        // no constraints
        _rotationInConstrainedFrame = targetRotation;
        _transformChanged = true;
        
        _rotation = delta * getRotation();
        return;
    }
    setRotationInConstrainedFrame(targetRotation);
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
    glm::quat targetRotation = _rotationInConstrainedFrame * glm::inverse(getRotation()) * delta * getRotation();
    if (mixFactor > 0.0f && mixFactor <= 1.0f) {
        targetRotation = safeMix(targetRotation, _fbxJoint->rotation, mixFactor);
    }
    if (_constraint) {
        _constraint->softClamp(targetRotation, _rotationInConstrainedFrame, 0.5f);
    }
    setRotationInConstrainedFrame(targetRotation);
}

void JointState::mixVisibleRotationDelta(const glm::quat& delta, float mixFactor) {
    // NOTE: delta is in model-frame
    assert(_fbxJoint != NULL);
    glm::quat targetRotation = _visibleRotationInConstrainedFrame * glm::inverse(_visibleRotation) * delta * _visibleRotation;
    if (mixFactor > 0.0f && mixFactor <= 1.0f) {
        //targetRotation = safeMix(targetRotation, _fbxJoint->rotation, mixFactor);
        targetRotation = safeMix(targetRotation, _rotationInConstrainedFrame, mixFactor);
    }
    setVisibleRotationInConstrainedFrame(targetRotation);
}

glm::quat JointState::computeParentRotation() const {
    // R = Rp * Rpre * r * Rpost
    // Rp = R * (Rpre * r * Rpost)^
    return getRotation() * glm::inverse(_fbxJoint->preRotation * _rotationInConstrainedFrame * _fbxJoint->postRotation);
}

glm::quat JointState::computeVisibleParentRotation() const {
    return _visibleRotation * glm::inverse(_fbxJoint->preRotation * _visibleRotationInConstrainedFrame * _fbxJoint->postRotation);
}

void JointState::setRotationInConstrainedFrame(const glm::quat& targetRotation) {
    glm::quat parentRotation = computeParentRotation();
    _rotationInConstrainedFrame = targetRotation;
    _transformChanged = true;
    // R' = Rp * Rpre * r' * Rpost
    _rotation = parentRotation * _fbxJoint->preRotation * _rotationInConstrainedFrame * _fbxJoint->postRotation;
}

void JointState::setVisibleRotationInConstrainedFrame(const glm::quat& targetRotation) {
    glm::quat parentRotation = computeVisibleParentRotation();
    _visibleRotationInConstrainedFrame = targetRotation;
    _visibleRotation = parentRotation * _fbxJoint->preRotation * _visibleRotationInConstrainedFrame * _fbxJoint->postRotation;
}

const bool JointState::rotationIsDefault(const glm::quat& rotation, float tolerance) const {
    glm::quat defaultRotation = _fbxJoint->rotation;
    return glm::abs(rotation.x - defaultRotation.x) < tolerance &&
        glm::abs(rotation.y - defaultRotation.y) < tolerance &&
        glm::abs(rotation.z - defaultRotation.z) < tolerance &&
        glm::abs(rotation.w - defaultRotation.w) < tolerance;
}

const glm::vec3& JointState::getDefaultTranslationInConstrainedFrame() const {
    assert(_fbxJoint != NULL);
    return _fbxJoint->translation;
}

void JointState::slaveVisibleTransform() {
    _visibleTransform = _transform;
    _visibleRotation = getRotation();
    _visibleRotationInConstrainedFrame = _rotationInConstrainedFrame;
}
