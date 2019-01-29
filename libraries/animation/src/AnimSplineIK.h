//
//  AnimSplineIK.h
//
//  Created by Angus Antley on 1/7/19.
//  Copyright (c) 2019 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimSplineIK_h
#define hifi_AnimSplineIK_h

#include "AnimNode.h"
#include "IKTarget.h"
#include "AnimChain.h"

// Spline IK for the spine
class AnimSplineIK : public AnimNode {
public:
    AnimSplineIK(const QString& id, float alpha, bool enabled, float interpDuration,
        const QString& baseJointName, const QString& midJointName, const QString& tipJointName,
        const QString& basePositionVar, const QString& baseRotationVar,
        const QString& midPositionVar, const QString& midRotationVar,
        const QString& tipPositionVar, const QString& tipRotationVar,
        const QString& alphaVar, const QString& enabledVar,
        const QString& tipTargetFlexCoefficients,
        const QString& midTargetFlexCoefficients);

	virtual ~AnimSplineIK() override;
    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, AnimVariantMap& triggersOut) override;

protected:

    enum class InterpType {
        None = 0,
        SnapshotToUnderPoses,
        SnapshotToSolve,
        NumTypes
    };

    void computeAbsolutePoses(AnimPoseVec& absolutePoses) const;
    void loadPoses(const AnimPoseVec& poses);

    // for AnimDebugDraw rendering
    virtual const AnimPoseVec& getPosesInternal() const override;
    virtual void setSkeletonInternal(AnimSkeleton::ConstPointer skeleton) override;

    void lookUpIndices();
    void beginInterp(InterpType interpType, const AnimChain& chain);

    AnimPoseVec _poses;

    float _alpha;
    bool _enabled;
    float _interpDuration;
    QString _baseJointName;
    QString _tipJointName;
    QString _midJointName;
    QString _basePositionVar;
    QString _baseRotationVar;
    QString _midPositionVar;
    QString _midRotationVar;
    QString _tipPositionVar;
    QString _tipRotationVar;

    static const int MAX_NUMBER_FLEX_VARIABLES = 10;
    float _tipTargetFlexCoefficients[MAX_NUMBER_FLEX_VARIABLES];
    float _midTargetFlexCoefficients[MAX_NUMBER_FLEX_VARIABLES];
    int _numTipTargetFlexCoefficients { 0 };
    int _numMidTargetFlexCoefficients { 0 };

    int _baseParentJointIndex { -1 };
    int _baseJointIndex { -1 };
    int _midJointIndex { -1 };
    int _tipJointIndex { -1 };

    QString _alphaVar;  // float - (0, 1) 0 means underPoses only, 1 means IK only.
    QString _enabledVar;  // bool

    bool _previousEnableDebugIKTargets { false };

    InterpType _interpType{ InterpType::None };
    float _interpAlphaVel{ 0.0f };
    float _interpAlpha{ 0.0f };

    AnimChain _snapshotChain;

    // used to pre-compute information about each joint influenced by a spline IK target.
    struct SplineJointInfo {
        int jointIndex;       // joint in the skeleton that this information pertains to.
        float ratio;          // percentage (0..1) along the spline for this joint.
        AnimPose offsetPose;  // local offset from the spline to the joint.
    };

    bool _lastEnableDebugDrawIKTargets { false };
    void AnimSplineIK::solveTargetWithSpline(const AnimContext& context, int base, const IKTarget& target, const AnimPoseVec& absolutePoses, bool debug, AnimChain& chainInfoOut) const;
    void computeAndCacheSplineJointInfosForIKTarget(const AnimContext& context, const IKTarget& target) const;
    const std::vector<SplineJointInfo>* findOrCreateSplineJointInfo(const AnimContext& context, const IKTarget& target) const;
    mutable std::map<int, std::vector<SplineJointInfo>> _splineJointInfoMap;

    // no copies
    AnimSplineIK(const AnimSplineIK&) = delete;
    AnimSplineIK&  operator=(const AnimSplineIK&) = delete;

};
#endif // hifi_AnimSplineIK_h
