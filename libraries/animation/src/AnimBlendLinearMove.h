//
//  AnimBlendLinearMove.h
//
//  Created by Anthony J. Thibault on 10/22/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimBlendLinearMove_h
#define hifi_AnimBlendLinearMove_h

#include "AnimNode.h"

// Synced linear blend between two AnimNodes, where the playback speed of
// the animation is timeScaled to match movement speed.
//
// Each child animation is associated with a chracteristic speed.
// This defines the speed of that animation when played at the normal playback rate, 30 frames per second.
//
// The user also specifies a desired speed.  This desired speed is used to timescale
// the animation to achive the desired movement velocity.
//
// Blending is determined by the alpha parameter.
// If the number of children is 2, then the alpha parameters should be between
// 0 and 1.  The first animation will have a (1 - alpha) factor, and the second
// will have factor of alpha.
//
// This node supports more then 2 children.  In this case the alpha should be
// between 0 and n - 1.  This alpha can be used to linearly interpolate between
// the closest two children poses.  This can be used to sweep through a series
// of animation poses.

class AnimBlendLinearMove : public AnimNode {
public:
    friend class AnimTests;

    AnimBlendLinearMove(const QString& id, float alpha, float desiredSpeed, const std::vector<float>& characteristicSpeeds);
    virtual ~AnimBlendLinearMove() override;

    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, float dt, Triggers& triggersOut) override;

    void setAlphaVar(const QString& alphaVar) { _alphaVar = alphaVar; }
    void setDesiredSpeedVar(const QString& desiredSpeedVar) { _desiredSpeedVar = desiredSpeedVar; }

protected:
    // for AnimDebugDraw rendering
    virtual const AnimPoseVec& getPosesInternal() const override;

    void evaluateAndBlendChildren(const AnimVariantMap& animVars, Triggers& triggersOut, float alpha,
                                  size_t prevPoseIndex, size_t nextPoseIndex,
                                  float prevDeltaTime, float nextDeltaTime);

    void setFrameAndPhase(float dt, float alpha, int prevPoseIndex, int nextPoseIndex,
                          float* prevDeltaTimeOut, float* nextDeltaTimeOut, Triggers& triggersOut);

    virtual void setCurrentFrameInternal(float frame) override;

    AnimPoseVec _poses;

    float _alpha;
    float _desiredSpeed;

    float _phase = 0.0f;

    QString _alphaVar;
    QString _desiredSpeedVar;

    std::vector<float> _characteristicSpeeds;

    // no copies
    AnimBlendLinearMove(const AnimBlendLinearMove&) = delete;
    AnimBlendLinearMove& operator=(const AnimBlendLinearMove&) = delete;
};

#endif // hifi_AnimBlendLinearMove_h
