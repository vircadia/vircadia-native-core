//
//  AnimController.h
//
//  Created by Anthony J. Thibault on 9/8/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimController_h
#define hifi_AnimController_h

#include "AnimNode.h"

// Allows procedural control over a set of joints.

class AnimController : public AnimNode {
public:
    friend class AnimTests;

    AnimController(const std::string& id, float alpha);
    virtual ~AnimController() override;

    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, float dt, Triggers& triggersOut) override;
    virtual const AnimPoseVec& overlay(const AnimVariantMap& animVars, float dt, Triggers& triggersOut, const AnimPoseVec& underPoses) override;

    void setAlphaVar(const std::string& alphaVar) { _alphaVar = alphaVar; }

    virtual void setSkeletonInternal(AnimSkeleton::ConstPointer skeleton) override;

    struct JointVar {
        JointVar(const std::string& varIn, const std::string& jointNameIn) : var(varIn), jointName(jointNameIn), jointIndex(-1), hasPerformedJointLookup(false) {}
        std::string var = "";
        std::string jointName = "";
        int jointIndex = -1;
        bool hasPerformedJointLookup = false;
    };

    void addJointVar(const JointVar& jointVar);

protected:
    // for AnimDebugDraw rendering
    virtual const AnimPoseVec& getPosesInternal() const override;

    AnimPoseVec _poses;
    float _alpha;
    std::string _alphaVar;

    std::vector<JointVar> _jointVars;

    // no copies
    AnimController(const AnimController&) = delete;
    AnimController& operator=(const AnimController&) = delete;

};

#endif // hifi_AnimController_h
