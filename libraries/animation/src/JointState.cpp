//
//  JointState.cpp
//  libraries/animation/src/
//
//  Created by Andrzej Kapolka on 10/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/norm.hpp>

#include <QThreadPool>

#include <SharedUtil.h>

#include "JointState.h"

JointState::~JointState() {
}

void JointState::copyState(const JointState& other) {
    _transformChanged = other._transformChanged;
    _transform = other._transform;
    _rotationIsValid = other._rotationIsValid;
    _rotation = other._rotation;
    _rotationInConstrainedFrame = other._rotationInConstrainedFrame;
    _positionInParentFrame = other._positionInParentFrame;
    _distanceToParent = other._distanceToParent;
    _animationPriority = other._animationPriority;
    
    _name = other._name;
    _isFree = other._isFree;
    _parentIndex = other._parentIndex;
    _defaultRotation = other._defaultRotation;
    _inverseDefaultRotation = other._inverseDefaultRotation;
    _translation = other._translation;
    _preRotation = other._preRotation;
    _postRotation = other._postRotation;
    _preTransform = other._preTransform;
    _postTransform = other._postTransform;
    _inverseBindRotation = other._inverseBindRotation;
}
JointState::JointState(const FBXJoint& joint) {
    _rotationInConstrainedFrame = joint.rotation;
    _name = joint.name;
    _isFree = joint.isFree;
    _parentIndex = joint.parentIndex;
    _translation = joint.translation;
    _defaultRotation = joint.rotation;
    _inverseDefaultRotation = joint.inverseDefaultRotation;
    _preRotation = joint.preRotation;
    _postRotation = joint.postRotation;
    _preTransform = joint.preTransform;
    _postTransform = joint.postTransform;
    _inverseBindRotation = joint.inverseBindRotation;
}

void JointState::buildConstraint() {
}

glm::quat JointState::getRotation() const {
    if (!_rotationIsValid) {
        const_cast<JointState*>(this)->_rotation = extractRotation(_transform);
        const_cast<JointState*>(this)->_rotationIsValid = true;
    }
    
    return _rotation;
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
    
    glm::quat rotationInParentFrame = _preRotation * _rotationInConstrainedFrame * _postRotation;
    glm::mat4 transformInParentFrame = _preTransform * glm::mat4_cast(rotationInParentFrame) * _postTransform;
    glm::mat4 newTransform = parentTransform * glm::translate(_translation) * transformInParentFrame;
    
    if (newTransform != _transform) {
        _transform = newTransform;
        _transformChanged = true;
        _rotationIsValid = false;
    }
}

glm::quat JointState::getRotationInBindFrame() const {
    return getRotation() * _inverseBindRotation;
}

glm::quat JointState::getRotationInParentFrame() const {
    return _preRotation * _rotationInConstrainedFrame * _postRotation;
}

void JointState::restoreRotation(float fraction, float priority) {
    if (priority == _animationPriority || _animationPriority == 0.0f) {
        setRotationInConstrainedFrameInternal(safeMix(_rotationInConstrainedFrame, _defaultRotation, fraction));
        _animationPriority = 0.0f;
    }
}

void JointState::setRotationInBindFrame(const glm::quat& rotation, float priority) {
    // rotation is from bind- to model-frame
    if (priority >= _animationPriority) {
        glm::quat targetRotation = _rotationInConstrainedFrame * glm::inverse(getRotation()) * rotation * glm::inverse(_inverseBindRotation);
        setRotationInConstrainedFrameInternal(targetRotation);
        _animationPriority = priority;
    }
}

void JointState::setRotationInModelFrame(const glm::quat& rotationInModelFrame, float priority) {
    // rotation is from bind- to model-frame
    if (priority >= _animationPriority) {
        glm::quat parentRotation = computeParentRotation();

        // R = Rp * Rpre * r * Rpost
        // R' = Rp * Rpre * r' * Rpost
        // r' = (Rp * Rpre)^ * R' * Rpost^
        glm::quat targetRotation = glm::inverse(parentRotation * _preRotation) * rotationInModelFrame * glm::inverse(_postRotation);
        _rotationInConstrainedFrame = glm::normalize(targetRotation);
        _transformChanged = true;
        _animationPriority = priority;
    }
}

void JointState::clearTransformTranslation() {
    _transform[3][0] = 0.0f;
    _transform[3][1] = 0.0f;
    _transform[3][2] = 0.0f;
    _transformChanged = true;
}

void JointState::applyRotationDelta(const glm::quat& delta, float priority) {
    // NOTE: delta is in model-frame
    if (priority < _animationPriority || delta == glm::quat()) {
        return;
    }
    _animationPriority = priority;
    glm::quat targetRotation = _rotationInConstrainedFrame * glm::inverse(getRotation()) * delta * getRotation();
    setRotationInConstrainedFrameInternal(targetRotation);
}

/// Applies delta rotation to joint but mixes a little bit of the default pose as well.
/// This helps keep an IK solution stable.
void JointState::mixRotationDelta(const glm::quat& delta, float mixFactor, float priority) {
    // NOTE: delta is in model-frame
    if (priority < _animationPriority) {
        return;
    }
    _animationPriority = priority;
    glm::quat targetRotation = _rotationInConstrainedFrame * glm::inverse(getRotation()) * delta * getRotation();
    if (mixFactor > 0.0f && mixFactor <= 1.0f) {
        targetRotation = safeMix(targetRotation, _defaultRotation, mixFactor);
    }
    setRotationInConstrainedFrameInternal(targetRotation);
}

glm::quat JointState::computeParentRotation() const {
    // R = Rp * Rpre * r * Rpost
    // Rp = R * (Rpre * r * Rpost)^
    return getRotation() * glm::inverse(_preRotation * _rotationInConstrainedFrame * _postRotation);
}

void JointState::setRotationInConstrainedFrame(glm::quat targetRotation, float priority, float mix) {
    if (priority >= _animationPriority || _animationPriority == 0.0f) {
        auto rotation = (mix == 1.0f) ? targetRotation : safeMix(getRotationInConstrainedFrame(), targetRotation, mix);
        setRotationInConstrainedFrameInternal(rotation);
        _animationPriority = priority;
    }
}

void JointState::setRotationInConstrainedFrameInternal(const glm::quat& targetRotation) {
    if (_rotationInConstrainedFrame != targetRotation) {
        glm::quat parentRotation = computeParentRotation();
        _rotationInConstrainedFrame = targetRotation;
        _transformChanged = true;
        // R' = Rp * Rpre * r' * Rpost
        _rotation = parentRotation * _preRotation * _rotationInConstrainedFrame * _postRotation;
    }
}

bool JointState::rotationIsDefault(const glm::quat& rotation, float tolerance) const {
    glm::quat defaultRotation = _defaultRotation;
    return glm::abs(rotation.x - defaultRotation.x) < tolerance &&
        glm::abs(rotation.y - defaultRotation.y) < tolerance &&
        glm::abs(rotation.z - defaultRotation.z) < tolerance &&
        glm::abs(rotation.w - defaultRotation.w) < tolerance;
}

glm::quat JointState::getDefaultRotationInParentFrame() const {
    // NOTE: the result is constant and could be cached
    return _preRotation * _defaultRotation * _postRotation;
}

const glm::vec3& JointState::getDefaultTranslationInConstrainedFrame() const {
    return _translation;
}
