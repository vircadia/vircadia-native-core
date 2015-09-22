//
//  AnimNode.cpp
//
//  Created by Anthony J. Thibault on 9/2/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimNode.h"

void AnimNode::removeChild(AnimNode::Pointer child) {
    auto iter = std::find(_children.begin(), _children.end(), child);
    if (iter != _children.end()) {
        _children.erase(iter);
    }
}

AnimNode::Pointer AnimNode::getChild(int i) const {
    assert(i >= 0 && i < (int)_children.size());
    return _children[i];
}

void AnimNode::setSkeleton(const AnimSkeleton::Pointer skeleton) {
    setSkeletonInternal(skeleton);
    for (auto&& child : _children) {
        child->setSkeleton(skeleton);
    }
}

const AnimPose AnimNode::getRootPose(int jointIndex) const {
    AnimPose pose = AnimPose::identity;
    if (_skeleton && jointIndex != -1) {
        const AnimPoseVec& poses = getPosesInternal();
        int numJoints = (int)(poses.size());
        if (jointIndex < numJoints) {
            int parentIndex = _skeleton->getParentIndex(jointIndex);
            while (parentIndex != -1 && parentIndex < numJoints) {
                jointIndex = parentIndex;
                parentIndex = _skeleton->getParentIndex(jointIndex);
            }
            pose = poses[jointIndex];
        }
    }
    return pose;
}

void AnimNode::setCurrentFrame(float frame) {
    setCurrentFrameInternal(frame);
    for (auto&& child : _children) {
        child->setCurrentFrameInternal(frame);
    }
}
