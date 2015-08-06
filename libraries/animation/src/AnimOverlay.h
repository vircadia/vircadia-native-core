//
//  AnimOverlay.h
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimNode.h"

#ifndef hifi_AnimOverlay_h
#define hifi_AnimOverlay_h

#include "AnimNode.h"

// Overlay the AnimPoses from one AnimNode on top of another AnimNode.
// child[0] is overlayed on top of child[1].  The boneset is used
// to control blending on a per-bone bases.

class AnimOverlay : public AnimNode {
public:

    enum BoneSet {
        FullBody = 0,
        UpperBodyBoneSet,
        LowerBodyBoneSet,
        RightArmBoneSet,
        LeftArmBoneSet,
        AboveTheHeadBoneSet,
        BelowTheHeadBoneSet,
        HeadOnlyBoneSet,
        SpineOnlyBoneSet,
        EmptyBoneSet,
        NumBoneSets,
    };

    AnimOverlay(const std::string& id, BoneSet boneSet);
    virtual ~AnimOverlay() override;

    void setBoneSet(BoneSet boneSet) { _boneSet = boneSet; }
    BoneSet getBoneSet() const { return _boneSet; }

    virtual const std::vector<AnimPose>& evaluate(float dt) override;

protected:
    // for AnimDebugDraw rendering
    virtual const std::vector<AnimPose>& getPosesInternal() const override;

    std::vector<AnimPose> _poses;
    BoneSet _boneSet;

    // no copies
    AnimOverlay(const AnimOverlay&) = delete;
    AnimOverlay& operator=(const AnimOverlay&) = delete;
};

#endif // hifi_AnimOverlay_h
