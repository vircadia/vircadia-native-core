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

    void setTargetVars(const QString& jointName, const QString& positionVar, const QString& rotationVar,
                       const QString& typeVar, const QString& weightVar, float weight, const std::vector<float>& flexCoefficients);

    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, AnimNode::Triggers& triggersOut) override;
    virtual const AnimPoseVec& overlay(const AnimVariantMap& animVars, const AnimContext& context, float dt, Triggers& triggersOut, const AnimPoseVec& underPoses) override;

    void clearIKJointLimitHistory();

    void setMaxHipsOffsetLength(float maxLength);

    float getMaxErrorOnLastSolve() { return _maxErrorOnLastSolve; }

    enum class SolutionSource {
        RelaxToUnderPoses = 0,
        RelaxToLimitCenterPoses,
        PreviousSolution,
        UnderPoses,
        LimitCenterPoses,
        NumSolutionSources,
    };

    void setSolutionSource(SolutionSource solutionSource) { _solutionSource = solutionSource; }
    void setSolutionSourceVar(const QString& solutionSourceVar) { _solutionSourceVar = solutionSourceVar; }

protected:
    void computeTargets(const AnimVariantMap& animVars, std::vector<IKTarget>& targets, const AnimPoseVec& underPoses);
    void solveWithCyclicCoordinateDescent(const AnimContext& context, const std::vector<IKTarget>& targets);
    void solveTargetWithCCD(const AnimContext& context, const IKTarget& target, const AnimPoseVec& absolutePoses, bool debug);
    virtual void setSkeletonInternal(AnimSkeleton::ConstPointer skeleton) override;
    struct DebugJoint {
        DebugJoint() : relRot(), constrained(false) {}
        DebugJoint(const glm::quat& relRotIn, bool constrainedIn) : relRot(relRotIn), constrained(constrainedIn) {}
        glm::quat relRot;
        bool constrained;
    };
    void debugDrawIKChain(std::map<int, DebugJoint>& debugJointMap, const AnimContext& context) const;
    void debugDrawRelativePoses(const AnimContext& context) const;
    void debugDrawConstraints(const AnimContext& context) const;
    void initRelativePosesFromSolutionSource(SolutionSource solutionSource, const AnimPoseVec& underPose);
    void blendToPoses(const AnimPoseVec& targetPoses, const AnimPoseVec& underPose, float blendFactor);


    // for AnimDebugDraw rendering
    virtual const AnimPoseVec& getPosesInternal() const override { return _relativePoses; }

    RotationConstraint* getConstraint(int index) const;
    void clearConstraints();
    void initConstraints();
    void initLimitCenterPoses();
    void computeHipsOffset(const std::vector<IKTarget>& targets, const AnimPoseVec& underPoses, float dt);

    // no copies
    AnimInverseKinematics(const AnimInverseKinematics&) = delete;
    AnimInverseKinematics& operator=(const AnimInverseKinematics&) = delete;

    enum FlexCoefficients { MAX_FLEX_COEFFICIENTS = 10 };
    struct IKTargetVar {
        IKTargetVar(const QString& jointNameIn, const QString& positionVarIn, const QString& rotationVarIn,
                    const QString& typeVarIn, const QString& weightVarIn, float weightIn, const std::vector<float>& flexCoefficientsIn);
        IKTargetVar(const IKTargetVar& orig);

        QString jointName;
        QString positionVar;
        QString rotationVar;
        QString typeVar;
        QString weightVar;
        float weight;
        float flexCoefficients[MAX_FLEX_COEFFICIENTS];
        size_t numFlexCoefficients;
        int jointIndex; // cached joint index
    };

    std::map<int, RotationConstraint*> _constraints;
    std::vector<RotationAccumulator> _accumulators;
    std::vector<IKTargetVar> _targetVarVec;
    AnimPoseVec _defaultRelativePoses; // poses of the relaxed state
    AnimPoseVec _relativePoses; // current relative poses
    AnimPoseVec _limitCenterPoses;  // relative

    // experimental data for moving hips during IK
    glm::vec3 _hipsOffset { Vectors::ZERO };
    float _maxHipsOffsetLength{ FLT_MAX };
    int _headIndex { -1 };
    int _hipsIndex { -1 };
    int _hipsParentIndex { -1 };
    int _hipsTargetIndex { -1 };

    // _maxTargetIndex is tracked to help optimize the recalculation of absolute poses
    // during the the cyclic coordinate descent algorithm
    int _maxTargetIndex { 0 };

    float _maxErrorOnLastSolve { FLT_MAX };
    bool _previousEnableDebugIKTargets { false };
    SolutionSource _solutionSource { SolutionSource::RelaxToUnderPoses };
    QString _solutionSourceVar;
};

#endif // hifi_AnimInverseKinematics_h
