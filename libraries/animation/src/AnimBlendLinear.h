//
//  AnimBlendLinear.h
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimNode.h"

#ifndef hifi_AnimBlendLinear_h
#define hifi_AnimBlendLinear_h

// Linear blend between two AnimNodes.
// the amount of blending is determined by the alpha parameter.
// If the number of children is 2, then the alpha parameters should be between
// 0 and 1.  The first animation will have a (1 - alpha) factor, and the second
// will have factor of alpha.
// This node supports more then 2 children.  In this case the alpha should be
// between 0 and n - 1.  This alpha can be used to linearly interpolate between
// the closest two children poses.  This can be used to sweep through a series
// of animation poses.

class AnimBlendLinear : public AnimNode {
public:

    AnimBlendLinear(const std::string& id, float alpha);
    virtual ~AnimBlendLinear() override;

    virtual const std::vector<AnimPose>& evaluate(const AnimVariantMap& animVars, float dt) override;

    void setAlpha(float alpha) { _alpha = alpha; }
    float getAlpha() const { return _alpha; }

protected:
    // for AnimDebugDraw rendering
    virtual const std::vector<AnimPose>& getPosesInternal() const override;

    std::vector<AnimPose> _poses;

    float _alpha;

    // no copies
    AnimBlendLinear(const AnimBlendLinear&) = delete;
    AnimBlendLinear& operator=(const AnimBlendLinear&) = delete;
};

#endif // hifi_AnimBlendLinear_h
