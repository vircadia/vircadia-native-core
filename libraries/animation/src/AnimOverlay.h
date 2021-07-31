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

    /*@jsdoc
     * <p>Specifies sets of joints.</p>
     * <table>
     *   <thead>
     *     <tr><th>Value</th><th>Name</th><th>Description</th>
     *   </thead>
     *   <tbody>
     *     <tr><td><code>0</code></td><td>FullBodyBoneSet</td><td>All joints.</td></tr>
     *     <tr><td><code>1</code></td><td>UpperBodyBoneSet</td><td>Only the "Spine" joint and its children.</td></tr>
     *     <tr><td><code>2</code></td><td>LowerBodyBoneSet</td><td>Only the leg joints and their children.</td></tr>
     *     <tr><td><code>3</code></td><td>LeftArmBoneSet</td><td>Joints that are the children of the "LeftShoulder" 
     *       joint.</td></tr>
     *     <tr><td><code>4</code></td><td>RightArmBoneSet</td><td>Joints that are the children of the "RightShoulder" 
     *       joint.</td></tr>
     *     <tr><td><code>5</code></td><td>AboveTheHeadBoneSet</td><td>Joints that are the children of the "Head" 
     *       joint.</td></tr>
     *     <tr><td><code>6</code></td><td>BelowTheHeadBoneSet</td><td>Joints that are NOT the children of the "head" 
     *       joint.</td></tr>
     *     <tr><td><code>7</code></td><td>HeadOnlyBoneSet</td><td>The "Head" joint.</td></tr>
     *     <tr><td><code>8</code></td><td>SpineOnlyBoneSet</td><td>The "Spine" joint.</td></tr>
     *     <tr><td><code>9</code></td><td>EmptyBoneSet</td><td>No joints.</td></tr>
     *     <tr><td><code>10</code></td><td>LeftHandBoneSet</td><td>joints that are the children of the "LeftHand" 
     *       joint.</td></tr>
     *     <tr><td><code>11</code></td><td>RightHandBoneSet</td><td>Joints that are the children of the "RightHand" 
     *       joint.</td></tr>
     *     <tr><td><code>12</code></td><td>HipsOnlyBoneSet</td><td>The "Hips" joint.</td></tr>
     *     <tr><td><code>13</code></td><td>BothFeetBoneSet</td><td>The "LeftFoot" and "RightFoot" joints.</td></tr>
     *   </tbody>
     * </table>
    * @typedef {number} MyAvatar.AnimOverlayBoneSet
     */
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
        HipsOnlyBoneSet,
        BothFeetBoneSet,
        NumBoneSets
    };

    AnimOverlay(const QString& id, BoneSet boneSet, float alpha);
    virtual ~AnimOverlay() override;

    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, AnimVariantMap& triggersOut) override;

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
    void buildHipsOnlyBoneSet();
    void buildBothFeetBoneSet();

    // no copies
    AnimOverlay(const AnimOverlay&) = delete;
    AnimOverlay& operator=(const AnimOverlay&) = delete;
};

#endif // hifi_AnimOverlay_h
