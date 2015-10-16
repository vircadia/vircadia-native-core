//
//  AnimBlendLinear.h
//
//  Created by Anthony J. Thibault on 9/2/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimBlendLinear_h
#define hifi_AnimBlendLinear_h

#include "AnimNode.h"

// Linear blend between two AnimNodes.
// the amount of blending is determined by the alpha parameter.
// If the number of children is 2, then the alpha parameters should be between
// 0 and 1.  The first animation will have a (1 - alpha) factor, and the second
// will have factor of alpha.
// This node supports more then 2 children.  In this case the alpha should be
// between 0 and n - 1.  This alpha can be used to linearly interpolate between
// the closest two children poses.  This can be used to sweep through a series
// of animation poses.
//
// The sync flag is used to synchronize between child animations of different lengths.
// Typically used to synchronize blending between walk and run cycles.

class AnimBlendLinear : public AnimNode {
public:
    friend class AnimTests;

    AnimBlendLinear(const QString& id, float alpha, bool sync);
    virtual ~AnimBlendLinear() override;

    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, float dt, Triggers& triggersOut) override;

    void setAlphaVar(const QString& alphaVar) { _alphaVar = alphaVar; }

protected:
    // for AnimDebugDraw rendering
    virtual const AnimPoseVec& getPosesInternal() const override;

    void evaluateAndBlendChildren(const AnimVariantMap& animVars, Triggers& triggersOut, float alpha,
                                  size_t prevPoseIndex, size_t nextPoseIndex,
                                  float prevPoseDeltaTime, float nextPoseDeltaTime);
    void setSyncFrameAndComputeDeltaTime(float dt, size_t prevPoseIndex, size_t nextPoseIndex,
                                         float* prevPoseDeltaTime, float* nextPoseDeltaTime,
                                         Triggers& triggersOut);

    AnimPoseVec _poses;

    float _alpha;
    bool _sync;
    float _syncFrame = 0.0f;
    float _timeScale = 1.0f;  // TODO: HOOK THIS UP TO AN ANIMVAR.

    QString _alphaVar;

    // no copies
    AnimBlendLinear(const AnimBlendLinear&) = delete;
    AnimBlendLinear& operator=(const AnimBlendLinear&) = delete;
};

#endif // hifi_AnimBlendLinear_h
