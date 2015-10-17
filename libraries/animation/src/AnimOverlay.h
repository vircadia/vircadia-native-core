//
//  AnimOverlay.h
//
//  Created by Anthony J. Thibault on 9/2/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimOverlay_h
#define hifi_AnimOverlay_h

#include "AnimNode.h"

// Overlay the AnimPoses from one AnimNode on top of another AnimNode.
// child[0] is overlayed on top of child[1].  The boneset is used
// to control blending on a per-bone bases.
// alpha gives the ability to fade in and fade out overlays.
// alpha of 0, will have no overlay, final pose will be 100% from child[1].
// alpha of 1, will be a full overlay.

class AnimOverlay : public AnimNode {
public:
    friend class AnimTests;

    enum BoneSet {
        FullBodyBoneSet = 0,
        UpperBodyBoneSet,
        LowerBodyBoneSet,
        LeftArmBoneSet,
        RightArmBoneSet,
        AboveTheHeadBoneSet,
        BelowTheHeadBoneSet,
        HeadOnlyBoneSet,
        SpineOnlyBoneSet,
        EmptyBoneSet,
        LeftHandBoneSet,
        RightHandBoneSet,
        NumBoneSets
    };

    AnimOverlay(const QString& id, BoneSet boneSet, float alpha);
    virtual ~AnimOverlay() override;

    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, float dt, Triggers& triggersOut) override;

    void setBoneSetVar(const QString& boneSetVar) { _boneSetVar = boneSetVar; }
    void setAlphaVar(const QString& alphaVar) { _alphaVar = alphaVar; }

 protected:
    void buildBoneSet(BoneSet boneSet);

    // for AnimDebugDraw rendering
    virtual const AnimPoseVec& getPosesInternal() const override;
    virtual void setSkeletonInternal(AnimSkeleton::ConstPointer skeleton) override;

    AnimPoseVec _poses;
    BoneSet _boneSet;
    float _alpha;
    std::vector<float> _boneSetVec;

    QString _boneSetVar;
    QString _alphaVar;

    void buildFullBodyBoneSet();
    void buildUpperBodyBoneSet();
    void buildLowerBodyBoneSet();
    void buildLeftArmBoneSet();
    void buildRightArmBoneSet();
    void buildAboveTheHeadBoneSet();
    void buildBelowTheHeadBoneSet();
    void buildHeadOnlyBoneSet();
    void buildSpineOnlyBoneSet();
    void buildEmptyBoneSet();
    void buildLeftHandBoneSet();
    void buildRightHandBoneSet();

    // no copies
    AnimOverlay(const AnimOverlay&) = delete;
    AnimOverlay& operator=(const AnimOverlay&) = delete;
};

#endif // hifi_AnimOverlay_h
