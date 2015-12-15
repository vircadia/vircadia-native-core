//
//  MotionTracker.cpp
//  interface/src/devices
//
//  Created by Sam Cake on 6/20/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MotionTracker.h"
#include "GLMHelpers.h"


// glm::mult(mat43, mat43) just the composition of the 2 matrices assuming they are in fact mat44 with the last raw = { 0, 0, 0, 1 }
namespace glm {
    mat4x3 mult(const mat4& lhs, const mat4x3& rhs) {
        vec3 lrx(lhs[0].x, lhs[1].x, lhs[2].x);
        vec3 lry(lhs[0].y, lhs[1].y, lhs[2].y);
        vec3 lrz(lhs[0].z, lhs[1].z, lhs[2].z);
        return mat4x3(
            dot(lrx, rhs[0]),
            dot(lry, rhs[0]),
            dot(lrz, rhs[0]),

            dot(lrx, rhs[1]),
            dot(lry, rhs[1]),
            dot(lrz, rhs[1]),

            dot(lrx, rhs[2]),
            dot(lry, rhs[2]),
            dot(lrz, rhs[2]),

            dot(lrx, rhs[3]) + lhs[3].x,
            dot(lry, rhs[3]) + lhs[3].y,
            dot(lrz, rhs[3]) + lhs[3].z
       );
    }
    mat4x3 mult(const mat4x3& lhs, const mat4x3& rhs) {
        vec3 lrx(lhs[0].x, lhs[1].x, lhs[2].x);
        vec3 lry(lhs[0].y, lhs[1].y, lhs[2].y);
        vec3 lrz(lhs[0].z, lhs[1].z, lhs[2].z);
        return mat4x3(
            dot(lrx, rhs[0]),
            dot(lry, rhs[0]),
            dot(lrz, rhs[0]),

            dot(lrx, rhs[1]),
            dot(lry, rhs[1]),
            dot(lrz, rhs[1]),

            dot(lrx, rhs[2]),
            dot(lry, rhs[2]),
            dot(lrz, rhs[2]),

            dot(lrx, rhs[3]) + lhs[3].x,
            dot(lry, rhs[3]) + lhs[3].y,
            dot(lrz, rhs[3]) + lhs[3].z
       );
    }
}

// MotionTracker
MotionTracker::MotionTracker() :
    DeviceTracker()
{
    _jointsArray.resize(1);
    _jointsMap.insert(JointTracker::Map::value_type(Semantic("Root"), 0));
}

MotionTracker::~MotionTracker()
{
}

bool MotionTracker::isConnected() const {
    return false;
}

MotionTracker::Index MotionTracker::addJoint(const Semantic& semantic, Index parent) {
    // Check the parent
    if (int(parent) < 0)
        return INVALID_PARENT;

    // Check that the semantic is not already in use
    Index foundIndex = findJointIndex(semantic);
    if (foundIndex >= 0) {
        return INVALID_SEMANTIC;
    }
    

    // All good then allocate the joint
    Index newIndex = (Index)_jointsArray.size();
    _jointsArray.push_back(JointTracker(semantic, parent));
    _jointsMap.insert(JointTracker::Map::value_type(semantic, newIndex));

    return newIndex;
}

MotionTracker::Index MotionTracker::findJointIndex(const Semantic& semantic) const {
    // TODO C++11 auto jointIt = _jointsMap.find(semantic);
    JointTracker::Map::const_iterator jointIt = _jointsMap.find(semantic);
    if (jointIt != _jointsMap.end()) {
        return (*jointIt).second;
    }
    
    return INVALID_SEMANTIC;
}

void MotionTracker::updateAllAbsTransform() {
    _jointsArray[0].updateAbsFromLocTransform(0);

    // Because we know the hierarchy is stored from root down the branches let's just traverse and update
    for (Index i = 1; i < (Index)(_jointsArray.size()); i++) {
        JointTracker* joint = _jointsArray.data() + i;
        joint->updateAbsFromLocTransform(_jointsArray.data() + joint->getParent());
    }
}


// MotionTracker::JointTracker
MotionTracker::JointTracker::JointTracker() :
    _locFrame(),
    _absFrame(),
    _semantic(""),
    _parent(INVALID_PARENT),
    _lastUpdate(1)  // Joint inactive
{
}

MotionTracker::JointTracker::JointTracker(const Semantic& semantic, Index parent) :
    _semantic(semantic),
    _parent(parent),
    _lastUpdate(1)  // Joint inactive
{
}

MotionTracker::JointTracker::JointTracker(const JointTracker& tracker) :
    _locFrame(tracker._locFrame),
    _absFrame(tracker._absFrame),
    _semantic(tracker._semantic),
    _parent(tracker._parent),
    _lastUpdate(tracker._lastUpdate)
{
}

void MotionTracker::JointTracker::updateAbsFromLocTransform(const JointTracker* parentJoint) {
    if (parentJoint) {
        editAbsFrame()._transform = (parentJoint->getAbsFrame()._transform * getLocFrame()._transform);
    } else {
        editAbsFrame()._transform = getLocFrame()._transform;
    }
}

void MotionTracker::JointTracker::updateLocFromAbsTransform(const JointTracker* parentJoint) {
    if (parentJoint) {
        glm::mat4 ip = glm::inverse(parentJoint->getAbsFrame()._transform);
        editLocFrame()._transform = (ip * getAbsFrame()._transform);
    } else {
        editLocFrame()._transform = getAbsFrame()._transform;
    }
}

//--------------------------------------------------------------------------------------
// MotionTracker::Frame
//--------------------------------------------------------------------------------------

MotionTracker::Frame::Frame() :
    _transform()
{
}

void MotionTracker::Frame::setRotation(const glm::quat& rotation) {
    glm::mat3x3 rot = glm::mat3_cast(rotation);
    _transform[0] = glm::vec4(rot[0], 0.0f);
    _transform[1] = glm::vec4(rot[1], 0.0f);
    _transform[2] = glm::vec4(rot[2], 0.0f);
}

void MotionTracker::Frame::getRotation(glm::quat& rotation) const {
    rotation = glm::quat_cast(_transform);
}

void MotionTracker::Frame::setTranslation(const glm::vec3& translation) {
    _transform[3] = glm::vec4(translation, 1.0f);
}

void MotionTracker::Frame::getTranslation(glm::vec3& translation) const {
    translation = extractTranslation(_transform);
}

