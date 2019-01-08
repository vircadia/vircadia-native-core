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
#include "AnimChain.h"

// Spline IK for the spine
class AnimSplineIK : public AnimNode {
public:
    AnimSplineIK(const QString& id, float alpha, bool enabled, float interpDuration,
        const QString& baseJointName, const QString& tipJointName,
        const QString& alphaVar, const QString& enabledVar,
        const QString& endEffectorRotationVarVar, const QString& endEffectorPositionVarVar);

	virtual ~AnimSplineIK() override;
    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, AnimVariantMap& triggersOut) override;

protected:

    enum class InterpType {
        None = 0,
        SnapshotToUnderPoses,
        SnapshotToSolve,
        NumTypes
    };

    // for AnimDebugDraw rendering
    virtual const AnimPoseVec& getPosesInternal() const override;
    virtual void setSkeletonInternal(AnimSkeleton::ConstPointer skeleton) override;

    void lookUpIndices();

    AnimPoseVec _poses;

    float _alpha;
    bool _enabled;
    float _interpDuration;
    QString _baseJointName;
    QString _tipJointName;

    int _baseParentJointIndex{ -1 };
    int _baseJointIndex{ -1 };
    int _tipJointIndex{ -1 };

    QString _alphaVar;  // float - (0, 1) 0 means underPoses only, 1 means IK only.
    QString _enabledVar;  // bool
    QString _endEffectorRotationVarVar; // string
    QString _endEffectorPositionVarVar; // string

    QString _prevEndEffectorRotationVar;
    QString _prevEndEffectorPositionVar;

    InterpType _interpType{ InterpType::None };
    float _interpAlphaVel{ 0.0f };
    float _interpAlpha{ 0.0f };

    AnimChain _snapshotChain;

    bool _lastEnableDebugDrawIKTargets{ false };

    // no copies
    AnimSplineIK(const AnimSplineIK&) = delete;
    AnimSplineIK&  operator=(const AnimSplineIK&) = delete;

};
#endif // hifi_AnimSplineIK_h
