//
//  AnimDefaultPose.h
//
//  Created by Anthony J. Thibault on 6/26/17.
//  Copyright (c) 2017 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimDefaultPose_h
#define hifi_AnimDefaultPose_h

#include <string>
#include "AnimNode.h"

// Always returns the default pose of the current skeleton.

class AnimDefaultPose : public AnimNode {
public:
    AnimDefaultPose(const QString& id);
    virtual ~AnimDefaultPose() override;

    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, Triggers& triggersOut) override;
protected:
    // for AnimDebugDraw rendering
    virtual const AnimPoseVec& getPosesInternal() const override;

    AnimPoseVec _poses;

    // no copies
    AnimDefaultPose(const AnimDefaultPose&) = delete;
    AnimDefaultPose& operator=(const AnimDefaultPose&) = delete;
};

#endif // hifi_AnimDefaultPose_h
