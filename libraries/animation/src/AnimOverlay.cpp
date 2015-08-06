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

AnimOverlay::AnimOverlay(const std::string& id, BoneSet boneSet) :
    AnimNode(AnimNode::OverlayType, id),
    _boneSet(boneSet) {
}

AnimOverlay::~AnimOverlay() {
}

const std::vector<AnimPose>& AnimOverlay::evaluate(float dt) {
    if (_children.size() >= 2) {
        auto underPoses = _children[1]->evaluate(dt);
        auto overPoses = _children[0]->overlay(dt, underPoses);

        if (underPoses.size() > 0 && underPoses.size() == overPoses.size()) {
            _poses.resize(underPoses.size());

            for (size_t i = 0; i < _poses.size(); i++) {
                float alpha = 1.0f;  // TODO: PULL from boneSet
                blend(1, &underPoses[i], &overPoses[i], alpha, &_poses[i]);
            }
        }
    }
    return _poses;
}
