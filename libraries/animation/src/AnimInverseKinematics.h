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

    /// \param index index of end effector
    /// \param position target position in the frame of the end-effector's root (not the parent!)
    /// \param rotation target rotation in the frame of the end-effector's root (not the parent!)
    void updateTarget(int index, const glm::vec3& position, const glm::quat& rotation);

    /// \param name name of end effector
    /// \param position target position in the frame of the end-effector's root (not the parent!)
    /// \param rotation target rotation in the frame of the end-effector's root (not the parent!)
    void updateTarget(const QString& name, const glm::vec3& position, const glm::quat& rotation);

    void clearTarget(int index);
    void clearAllTargets();

    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, float dt, AnimNode::Triggers& triggersOut) override;

protected:
    virtual void setSkeletonInternal(AnimSkeleton::ConstPointer skeleton);

    // for AnimDebugDraw rendering
    virtual const AnimPoseVec& getPosesInternal() const override { return _relativePoses; }

    void relaxTowardDefaults(float dt);

    RotationConstraint* getConstraint(int index);
    void clearConstraints();
    void initConstraints();

    struct IKTarget {
        AnimPose pose; // in root-frame
        int rootIndex; // cached root index
    };

    std::map<int, RotationConstraint*> _constraints;
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
