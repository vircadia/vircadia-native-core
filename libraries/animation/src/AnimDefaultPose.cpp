//
//  AnimDefaultPose.cpp
//
//  Created by Anthony J. Thibault on 6/26/17.
//  Copyright (c) 2017 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimDefaultPose.h"

AnimDefaultPose::AnimDefaultPose(const QString& id) :
    AnimNode(AnimNode::Type::DefaultPose, id)
{

}

AnimDefaultPose::~AnimDefaultPose() {

}

const AnimPoseVec& AnimDefaultPose::evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, Triggers& triggersOut) {
    if (_skeleton) {
        _poses = _skeleton->getRelativeDefaultPoses();
    } else {
        _poses.clear();
    }
    return _poses;
}

const AnimPoseVec& AnimDefaultPose::getPosesInternal() const {
    return _poses;
}
