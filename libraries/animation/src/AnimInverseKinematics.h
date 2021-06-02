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

    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, AnimVariantMap& triggersOut) override;
    virtual const AnimPoseVec& overlay(const AnimVariantMap& animVars, const AnimContext& context, float dt, AnimVariantMap& triggersOut, const AnimPoseVec& underPoses) override;

    void clearIKJointLimitHistory();

    float getMaxErrorOnLastSolve() { return _maxErrorOnLastSolve; }

    /*@jsdoc
     * <p>Specifies the initial conditions of the IK solver.</p>
     * <table>
     *   <thead>
     *     <tr><th>Value</th><th>Name</th><th>Description</th>
     *   </thead>
     *   <tbody>
     *     <tr><td><code>0</code></td><td>RelaxToUnderPoses</td><td>This is a blend: it is 15/16 <code>PreviousSolution</code> 
     *       and 1/16 <code>UnderPoses</code>. This provides some of the benefits of using <code>UnderPoses</code> so that the 
     *       underlying animation is still visible, while at the same time converging faster then using the 
     *       <code>UnderPoses</code> as the only initial solution.</td></tr>
     *     <tr><td><code>1</code></td><td>RelaxToLimitCenterPoses</td><td>This is a blend: it is 15/16 
     *       <code>PreviousSolution</code> and 1/16 <code>LimitCenterPoses</code>. This should converge quickly because it is 
     *       close to the previous solution, but still provides the benefits of avoiding limb locking.</td></tr>
     *     <tr><td><code>2</code></td><td>PreviousSolution</td><td>
     *       <p>The IK system will begin to solve from the same position and orientations for each joint that was the result 
     *       from the previous frame.</p>
     *       <p>Pros: As the end effectors typically do not move much from frame to frame, this is likely to converge quickly 
     *       to a valid solution.</p>
     *       <p>Cons: If the previous solution resulted in an awkward or uncomfortable posture, the next frame will also be 
     *       awkward and uncomfortable. It can also result in locked elbows and knees.</p>
     *       </td></tr>
     *     <tr><td><code>3</code></td><td>UnderPoses</td><td>The IK occurs at one of the top-most layers. It has access to the 
     *       full posture that was computed via canned animations and blends. We call this animated set of poses the "under 
     *       pose". The under poses are what would be visible if IK was completely disabled. Using the under poses as the 
     *       initial conditions of the CCD solve will cause some of the animated motion to be blended into the result of the 
     *       IK. This can result in very natural results, especially if there are only a few IK targets enabled. On the other 
     *       hand, because the under poses might be quite far from the desired end effector, it can converge slowly in some 
     *       cases, causing it to never reach the IK target in the allotted number of iterations. Also, in situations where all 
     *       of the IK targets are being controlled by external sensors, sometimes starting from the under poses can cause 
     *       awkward motions from the underlying animations to leak into the IK result.</td></tr>
     *     <tr><td><code>4</code></td><td>LimitCenterPoses</td><td>This pose is taken to be the center of all the joint 
     *       constraints. This can prevent the IK solution from getting locked or stuck at a particular constraint. For 
     *       example, if the arm is pointing straight outward from the body, as the end effector moves towards the body, at 
     *       some point the elbow should bend to accommodate. However, because the CCD solver is stuck at a local maximum, it 
     *       will not rotate the elbow, unless the initial conditions already have the elbow bent, which is the case for 
     *       <code>LimitCenterPoses</code>. When all the IK targets are enabled, this result will provide a consistent starting 
     *       point for each IK solve, hopefully resulting in a consistent, natural result.</td></tr>
     *   </tbody>
     * </table>
     * @typedef {number} MyAvatar.AnimIKSolutionSource
     */
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
    float getInterpolationAlpha(float timer);

    // no copies
    AnimInverseKinematics(const AnimInverseKinematics&) = delete;
    AnimInverseKinematics& operator=(const AnimInverseKinematics&) = delete;

    enum FlexCoefficients { MAX_FLEX_COEFFICIENTS = 10 };
    struct IKTargetVar {
        IKTargetVar(const QString& jointNameIn, const QString& positionVarIn, const QString& rotationVarIn,
                    const QString& typeVarIn, const QString& weightVarIn, float weightIn, const std::vector<float>& flexCoefficientsIn,
                    const QString& poleVectorEnabledVar, const QString& poleReferenceVectorVar, const QString& poleVectorVar);
        IKTargetVar(const IKTargetVar& orig);
        AnimInverseKinematics::IKTargetVar& operator=(const AnimInverseKinematics::IKTargetVar&) = default;

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
    std::map<int, glm::quat> _rotationOnlyIKRotations;

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
