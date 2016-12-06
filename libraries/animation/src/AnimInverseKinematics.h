//
//  AnimInverseKinematics.h
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimInverseKinematics_h
#define hifi_AnimInverseKinematics_h

#include <string>

#include <map>
#include <vector>

#include "AnimNode.h"
#include "IKTarget.h"

#include "RotationAccumulator.h"

class RotationConstraint;

class AnimInverseKinematics : public AnimNode {
public:

    explicit AnimInverseKinematics(const QString& id);
    virtual ~AnimInverseKinematics() override;

    void loadDefaultPoses(const AnimPoseVec& poses);
    void loadPoses(const AnimPoseVec& poses);
    void computeAbsolutePoses(AnimPoseVec& absolutePoses) const;

    void setTargetVars(const QString& jointName, const QString& positionVar, const QString& rotationVar, const QString& typeVar);

    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, float dt, AnimNode::Triggers& triggersOut) override;
    virtual const AnimPoseVec& overlay(const AnimVariantMap& animVars, float dt, Triggers& triggersOut, const AnimPoseVec& underPoses) override;

    void clearIKJointLimitHistory();

protected:
    void computeTargets(const AnimVariantMap& animVars, std::vector<IKTarget>& targets, const AnimPoseVec& underPoses);
    void solveWithCyclicCoordinateDescent(const std::vector<IKTarget>& targets);
    int solveTargetWithCCD(const IKTarget& target, AnimPoseVec& absolutePoses);
    virtual void setSkeletonInternal(AnimSkeleton::ConstPointer skeleton) override;

    // for AnimDebugDraw rendering
    virtual const AnimPoseVec& getPosesInternal() const override { return _relativePoses; }

    RotationConstraint* getConstraint(int index);
    void clearConstraints();
    void initConstraints();

    // no copies
    AnimInverseKinematics(const AnimInverseKinematics&) = delete;
    AnimInverseKinematics& operator=(const AnimInverseKinematics&) = delete;

    struct IKTargetVar {
        IKTargetVar(const QString& jointNameIn,
                const QString& positionVarIn,
                const QString& rotationVarIn,
                const QString& typeVarIn) :
            positionVar(positionVarIn),
            rotationVar(rotationVarIn),
            typeVar(typeVarIn),
            jointName(jointNameIn),
            jointIndex(-1)
        {}

        QString positionVar;
        QString rotationVar;
        QString typeVar;
        QString jointName;
        int jointIndex; // cached joint index
    };

    std::map<int, RotationConstraint*> _constraints;
    std::vector<RotationAccumulator> _accumulators;
    std::vector<IKTargetVar> _targetVarVec;
    AnimPoseVec _defaultRelativePoses; // poses of the relaxed state
    AnimPoseVec _relativePoses; // current relative poses

    // experimental data for moving hips during IK
    glm::vec3 _hipsOffset { Vectors::ZERO };
    int _headIndex { -1 };
    int _hipsIndex { -1 };
    int _hipsParentIndex { -1 };

    // _maxTargetIndex is tracked to help optimize the recalculation of absolute poses
    // during the the cyclic coordinate descent algorithm
    int _maxTargetIndex { 0 };
};

#endif // hifi_AnimInverseKinematics_h
