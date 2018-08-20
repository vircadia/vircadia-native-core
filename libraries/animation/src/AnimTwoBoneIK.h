//
//  AnimTwoBoneIK.h
//
//  Created by Anthony J. Thibault on 5/12/18.
//  Copyright (c) 2018 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimTwoBoneIK_h
#define hifi_AnimTwoBoneIK_h

#include "AnimNode.h"
#include "AnimChain.h"

// Simple two bone IK chain
class AnimTwoBoneIK : public AnimNode {
public:
    friend class AnimTests;

    AnimTwoBoneIK(const QString& id, float alpha, bool enabled, float interpDuration,
                  const QString& baseJointName, const QString& midJointName,
                  const QString& tipJointName, const glm::vec3& midHingeAxis,
                  const QString& alphaVar, const QString& enabledVar,
                  const QString& endEffectorRotationVarVar, const QString& endEffectorPositionVarVar);
    virtual ~AnimTwoBoneIK() override;

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
    void beginInterp(InterpType interpType, const AnimChain& chain);

    AnimPoseVec _poses;

    float _alpha;
    bool _enabled;
    float _interpDuration;  // in frames (1/30 sec)
    QString _baseJointName;
    QString _midJointName;
    QString _tipJointName;
    glm::vec3 _midHingeAxis;  // in baseJoint relative frame, should be normalized

    int _baseParentJointIndex { -1 };
    int _baseJointIndex { -1 };
    int _midJointIndex { -1 };
    int _tipJointIndex { -1 };

    QString _alphaVar;  // float - (0, 1) 0 means underPoses only, 1 means IK only.
    QString _enabledVar;  // bool
    QString _endEffectorRotationVarVar; // string
    QString _endEffectorPositionVarVar; // string

    QString _prevEndEffectorRotationVar;
    QString _prevEndEffectorPositionVar;

    InterpType _interpType { InterpType::None };
    float _interpAlphaVel { 0.0f };
    float _interpAlpha { 0.0f };

    AnimChain _snapshotChain;

    bool _lastEnableDebugDrawIKTargets { false };

    // no copies
    AnimTwoBoneIK(const AnimTwoBoneIK&) = delete;
    AnimTwoBoneIK& operator=(const AnimTwoBoneIK&) = delete;
};

#endif // hifi_AnimTwoBoneIK_h
