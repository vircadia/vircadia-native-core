//
//  AnimOverlay.cpp
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimOverlay.h"
#include "AnimUtil.h"
#include <queue>

AnimOverlay::AnimOverlay(const std::string& id, BoneSet boneSet) :
    AnimNode(AnimNode::OverlayType, id), _boneSet(boneSet) {
}

AnimOverlay::~AnimOverlay() {

}

void AnimOverlay::setBoneSet(BoneSet boneSet) {
    _boneSet = boneSet;
    assert(_skeleton);
    switch (boneSet) {
    case FullBodyBoneSet: buildFullBodyBoneSet(); break;
    case UpperBodyBoneSet: buildUpperBodyBoneSet(); break;
    case LowerBodyBoneSet: buildLowerBodyBoneSet(); break;
    case RightArmBoneSet: buildRightArmBoneSet(); break;
    case LeftArmBoneSet: buildLeftArmBoneSet(); break;
    case AboveTheHeadBoneSet: buildAboveTheHeadBoneSet(); break;
    case BelowTheHeadBoneSet: buildBelowTheHeadBoneSet(); break;
    case HeadOnlyBoneSet: buildHeadOnlyBoneSet(); break;
    case SpineOnlyBoneSet: buildSpineOnlyBoneSet(); break;
    default:
    case EmptyBoneSet: buildEmptyBoneSet(); break;
    }
}

const std::vector<AnimPose>& AnimOverlay::evaluate(const AnimVariantMap& animVars, float dt) {

    if (_children.size() >= 2) {
        auto underPoses = _children[1]->evaluate(animVars, dt);
        auto overPoses = _children[0]->overlay(animVars, dt, underPoses);

        if (underPoses.size() > 0 && underPoses.size() == overPoses.size()) {
            _poses.resize(underPoses.size());
            assert(_boneSetVec.size() == _poses.size());

            for (size_t i = 0; i < _poses.size(); i++) {
                float alpha = _boneSetVec[i];
                blend(1, &underPoses[i], &overPoses[i], alpha, &_poses[i]);
            }
        }
    }
    return _poses;
}

template <typename Func>
void for_each_child_joint(AnimSkeleton::ConstPointer skeleton, int startJoint, Func f) {
    std::queue<int> q;
    q.push(startJoint);
    while(q.size() > 0) {
        int jointIndex = q.front();
        for (int i = 0; i < skeleton->getNumJoints(); i++) {
            if (jointIndex == skeleton->getParentIndex(i)) {
                f(i);
                q.push(i);
            }
        }
        q.pop();
    }
}

void AnimOverlay::buildFullBodyBoneSet() {
    assert(_skeleton);
    _boneSetVec.resize(_skeleton->getNumJoints());
    for (int i = 0; i < _skeleton->getNumJoints(); i++) {
        _boneSetVec[i] = 1.0f;
    }
}

void AnimOverlay::buildUpperBodyBoneSet() {
    assert(_skeleton);
    buildEmptyBoneSet();
    int spineJoint = _skeleton->nameToJointIndex("Spine");
    for_each_child_joint(_skeleton, spineJoint, [&](int i) {
        _boneSetVec[i] = 1.0f;
    });
}

void AnimOverlay::buildLowerBodyBoneSet() {
    assert(_skeleton);
    buildFullBodyBoneSet();
    int hipsJoint = _skeleton->nameToJointIndex("Hips");
    int spineJoint = _skeleton->nameToJointIndex("Spine");
    _boneSetVec.resize(_skeleton->getNumJoints());
    for_each_child_joint(_skeleton, spineJoint, [&](int i) {
        _boneSetVec[i] = 0.0f;
    });
    _boneSetVec[hipsJoint] = 0.0f;
}

void AnimOverlay::buildRightArmBoneSet() {
    assert(_skeleton);
    buildEmptyBoneSet();
    int rightShoulderJoint = _skeleton->nameToJointIndex("RightShoulder");
    for_each_child_joint(_skeleton, rightShoulderJoint, [&](int i) {
        _boneSetVec[i] = 1.0f;
    });
}

void AnimOverlay::buildLeftArmBoneSet() {
    assert(_skeleton);
    buildEmptyBoneSet();
    int leftShoulderJoint = _skeleton->nameToJointIndex("LeftShoulder");
    for_each_child_joint(_skeleton, leftShoulderJoint, [&](int i) {
        _boneSetVec[i] = 1.0f;
    });
}

void AnimOverlay::buildAboveTheHeadBoneSet() {
    assert(_skeleton);
    buildEmptyBoneSet();
    int headJoint = _skeleton->nameToJointIndex("Head");
    for_each_child_joint(_skeleton, headJoint, [&](int i) {
        _boneSetVec[i] = 1.0f;
    });
}

void AnimOverlay::buildBelowTheHeadBoneSet() {
    assert(_skeleton);
    buildFullBodyBoneSet();
    int headJoint = _skeleton->nameToJointIndex("Head");
    for_each_child_joint(_skeleton, headJoint, [&](int i) {
        _boneSetVec[i] = 0.0f;
    });
}

void AnimOverlay::buildHeadOnlyBoneSet() {
    assert(_skeleton);
    buildEmptyBoneSet();
    int headJoint = _skeleton->nameToJointIndex("Head");
    _boneSetVec[headJoint] = 1.0f;
}

void AnimOverlay::buildSpineOnlyBoneSet() {
    assert(_skeleton);
    buildEmptyBoneSet();
    int spineJoint = _skeleton->nameToJointIndex("Spine");
    _boneSetVec[spineJoint] = 1.0f;
}

void AnimOverlay::buildEmptyBoneSet() {
    assert(_skeleton);
    _boneSetVec.resize(_skeleton->getNumJoints());
    for (int i = 0; i < _skeleton->getNumJoints(); i++) {
        _boneSetVec[i] = 0.0f;
    }
}

// for AnimDebugDraw rendering
const std::vector<AnimPose>& AnimOverlay::getPosesInternal() const {
    return _poses;
}

void AnimOverlay::setSkeletonInternal(AnimSkeleton::ConstPointer skeleton) {
    _skeleton = skeleton;

    // we have to re-build the bone set when the skeleton changes.
    setBoneSet(_boneSet);
}
