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

#include "AnimNode.h"

class RotationConstraint;

class AnimInverseKinematics : public AnimNode {
public:

    AnimInverseKinematics(const std::string& id);
    virtual ~AnimInverseKinematics() override;

    void loadDefaultPoses(const AnimPoseVec& poses);
    void loadPoses(const AnimPoseVec& poses);
    const AnimPoseVec& getRelativePoses() const { return _relativePoses; }
    void computeAbsolutePoses(AnimPoseVec& absolutePoses) const;

    void setTargetVars(const QString& jointName, const QString& positionVar, const QString& rotationVar);

    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, float dt, AnimNode::Triggers& triggersOut) override;
    virtual const AnimPoseVec& overlay(const AnimVariantMap& animVars, float dt, Triggers& triggersOut, const AnimPoseVec& underPoses) override;

protected:
    virtual void setSkeletonInternal(AnimSkeleton::ConstPointer skeleton);

    // for AnimDebugDraw rendering
    virtual const AnimPoseVec& getPosesInternal() const override { return _relativePoses; }

    void relaxTowardDefaults(float dt);

    RotationConstraint* getConstraint(int index);
    void clearConstraints();
    void initConstraints();

    struct IKTargetVar {
        IKTargetVar(const QString& jointNameIn, const std::string& positionVarIn, const std::string& rotationVarIn) :
            positionVar(positionVarIn),
            rotationVar(rotationVarIn),
            jointName(jointNameIn),
            jointIndex(-1),
            hasPerformedJointLookup(false) {}

        std::string positionVar;
        std::string rotationVar;
        QString jointName;
        int jointIndex; // cached joint index
        bool hasPerformedJointLookup = false;
    };

    struct IKTarget {
        AnimPose pose;
        int rootIndex;
    };

    std::map<int, RotationConstraint*> _constraints;
    std::vector<IKTargetVar> _targetVarVec;
    std::map<int, IKTarget> _absoluteTargets; // IK targets of end-points
    AnimPoseVec _defaultRelativePoses; // poses of the relaxed state
    AnimPoseVec _relativePoses; // current relative poses

    // no copies
    AnimInverseKinematics(const AnimInverseKinematics&) = delete;
    AnimInverseKinematics& operator=(const AnimInverseKinematics&) = delete;

    // _maxTargetIndex is tracked to help optimize the recalculation of absolute poses
    // during the the cyclic coordinate descent algorithm
    int _maxTargetIndex = 0;
};

#endif // hifi_AnimInverseKinematics_h
