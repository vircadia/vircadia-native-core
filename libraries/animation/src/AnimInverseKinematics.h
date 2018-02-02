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
#include "TranslationAccumulator.h"

class RotationConstraint;

class AnimInverseKinematics : public AnimNode {
public:

    struct JointInfo {
        glm::quat rot;
        glm::vec3 trans;
        int jointIndex;
        bool constrained;
    };

    struct JointChainInfo {
        std::vector<JointInfo> jointInfoVec;
        IKTarget target;
        float timer { 0.0f };
    };

    using JointChainInfoVec = std::vector<JointChainInfo>;

    explicit AnimInverseKinematics(const QString& id);
    virtual ~AnimInverseKinematics() override;

    void loadDefaultPoses(const AnimPoseVec& poses);
    void loadPoses(const AnimPoseVec& poses);
    void computeAbsolutePoses(AnimPoseVec& absolutePoses) const;

    void setTargetVars(const QString& jointName, const QString& positionVar, const QString& rotationVar,
                       const QString& typeVar, const QString& weightVar, float weight, const std::vector<float>& flexCoefficients,
                       const QString& poleVectorEnabledVar, const QString& poleReferenceVectorVar, const QString& poleVectorVar);

    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, AnimNode::Triggers& triggersOut) override;
    virtual const AnimPoseVec& overlay(const AnimVariantMap& animVars, const AnimContext& context, float dt, Triggers& triggersOut, const AnimPoseVec& underPoses) override;

    void clearIKJointLimitHistory();

    float getMaxErrorOnLastSolve() { return _maxErrorOnLastSolve; }

    enum class SolutionSource {
        RelaxToUnderPoses = 0,
        RelaxToLimitCenterPoses,
        PreviousSolution,
        UnderPoses,
        LimitCenterPoses,
        NumSolutionSources,
    };

    void setSecondaryTargetInRigFrame(int jointIndex, const AnimPose& pose);
    void clearSecondaryTarget(int jointIndex);

    void setSolutionSource(SolutionSource solutionSource) { _solutionSource = solutionSource; }
    void setSolutionSourceVar(const QString& solutionSourceVar) { _solutionSourceVar = solutionSourceVar; }

protected:
    void computeTargets(const AnimVariantMap& animVars, std::vector<IKTarget>& targets, const AnimPoseVec& underPoses);
    void solve(const AnimContext& context, const std::vector<IKTarget>& targets, float dt, JointChainInfoVec& jointChainInfoVec);
    void solveTargetWithCCD(const AnimContext& context, const IKTarget& target, const AnimPoseVec& absolutePoses,
                            bool debug, JointChainInfo& jointChainInfoOut) const;
    void solveTargetWithSpline(const AnimContext& context, const IKTarget& target, const AnimPoseVec& absolutePoses,
                               bool debug, JointChainInfo& jointChainInfoOut) const;
    virtual void setSkeletonInternal(AnimSkeleton::ConstPointer skeleton) override;
    void debugDrawIKChain(const JointChainInfo& jointChainInfo, const AnimContext& context) const;
    void debugDrawRelativePoses(const AnimContext& context) const;
    void debugDrawConstraints(const AnimContext& context) const;
    void debugDrawSpineSplines(const AnimContext& context, const std::vector<IKTarget>& targets) const;
    void initRelativePosesFromSolutionSource(SolutionSource solutionSource, const AnimPoseVec& underPose);
    void blendToPoses(const AnimPoseVec& targetPoses, const AnimPoseVec& underPose, float blendFactor);
    void preconditionRelativePosesToAvoidLimbLock(const AnimContext& context, const std::vector<IKTarget>& targets);
    void setSecondaryTargets(const AnimContext& context);

    // used to pre-compute information about each joint influeced by a spline IK target.
    struct SplineJointInfo {
        int jointIndex;       // joint in the skeleton that this information pertains to.
        float ratio;          // percentage (0..1) along the spline for this joint.
        AnimPose offsetPose;  // local offset from the spline to the joint.
    };

    void computeAndCacheSplineJointInfosForIKTarget(const AnimContext& context, const IKTarget& target) const;
    const std::vector<SplineJointInfo>* findOrCreateSplineJointInfo(const AnimContext& context, const IKTarget& target) const;

    // for AnimDebugDraw rendering
    virtual const AnimPoseVec& getPosesInternal() const override { return _relativePoses; }

    RotationConstraint* getConstraint(int index) const;
    void clearConstraints();
    void initConstraints();
    void initLimitCenterPoses();

    // no copies
    AnimInverseKinematics(const AnimInverseKinematics&) = delete;
    AnimInverseKinematics& operator=(const AnimInverseKinematics&) = delete;

    enum FlexCoefficients { MAX_FLEX_COEFFICIENTS = 10 };
    struct IKTargetVar {
        IKTargetVar(const QString& jointNameIn, const QString& positionVarIn, const QString& rotationVarIn,
                    const QString& typeVarIn, const QString& weightVarIn, float weightIn, const std::vector<float>& flexCoefficientsIn,
                    const QString& poleVectorEnabledVar, const QString& poleReferenceVectorVar, const QString& poleVectorVar);
        IKTargetVar(const IKTargetVar& orig);

        QString jointName;
        QString positionVar;
        QString rotationVar;
        QString typeVar;
        QString weightVar;
        QString poleVectorEnabledVar;
        QString poleReferenceVectorVar;
        QString poleVectorVar;
        float weight;
        float flexCoefficients[MAX_FLEX_COEFFICIENTS];
        size_t numFlexCoefficients;
        int jointIndex; // cached joint index
    };

    std::map<int, RotationConstraint*> _constraints;
    std::vector<RotationAccumulator> _rotationAccumulators;
    std::vector<TranslationAccumulator> _translationAccumulators;
    std::vector<IKTargetVar> _targetVarVec;
    AnimPoseVec _defaultRelativePoses; // poses of the relaxed state
    AnimPoseVec _relativePoses; // current relative poses
    AnimPoseVec _limitCenterPoses;  // relative

    std::map<int, AnimPose> _secondaryTargetsInRigFrame;

    mutable std::map<int, std::vector<SplineJointInfo>> _splineJointInfoMap;

    int _headIndex { -1 };
    int _hipsIndex { -1 };
    int _hipsParentIndex { -1 };
    int _hipsTargetIndex { -1 };
    int _leftHandIndex { -1 };
    int _rightHandIndex { -1 };

    float _maxErrorOnLastSolve { FLT_MAX };
    bool _previousEnableDebugIKTargets { false };
    SolutionSource _solutionSource { SolutionSource::RelaxToUnderPoses };
    QString _solutionSourceVar;

    JointChainInfoVec _prevJointChainInfoVec;
};

#endif // hifi_AnimInverseKinematics_h
