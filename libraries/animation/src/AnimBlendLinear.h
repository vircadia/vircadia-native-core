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

class AnimBlendLinear : public AnimNode {
public:

    AnimBlendLinear(const std::string& id, float alpha);
    virtual ~AnimBlendLinear() override;

    virtual const std::vector<AnimPose>& evaluate(float dt) override;

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
