//
//  AnimInverseKinematics.cpp
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimInverseKinematics.h"

#include <GeometryUtil.h>
#include <GLMHelpers.h>
#include <NumericalConstants.h>
#include <SharedUtil.h>
#include <shared/NsightHelpers.h>
#include <DebugDraw.h>
#include "Rig.h"

#include "ElbowConstraint.h"
#include "SwingTwistConstraint.h"
#include "AnimationLogging.h"

bool HACKY_GLOBAL_ENABLE_DEBUG_DRAW_IK_TARGETS = false;
Rig* HACKY_GLOBAL_RIG_POINTER = nullptr;

AnimInverseKinematics::AnimInverseKinematics(const QString& id) : AnimNode(AnimNode::Type::InverseKinematics, id) {
}

AnimInverseKinematics::~AnimInverseKinematics() {
    clearConstraints();
    _accumulators.clear();
    _targetVarVec.clear();
}

void AnimInverseKinematics::loadDefaultPoses(const AnimPoseVec& poses) {
    _defaultRelativePoses = poses;
    assert(_skeleton && _skeleton->getNumJoints() == (int)poses.size());
}

void AnimInverseKinematics::loadPoses(const AnimPoseVec& poses) {
    assert(_skeleton && ((poses.size() == 0) || (_skeleton->getNumJoints() == (int)poses.size())));
    if (_skeleton->getNumJoints() == (int)poses.size()) {
        _relativePoses = poses;
        _accumulators.resize(_relativePoses.size());
    } else {
        _relativePoses.clear();
        _accumulators.clear();
    }
}

void AnimInverseKinematics::computeAbsolutePoses(AnimPoseVec& absolutePoses) const {
    int numJoints = (int)_relativePoses.size();
    assert(numJoints <= _skeleton->getNumJoints());
    assert(numJoints == (int)absolutePoses.size());
    for (int i = 0; i < numJoints; ++i) {
        int parentIndex = _skeleton->getParentIndex(i);
        if (parentIndex < 0) {
            absolutePoses[i] = _relativePoses[i];
        } else {
            absolutePoses[i] = absolutePoses[parentIndex] * _relativePoses[i];
        }
    }
}

void AnimInverseKinematics::setTargetVars(
        const QString& jointName,
        const QString& positionVar,
        const QString& rotationVar,
        const QString& typeVar) {
    // if there are dups, last one wins.
    bool found = false;
    for (auto& targetVar: _targetVarVec) {
        if (targetVar.jointName == jointName) {
            // update existing targetVar
            targetVar.positionVar = positionVar;
            targetVar.rotationVar = rotationVar;
            targetVar.typeVar = typeVar;
            found = true;
            break;
        }
    }
    if (!found) {
        // create a new entry
        _targetVarVec.push_back(IKTargetVar(jointName, positionVar, rotationVar, typeVar));
    }
}

void AnimInverseKinematics::computeTargets(const AnimVariantMap& animVars, std::vector<IKTarget>& targets, const AnimPoseVec& underPoses) {
    // build a list of valid targets from _targetVarVec and animVars
    _maxTargetIndex = -1;
    bool removeUnfoundJoints = false;
    for (auto& targetVar : _targetVarVec) {
        if (targetVar.jointIndex == -1) {
            // this targetVar hasn't been validated yet...
            int jointIndex = _skeleton->nameToJointIndex(targetVar.jointName);
            if (jointIndex >= 0) {
                // this targetVar has a valid joint --> cache the indices
                targetVar.jointIndex = jointIndex;
            } else {
                qCWarning(animation) << "AnimInverseKinematics could not find jointName" << targetVar.jointName << "in skeleton";
                removeUnfoundJoints = true;
            }
        } else {
            IKTarget target;
            target.setType(animVars.lookup(targetVar.typeVar, (int)IKTarget::Type::RotationAndPosition));
            if (target.getType() != IKTarget::Type::Unknown) {
                AnimPose defaultPose = _skeleton->getAbsolutePose(targetVar.jointIndex, underPoses);
                glm::quat rotation = animVars.lookupRigToGeometry(targetVar.rotationVar, defaultPose.rot);
                glm::vec3 translation = animVars.lookupRigToGeometry(targetVar.positionVar, defaultPose.trans);
                if (target.getType() == IKTarget::Type::HipsRelativeRotationAndPosition) {
                    translation += _hipsOffset;
                }
                target.setPose(rotation, translation);
                target.setIndex(targetVar.jointIndex);
                targets.push_back(target);
                if (targetVar.jointIndex > _maxTargetIndex) {
                    _maxTargetIndex = targetVar.jointIndex;
                }
            }
        }
    }

    if (removeUnfoundJoints) {
        int numVars = (int)_targetVarVec.size();
        int i = 0;
        while (i < numVars) {
            if (_targetVarVec[i].jointIndex == -1) {
                if (numVars > 1) {
                    // swap i for last element
                    _targetVarVec[i] = _targetVarVec[numVars - 1];
                }
                _targetVarVec.pop_back();
                --numVars;
            } else {
                ++i;
            }
        }
    }
}

void AnimInverseKinematics::solveWithCyclicCoordinateDescent(const std::vector<IKTarget>& targets) {
    // compute absolute poses that correspond to relative target poses
    AnimPoseVec absolutePoses;
    absolutePoses.resize(_relativePoses.size());
    computeAbsolutePoses(absolutePoses);

    // clear the accumulators before we start the IK solver
    for (auto& accumulator: _accumulators) {
        accumulator.clearAndClean();
    }

    float maxError = FLT_MAX;
    int numLoops = 0;
    const int MAX_IK_LOOPS = 16;
    const float MAX_ERROR_TOLERANCE = 0.1f; // cm
    while (maxError > MAX_ERROR_TOLERANCE && numLoops < MAX_IK_LOOPS) {
        ++numLoops;

        // solve all targets
        int lowestMovedIndex = (int)_relativePoses.size();
        for (auto& target: targets) {
            int lowIndex = solveTargetWithCCD(target, absolutePoses);
            if (lowIndex < lowestMovedIndex) {
                lowestMovedIndex = lowIndex;
            }
        }

        // harvest accumulated rotations and apply the average
        for (int i = lowestMovedIndex; i < _maxTargetIndex; ++i) {
            if (_accumulators[i].size() > 0) {
                _relativePoses[i].rot = _accumulators[i].getAverage();
                _accumulators[i].clear();
            }
        }

        // update the absolutePoses that need it (from lowestMovedIndex to _maxTargetIndex)
        for (auto i = lowestMovedIndex; i <= _maxTargetIndex; ++i) {
            auto parentIndex = _skeleton->getParentIndex((int)i);
            if (parentIndex != -1) {
                absolutePoses[i] = absolutePoses[parentIndex] * _relativePoses[i];
            }
        }

        // compute maxError
        maxError = 0.0f;
        for (size_t i = 0; i < targets.size(); i++) {
            if (targets[i].getType() == IKTarget::Type::RotationAndPosition || targets[i].getType() == IKTarget::Type::HmdHead ||
                targets[i].getType() == IKTarget::Type::HipsRelativeRotationAndPosition) {
                float error = glm::length(absolutePoses[targets[i].getIndex()].trans - targets[i].getTranslation());
                if (error > maxError) {
                    maxError = error;
                }
            }
        }
    }

    // finally set the relative rotation of each tip to agree with absolute target rotation
    for (auto& target: targets) {
        int tipIndex = target.getIndex();
        int parentIndex = _skeleton->getParentIndex(tipIndex);
        if (parentIndex != -1) {
            const glm::quat& targetRotation = target.getRotation();
            // compute tip's new parent-relative rotation
            // Q = Qp * q   -->   q' = Qp^ * Q
            glm::quat newRelativeRotation = glm::inverse(absolutePoses[parentIndex].rot) * targetRotation;
            RotationConstraint* constraint = getConstraint(tipIndex);
            if (constraint) {
                constraint->apply(newRelativeRotation);
                // TODO: ATM the final rotation target just fails but we need to provide
                // feedback to the IK system so that it can adjust the bones up the skeleton
                // to help this rotation target get met.
            }
            _relativePoses[tipIndex].rot = newRelativeRotation;
            absolutePoses[tipIndex].rot = targetRotation;
        }
    }
}

int AnimInverseKinematics::solveTargetWithCCD(const IKTarget& target, AnimPoseVec& absolutePoses) {
    int lowestMovedIndex = (int)_relativePoses.size();
    IKTarget::Type targetType = target.getType();
    if (targetType == IKTarget::Type::RotationOnly) {
        // the final rotation will be enforced after the iterations
        // TODO: solve this correctly
        return lowestMovedIndex;
    }

    int tipIndex = target.getIndex();
    int pivotIndex = _skeleton->getParentIndex(tipIndex);
    if (pivotIndex == -1 || pivotIndex == _hipsIndex) {
        return lowestMovedIndex;
    }
    int pivotsParentIndex = _skeleton->getParentIndex(pivotIndex);
    if (pivotsParentIndex == -1) {
        // TODO?: handle case where tip's parent is root?
        return lowestMovedIndex;
    }

    // cache tip's absolute orientation
    glm::quat tipOrientation = absolutePoses[tipIndex].rot;

    // also cache tip's parent's absolute orientation so we can recompute
    // the tip's parent-relative as we proceed up the chain
    glm::quat tipParentOrientation = absolutePoses[pivotIndex].rot;

    if (targetType == IKTarget::Type::HmdHead) {
        // rotate tip directly to target orientation
        tipOrientation = target.getRotation();
        glm::quat tipRelativeRotation = glm::normalize(tipOrientation * glm::inverse(tipParentOrientation));

        // enforce tip's constraint
        RotationConstraint* constraint = getConstraint(tipIndex);
        if (constraint) {
            bool constrained = constraint->apply(tipRelativeRotation);
            if (constrained) {
                tipOrientation = glm::normalize(tipRelativeRotation * tipParentOrientation);
                tipRelativeRotation = glm::normalize(tipOrientation * glm::inverse(tipParentOrientation));
            }
        }
        // store the relative rotation change in the accumulator
        _accumulators[tipIndex].add(tipRelativeRotation, target.getWeight());
    }

    // cache tip absolute position
    glm::vec3 tipPosition = absolutePoses[tipIndex].trans;

    // descend toward root, pivoting each joint to get tip closer to target position
    while (pivotIndex != _hipsIndex && pivotsParentIndex != -1) {
        // compute the two lines that should be aligned
        glm::vec3 jointPosition = absolutePoses[pivotIndex].trans;
        glm::vec3 leverArm = tipPosition - jointPosition;

        glm::quat deltaRotation;
        if (targetType == IKTarget::Type::RotationAndPosition ||
                targetType == IKTarget::Type::HipsRelativeRotationAndPosition) {
            // compute the swing that would get get tip closer
            glm::vec3 targetLine = target.getTranslation() - jointPosition;

            const float MIN_AXIS_LENGTH = 1.0e-4f;
            RotationConstraint* constraint = getConstraint(pivotIndex);
            if (constraint && constraint->isLowerSpine()) {
                // for these types of targets we only allow twist at the lower-spine
                // (this prevents the hand targets from bending the spine too much and thereby driving the hips too far)
                glm::vec3 twistAxis = absolutePoses[pivotIndex].trans - absolutePoses[pivotsParentIndex].trans;
                float twistAxisLength = glm::length(twistAxis);
                if (twistAxisLength > MIN_AXIS_LENGTH) {
                    // project leverArm and targetLine to the plane
                    twistAxis /= twistAxisLength;
                    leverArm -= glm::dot(leverArm, twistAxis) * twistAxis;
                    targetLine -= glm::dot(targetLine, twistAxis) * twistAxis;
                } else {
                    leverArm = Vectors::ZERO;
                    targetLine = Vectors::ZERO;
                }
            }

            glm::vec3 axis = glm::cross(leverArm, targetLine);
            float axisLength = glm::length(axis);
            if (axisLength > MIN_AXIS_LENGTH) {
                // compute angle of rotation that brings tip closer to target
                axis /= axisLength;
                float cosAngle = glm::clamp(glm::dot(leverArm, targetLine) / (glm::length(leverArm) * glm::length(targetLine)), -1.0f, 1.0f);
                float angle = acosf(cosAngle);
                const float MIN_ADJUSTMENT_ANGLE = 1.0e-4f;
                if (angle > MIN_ADJUSTMENT_ANGLE) {
                    // reduce angle by a fraction (for stability)
                    const float FRACTION = 0.5f;
                    angle *= FRACTION;
                    deltaRotation = glm::angleAxis(angle, axis);

                    // The swing will re-orient the tip but there will tend to be be a non-zero delta between the tip's
                    // new orientation and its target.  This is the final parent-relative orientation that the tip joint have
                    // make to achieve its target orientation.
                    glm::quat tipRelativeRotation = glm::inverse(deltaRotation * tipParentOrientation) * target.getRotation();

                    // enforce tip's constraint
                    RotationConstraint* constraint = getConstraint(tipIndex);
                    if (constraint) {
                        bool constrained = constraint->apply(tipRelativeRotation);
                        if (constrained) {
                            // The tip's final parent-relative rotation would violate its constraint
                            // so we try to pre-twist this pivot to compensate.
                            glm::quat constrainedTipRotation = deltaRotation * tipParentOrientation * tipRelativeRotation;
                            glm::quat missingRotation = target.getRotation() * glm::inverse(constrainedTipRotation);
                            glm::quat swingPart;
                            glm::quat twistPart;
                            glm::vec3 axis = glm::normalize(deltaRotation * leverArm);
                            swingTwistDecomposition(missingRotation, axis, swingPart, twistPart);
                            float dotSign = copysignf(1.0f, twistPart.w);
                            deltaRotation = glm::normalize(glm::lerp(glm::quat(), dotSign * twistPart, FRACTION)) * deltaRotation;
                        }
                    }
                }
            }
        } else if (targetType == IKTarget::Type::HmdHead) {
            // An HmdHead target slaves the orientation of the end-effector by distributing rotation
            // deltas up the hierarchy.  Its target position is enforced later (by shifting the hips).
            deltaRotation = target.getRotation() * glm::inverse(tipOrientation);
            float dotSign = copysignf(1.0f, deltaRotation.w);
            const float ANGLE_DISTRIBUTION_FACTOR = 0.45f;
            deltaRotation = glm::normalize(glm::lerp(glm::quat(), dotSign * deltaRotation, ANGLE_DISTRIBUTION_FACTOR));
        }

        // compute joint's new parent-relative rotation after swing
        // Q' = dQ * Q   and   Q = Qp * q   -->   q' = Qp^ * dQ * Q
        glm::quat newRot = glm::normalize(glm::inverse(
                absolutePoses[pivotsParentIndex].rot) *
                deltaRotation *
                absolutePoses[pivotIndex].rot);

        // enforce pivot's constraint
        RotationConstraint* constraint = getConstraint(pivotIndex);
        if (constraint) {
            bool constrained = constraint->apply(newRot);
            if (constrained) {
                // the constraint will modify the local rotation of the tip so we must
                // compute the corresponding model-frame deltaRotation
                // Q' = Qp^ * dQ * Q  -->  dQ =   Qp * Q' * Q^
                deltaRotation = absolutePoses[pivotsParentIndex].rot * newRot * glm::inverse(absolutePoses[pivotIndex].rot);
            }
        }

        // store the relative rotation change in the accumulator
        _accumulators[pivotIndex].add(newRot, target.getWeight());

        // this joint has been changed so we check to see if it has the lowest index
        if (pivotIndex < lowestMovedIndex) {
            lowestMovedIndex = pivotIndex;
        }

        // keep track of tip's new transform as we descend towards root
        tipPosition = jointPosition + deltaRotation * (tipPosition - jointPosition);
        tipOrientation = glm::normalize(deltaRotation * tipOrientation);
        tipParentOrientation = glm::normalize(deltaRotation * tipParentOrientation);

        pivotIndex = pivotsParentIndex;
        pivotsParentIndex = _skeleton->getParentIndex(pivotIndex);
    }
    return lowestMovedIndex;
}

//virtual
const AnimPoseVec& AnimInverseKinematics::evaluate(const AnimVariantMap& animVars, float dt, AnimNode::Triggers& triggersOut) {
    // don't call this function, call overlay() instead
    assert(false);
    return _relativePoses;
}

//virtual
const AnimPoseVec& AnimInverseKinematics::overlay(const AnimVariantMap& animVars, float dt, Triggers& triggersOut, const AnimPoseVec& underPoses) {

    const float MAX_OVERLAY_DT = 1.0f / 30.0f; // what to clamp delta-time to in AnimInverseKinematics::overlay
    if (dt > MAX_OVERLAY_DT) {
        dt = MAX_OVERLAY_DT;
    }

    if (_relativePoses.size() != underPoses.size()) {
        loadPoses(underPoses);
    } else {

        PROFILE_RANGE_EX("ik/relax", 0xffff00ff, 0);

        // relax toward underPoses
        // HACK: this relaxation needs to be constant per-frame rather than per-realtime
        // in order to prevent IK "flutter" for bad FPS.  The bad news is that the good parts
        // of this relaxation will be FPS dependent (low FPS will make the limbs align slower
        // in real-time), however most people will not notice this and this problem is less
        // annoying than the flutter.
        const float blend = (1.0f / 60.0f) / (0.25f); // effectively: dt / RELAXATION_TIMESCALE
        int numJoints = (int)_relativePoses.size();
        for (int i = 0; i < numJoints; ++i) {
            float dotSign = copysignf(1.0f, glm::dot(_relativePoses[i].rot, underPoses[i].rot));
            if (_accumulators[i].isDirty()) {
                // this joint is affected by IK --> blend toward underPose rotation
                _relativePoses[i].rot = glm::normalize(glm::lerp(_relativePoses[i].rot, dotSign * underPoses[i].rot, blend));
            } else {
                // this joint is NOT affected by IK --> slam to underPose rotation
                _relativePoses[i].rot = underPoses[i].rot;
            }
            _relativePoses[i].trans = underPoses[i].trans;
        }

        if (!_relativePoses.empty()) {
            // Sometimes the underpose itself can violate the constraints.  Rather than
            // clamp the animation we dynamically expand each constraint to accomodate it.
            std::map<int, RotationConstraint*>::iterator constraintItr = _constraints.begin();
            while (constraintItr != _constraints.end()) {
                int index = constraintItr->first;
                constraintItr->second->dynamicallyAdjustLimits(_relativePoses[index].rot);
                ++constraintItr;
            }
        }
    }

    if (!_relativePoses.empty()) {

        // build a list of targets from _targetVarVec
        std::vector<IKTarget> targets;
        {
            PROFILE_RANGE_EX("ik/computeTargets", 0xffff00ff, 0);
            computeTargets(animVars, targets, underPoses);
        }

        // AJT: HACK
        if (HACKY_GLOBAL_ENABLE_DEBUG_DRAW_IK_TARGETS && HACKY_GLOBAL_RIG_POINTER) {
            const vec4 WHITE(1.0f);
            glm::mat4 geomToRigMat = HACKY_GLOBAL_RIG_POINTER->getGeometryToRigTransform();
            glm::mat4 rigToAvatarMat = createMatFromQuatAndPos(Quaternions::Y_180, glm::vec3());

            for (auto& target : targets) {
                glm::mat4 geomTargetMat = createMatFromQuatAndPos(target.getRotation(), target.getTranslation());
                glm::mat4 avatarTargetMat = rigToAvatarMat * geomToRigMat * geomTargetMat;

                std::string name = "ikTarget" + std::to_string(target.getIndex());
                DebugDraw::getInstance().addMyAvatarMarker(name, glmExtractRotation(avatarTargetMat), extractTranslation(avatarTargetMat), WHITE);
            }
        }

        if (targets.empty()) {
            // no IK targets but still need to enforce constraints
            std::map<int, RotationConstraint*>::iterator constraintItr = _constraints.begin();
            while (constraintItr != _constraints.end()) {
                int index = constraintItr->first;
                glm::quat rotation = _relativePoses[index].rot;
                constraintItr->second->apply(rotation);
                _relativePoses[index].rot = rotation;
                ++constraintItr;
            }
        } else {

            {
                PROFILE_RANGE_EX("ik/shiftHips", 0xffff00ff, 0);

                // shift hips according to the _hipsOffset from the previous frame
                float offsetLength = glm::length(_hipsOffset);
                const float MIN_HIPS_OFFSET_LENGTH = 0.03f;
                if (offsetLength > MIN_HIPS_OFFSET_LENGTH && _hipsIndex >= 0) {
                    // but only if offset is long enough
                    float scaleFactor = ((offsetLength - MIN_HIPS_OFFSET_LENGTH) / offsetLength);
                    if (_hipsParentIndex == -1) {
                        // the hips are the root so _hipsOffset is in the correct frame
                        _relativePoses[_hipsIndex].trans = underPoses[_hipsIndex].trans + scaleFactor * _hipsOffset;
                    } else {
                        // the hips are NOT the root so we need to transform _hipsOffset into hips local-frame
                        glm::quat hipsFrameRotation = _relativePoses[_hipsParentIndex].rot;
                        int index = _skeleton->getParentIndex(_hipsParentIndex);
                        while (index != -1) {
                            hipsFrameRotation *= _relativePoses[index].rot;
                            index = _skeleton->getParentIndex(index);
                        }
                        _relativePoses[_hipsIndex].trans = underPoses[_hipsIndex].trans
                            + glm::inverse(glm::normalize(hipsFrameRotation)) * (scaleFactor * _hipsOffset);
                    }
                }
            }

            {
                PROFILE_RANGE_EX("ik/ccd", 0xffff00ff, 0);
                solveWithCyclicCoordinateDescent(targets);
            }

            {
                PROFILE_RANGE_EX("ik/measureHipsOffset", 0xffff00ff, 0);

                // measure new _hipsOffset for next frame
                // by looking for discrepancies between where a targeted endEffector is
                // and where it wants to be (after IK solutions are done)
                glm::vec3 newHipsOffset = Vectors::ZERO;
                for (auto& target: targets) {
                    int targetIndex = target.getIndex();
                    if (targetIndex == _headIndex && _headIndex != -1) {
                        // special handling for headTarget
                        if (target.getType() == IKTarget::Type::RotationOnly) {
                            // we want to shift the hips to bring the underPose closer
                            // to where the head happens to be (overpose)
                            glm::vec3 under = _skeleton->getAbsolutePose(_headIndex, underPoses).trans;
                            glm::vec3 actual = _skeleton->getAbsolutePose(_headIndex, _relativePoses).trans;
                            const float HEAD_OFFSET_SLAVE_FACTOR = 0.65f;
                            newHipsOffset += HEAD_OFFSET_SLAVE_FACTOR * (actual - under);
                        } else if (target.getType() == IKTarget::Type::HmdHead) {
                            // we want to shift the hips to bring the head to its designated position
                            glm::vec3 actual = _skeleton->getAbsolutePose(_headIndex, _relativePoses).trans;
                            _hipsOffset += target.getTranslation() - actual;
                            // and ignore all other targets
                            newHipsOffset = _hipsOffset;
                            break;
                        }
                    } else if (target.getType() == IKTarget::Type::RotationAndPosition) {
                        glm::vec3 actualPosition = _skeleton->getAbsolutePose(targetIndex, _relativePoses).trans;
                        glm::vec3 targetPosition = target.getTranslation();
                        newHipsOffset += targetPosition - actualPosition;
                    }
                }

                // smooth transitions by relaxing _hipsOffset toward the new value
                const float HIPS_OFFSET_SLAVE_TIMESCALE = 0.15f;
                float tau = dt < HIPS_OFFSET_SLAVE_TIMESCALE ?  dt / HIPS_OFFSET_SLAVE_TIMESCALE : 1.0f;
                _hipsOffset += (newHipsOffset - _hipsOffset) * tau;
            }
        }
    }
    return _relativePoses;
}

void AnimInverseKinematics::clearIKJointLimitHistory() {
    for (auto& pair : _constraints) {
        pair.second->clearHistory();
    }
}

RotationConstraint* AnimInverseKinematics::getConstraint(int index) {
    RotationConstraint* constraint = nullptr;
    std::map<int, RotationConstraint*>::iterator constraintItr = _constraints.find(index);
    if (constraintItr != _constraints.end()) {
        constraint = constraintItr->second;
    }
    return constraint;
}

void AnimInverseKinematics::clearConstraints() {
    std::map<int, RotationConstraint*>::iterator constraintItr = _constraints.begin();
    while (constraintItr != _constraints.end()) {
        delete constraintItr->second;
        ++constraintItr;
    }
    _constraints.clear();
}

void AnimInverseKinematics::initConstraints() {
    if (!_skeleton) {
        return;
    }
    // We create constraints for the joints shown here
    // (and their Left counterparts if applicable).
    //
    //
    //                                    O RightHand
    //                      Head         /
    //                          O       /
    //                      Neck|      O RightForeArm
    //                          O     /
    //                        O | O  / RightShoulder
    //      O-------O-------O' \|/ 'O
    //                   Spine2 O  RightArm
    //                          |
    //                          |
    //                   Spine1 O
    //                          |
    //                          |
    //                    Spine O
    //         y                |
    //         |                |
    //         |            O---O---O RightUpLeg
    //      z  |            | Hips2 |
    //       \ |            |       |
    //        \|            |       |
    //  x -----+            O       O RightLeg
    //                      |       |
    //                      |       |
    //                      |       |
    //                      O       O RightFoot
    //                     /       /
    //                 O--O    O--O

    loadDefaultPoses(_skeleton->getRelativeBindPoses());

    // compute corresponding absolute poses
    int numJoints = (int)_defaultRelativePoses.size();
    AnimPoseVec absolutePoses;
    absolutePoses.resize(numJoints);
    for (int i = 0; i < numJoints; ++i) {
        int parentIndex = _skeleton->getParentIndex(i);
        if (parentIndex < 0) {
            absolutePoses[i] = _defaultRelativePoses[i];
        } else {
            absolutePoses[i] = absolutePoses[parentIndex] * _defaultRelativePoses[i];
        }
    }

    clearConstraints();
    for (int i = 0; i < numJoints; ++i) {
        // compute the joint's baseName and remember whether its prefix was "Left" or not
        QString baseName = _skeleton->getJointName(i);
        bool isLeft = baseName.startsWith("Left", Qt::CaseSensitive);
        float mirror = isLeft ? -1.0f : 1.0f;
        if (isLeft) {
            baseName.remove(0, 4);
        } else if (baseName.startsWith("Right", Qt::CaseSensitive)) {
            baseName.remove(0, 5);
        }

        RotationConstraint* constraint = nullptr;
        if (0 == baseName.compare("Arm", Qt::CaseSensitive)) {
            SwingTwistConstraint* stConstraint = new SwingTwistConstraint();
            stConstraint->setReferenceRotation(_defaultRelativePoses[i].rot);
            stConstraint->setTwistLimits(-PI / 2.0f, PI / 2.0f);

            /* KEEP THIS CODE for future experimentation
            // these directions are approximate swing limits in root-frame
            // NOTE: they don't need to be normalized
            std::vector<glm::vec3> swungDirections;
            swungDirections.push_back(glm::vec3(mirror * 1.0f, 1.0f, 1.0f));
            swungDirections.push_back(glm::vec3(mirror * 1.0f, 0.0f, 1.0f));
            swungDirections.push_back(glm::vec3(mirror * 1.0f, -1.0f, 0.5f));
            swungDirections.push_back(glm::vec3(mirror * 0.0f, -1.0f, 0.0f));
            swungDirections.push_back(glm::vec3(mirror * 0.0f, -1.0f, -1.0f));
            swungDirections.push_back(glm::vec3(mirror * -0.5f, 0.0f, -1.0f));
            swungDirections.push_back(glm::vec3(mirror * 0.0f, 1.0f, -1.0f));
            swungDirections.push_back(glm::vec3(mirror * 0.0f, 1.0f, 0.0f));

            // rotate directions into joint-frame
            glm::quat invAbsoluteRotation = glm::inverse(absolutePoses[i].rot);
            int numDirections = (int)swungDirections.size();
            for (int j = 0; j < numDirections; ++j) {
                swungDirections[j] = invAbsoluteRotation * swungDirections[j];
            }
            stConstraint->setSwingLimits(swungDirections);
            */

            // simple cone
            std::vector<float> minDots;
            const float MAX_HAND_SWING = PI / 2.0f;
            minDots.push_back(cosf(MAX_HAND_SWING));
            stConstraint->setSwingLimits(minDots);

            constraint = static_cast<RotationConstraint*>(stConstraint);
        } else if (0 == baseName.compare("UpLeg", Qt::CaseSensitive)) {
            SwingTwistConstraint* stConstraint = new SwingTwistConstraint();
            stConstraint->setReferenceRotation(_defaultRelativePoses[i].rot);
            stConstraint->setTwistLimits(-PI / 4.0f, PI / 4.0f);

            std::vector<glm::vec3> swungDirections;
            float deltaTheta = PI / 4.0f;
            float theta = 0.0f;
            swungDirections.push_back(glm::vec3(mirror * cosf(theta), 0.25f, sinf(theta)));
            theta += deltaTheta;
            swungDirections.push_back(glm::vec3(mirror * cosf(theta), 0.0f, sinf(theta)));
            theta += deltaTheta;
            swungDirections.push_back(glm::vec3(mirror * cosf(theta), -0.25f, sinf(theta))); // posterior
            theta += deltaTheta;
            swungDirections.push_back(glm::vec3(mirror * cosf(theta), 0.0f, sinf(theta)));
            theta += deltaTheta;
            swungDirections.push_back(glm::vec3(mirror * cosf(theta), 0.25f, sinf(theta)));
            theta += deltaTheta;
            swungDirections.push_back(glm::vec3(mirror * cosf(theta), 0.5f, sinf(theta)));
            theta += deltaTheta;
            swungDirections.push_back(glm::vec3(mirror * cosf(theta), 0.5f, sinf(theta))); // anterior
            theta += deltaTheta;
            swungDirections.push_back(glm::vec3(mirror * cosf(theta), 0.5f, sinf(theta)));

            // rotate directions into joint-frame
            glm::quat invAbsoluteRotation = glm::inverse(absolutePoses[i].rot);
            int numDirections = (int)swungDirections.size();
            for (int j = 0; j < numDirections; ++j) {
                swungDirections[j] = invAbsoluteRotation * swungDirections[j];
            }
            stConstraint->setSwingLimits(swungDirections);

            constraint = static_cast<RotationConstraint*>(stConstraint);
        } else if (0 == baseName.compare("Hand", Qt::CaseSensitive)) {
            SwingTwistConstraint* stConstraint = new SwingTwistConstraint();
            stConstraint->setReferenceRotation(_defaultRelativePoses[i].rot);
            stConstraint->setTwistLimits(0.0f, 0.0f); // max == min, disables twist limits

            /* KEEP THIS CODE for future experimentation -- twist limits for hands
            const float MAX_HAND_TWIST = 3.0f * PI / 5.0f;
            const float MIN_HAND_TWIST = -PI / 2.0f;
            if (isLeft) {
                stConstraint->setTwistLimits(-MAX_HAND_TWIST, -MIN_HAND_TWIST);
            } else {
                stConstraint->setTwistLimits(MIN_HAND_TWIST, MAX_HAND_TWIST);
            }
            */

            /* KEEP THIS CODE for future experimentation -- non-symmetrical swing limits for wrist
             * a more complicated wrist with asymmetric cone
            // these directions are approximate swing limits in parent-frame
            // NOTE: they don't need to be normalized
            std::vector<glm::vec3> swungDirections;
            swungDirections.push_back(glm::vec3(1.0f, 1.0f, 0.0f));
            swungDirections.push_back(glm::vec3(0.75f, 1.0f, -1.0f));
            swungDirections.push_back(glm::vec3(-0.75f, 1.0f, -1.0f));
            swungDirections.push_back(glm::vec3(-1.0f, 1.0f, 0.0f));
            swungDirections.push_back(glm::vec3(-0.75f, 1.0f, 1.0f));
            swungDirections.push_back(glm::vec3(0.75f, 1.0f, 1.0f));

            // rotate directions into joint-frame
            glm::quat invRelativeRotation = glm::inverse(_defaultRelativePoses[i].rot);
            int numDirections = (int)swungDirections.size();
            for (int j = 0; j < numDirections; ++j) {
                swungDirections[j] = invRelativeRotation * swungDirections[j];
            }
            stConstraint->setSwingLimits(swungDirections);
            */

            // simple cone
            std::vector<float> minDots;
            const float MAX_HAND_SWING = PI / 2.0f;
            minDots.push_back(cosf(MAX_HAND_SWING));
            stConstraint->setSwingLimits(minDots);

            constraint = static_cast<RotationConstraint*>(stConstraint);
        } else if (baseName.startsWith("Shoulder", Qt::CaseSensitive)) {
            SwingTwistConstraint* stConstraint = new SwingTwistConstraint();
            stConstraint->setReferenceRotation(_defaultRelativePoses[i].rot);
            const float MAX_SHOULDER_TWIST = PI / 20.0f;
            stConstraint->setTwistLimits(-MAX_SHOULDER_TWIST, MAX_SHOULDER_TWIST);

            std::vector<float> minDots;
            const float MAX_SHOULDER_SWING = PI / 20.0f;
            minDots.push_back(cosf(MAX_SHOULDER_SWING));
            stConstraint->setSwingLimits(minDots);

            constraint = static_cast<RotationConstraint*>(stConstraint);
        } else if (baseName.startsWith("Spine", Qt::CaseSensitive)) {
            SwingTwistConstraint* stConstraint = new SwingTwistConstraint();
            stConstraint->setReferenceRotation(_defaultRelativePoses[i].rot);
            const float MAX_SPINE_TWIST = PI / 12.0f;
            stConstraint->setTwistLimits(-MAX_SPINE_TWIST, MAX_SPINE_TWIST);

            std::vector<float> minDots;
            const float MAX_SPINE_SWING = PI / 14.0f;
            minDots.push_back(cosf(MAX_SPINE_SWING));
            stConstraint->setSwingLimits(minDots);
            if (0 == baseName.compare("Spine1", Qt::CaseSensitive)
                    || 0 == baseName.compare("Spine", Qt::CaseSensitive)) {
                stConstraint->setLowerSpine(true);
            }

            constraint = static_cast<RotationConstraint*>(stConstraint);
        } else if (baseName.startsWith("Hips2", Qt::CaseSensitive)) {
            SwingTwistConstraint* stConstraint = new SwingTwistConstraint();
            stConstraint->setReferenceRotation(_defaultRelativePoses[i].rot);
            const float MAX_SPINE_TWIST = PI / 8.0f;
            stConstraint->setTwistLimits(-MAX_SPINE_TWIST, MAX_SPINE_TWIST);

            std::vector<float> minDots;
            const float MAX_SPINE_SWING = PI / 14.0f;
            minDots.push_back(cosf(MAX_SPINE_SWING));
            stConstraint->setSwingLimits(minDots);

            constraint = static_cast<RotationConstraint*>(stConstraint);
        } else if (0 == baseName.compare("Neck", Qt::CaseSensitive)) {
            SwingTwistConstraint* stConstraint = new SwingTwistConstraint();
            stConstraint->setReferenceRotation(_defaultRelativePoses[i].rot);
            const float MAX_NECK_TWIST = PI / 9.0f;
            stConstraint->setTwistLimits(-MAX_NECK_TWIST, MAX_NECK_TWIST);

            std::vector<float> minDots;
            const float MAX_NECK_SWING = PI / 8.0f;
            minDots.push_back(cosf(MAX_NECK_SWING));
            stConstraint->setSwingLimits(minDots);

            constraint = static_cast<RotationConstraint*>(stConstraint);
        } else if (0 == baseName.compare("Head", Qt::CaseSensitive)) {
            SwingTwistConstraint* stConstraint = new SwingTwistConstraint();
            stConstraint->setReferenceRotation(_defaultRelativePoses[i].rot);
            const float MAX_HEAD_TWIST = PI / 9.0f;
            stConstraint->setTwistLimits(-MAX_HEAD_TWIST, MAX_HEAD_TWIST);

            std::vector<float> minDots;
            const float MAX_HEAD_SWING = PI / 10.0f;
            minDots.push_back(cosf(MAX_HEAD_SWING));
            stConstraint->setSwingLimits(minDots);

            constraint = static_cast<RotationConstraint*>(stConstraint);
        } else if (0 == baseName.compare("ForeArm", Qt::CaseSensitive)) {
            // The elbow joint rotates about the parent-frame's zAxis (-zAxis) for the Right (Left) arm.
            ElbowConstraint* eConstraint = new ElbowConstraint();
            glm::quat referenceRotation = _defaultRelativePoses[i].rot;
            eConstraint->setReferenceRotation(referenceRotation);

            // we determine the max/min angles by rotating the swing limit lines from parent- to child-frame
            // then measure the angles to swing the yAxis into alignment
            glm::vec3 hingeAxis = - mirror * Vectors::UNIT_Z;
            const float MIN_ELBOW_ANGLE = 0.05f;
            const float MAX_ELBOW_ANGLE = 11.0f * PI / 12.0f;
            glm::quat invReferenceRotation = glm::inverse(referenceRotation);
            glm::vec3 minSwingAxis = invReferenceRotation * glm::angleAxis(MIN_ELBOW_ANGLE, hingeAxis) * Vectors::UNIT_Y;
            glm::vec3 maxSwingAxis = invReferenceRotation * glm::angleAxis(MAX_ELBOW_ANGLE, hingeAxis) * Vectors::UNIT_Y;

            // for the rest of the math we rotate hingeAxis into the child frame
            hingeAxis = referenceRotation * hingeAxis;
            eConstraint->setHingeAxis(hingeAxis);

            glm::vec3 projectedYAxis = glm::normalize(Vectors::UNIT_Y - glm::dot(Vectors::UNIT_Y, hingeAxis) * hingeAxis);
            float minAngle = acosf(glm::dot(projectedYAxis, minSwingAxis));
            if (glm::dot(hingeAxis, glm::cross(projectedYAxis, minSwingAxis)) < 0.0f) {
                minAngle = - minAngle;
            }
            float maxAngle = acosf(glm::dot(projectedYAxis, maxSwingAxis));
            if (glm::dot(hingeAxis, glm::cross(projectedYAxis, maxSwingAxis)) < 0.0f) {
                maxAngle = - maxAngle;
            }
            eConstraint->setAngleLimits(minAngle, maxAngle);

            constraint = static_cast<RotationConstraint*>(eConstraint);
        } else if (0 == baseName.compare("Leg", Qt::CaseSensitive)) {
            // The knee joint rotates about the parent-frame's -xAxis.
            ElbowConstraint* eConstraint = new ElbowConstraint();
            glm::quat referenceRotation = _defaultRelativePoses[i].rot;
            eConstraint->setReferenceRotation(referenceRotation);
            glm::vec3 hingeAxis = -1.0f * Vectors::UNIT_X;

            // we determine the max/min angles by rotating the swing limit lines from parent- to child-frame
            // then measure the angles to swing the yAxis into alignment
            const float MIN_KNEE_ANGLE = 0.0f;
            const float MAX_KNEE_ANGLE = 7.0f * PI / 8.0f;
            glm::quat invReferenceRotation = glm::inverse(referenceRotation);
            glm::vec3 minSwingAxis = invReferenceRotation * glm::angleAxis(MIN_KNEE_ANGLE, hingeAxis) * Vectors::UNIT_Y;
            glm::vec3 maxSwingAxis = invReferenceRotation * glm::angleAxis(MAX_KNEE_ANGLE, hingeAxis) * Vectors::UNIT_Y;

            // for the rest of the math we rotate hingeAxis into the child frame
            hingeAxis = referenceRotation * hingeAxis;
            eConstraint->setHingeAxis(hingeAxis);

            glm::vec3 projectedYAxis = glm::normalize(Vectors::UNIT_Y - glm::dot(Vectors::UNIT_Y, hingeAxis) * hingeAxis);
            float minAngle = acosf(glm::dot(projectedYAxis, minSwingAxis));
            if (glm::dot(hingeAxis, glm::cross(projectedYAxis, minSwingAxis)) < 0.0f) {
                minAngle = - minAngle;
            }
            float maxAngle = acosf(glm::dot(projectedYAxis, maxSwingAxis));
            if (glm::dot(hingeAxis, glm::cross(projectedYAxis, maxSwingAxis)) < 0.0f) {
                maxAngle = - maxAngle;
            }
            eConstraint->setAngleLimits(minAngle, maxAngle);

            constraint = static_cast<RotationConstraint*>(eConstraint);
        } else if (0 == baseName.compare("Foot", Qt::CaseSensitive)) {
            SwingTwistConstraint* stConstraint = new SwingTwistConstraint();
            stConstraint->setReferenceRotation(_defaultRelativePoses[i].rot);
            stConstraint->setTwistLimits(-PI / 4.0f, PI / 4.0f);

            // these directions are approximate swing limits in parent-frame
            // NOTE: they don't need to be normalized
            std::vector<glm::vec3> swungDirections;
            swungDirections.push_back(Vectors::UNIT_Y);
            swungDirections.push_back(Vectors::UNIT_X);
            swungDirections.push_back(glm::vec3(1.0f, 1.0f, 1.0f));
            swungDirections.push_back(glm::vec3(1.0f, 1.0f, -1.0f));

            // rotate directions into joint-frame
            glm::quat invRelativeRotation = glm::inverse(_defaultRelativePoses[i].rot);
            int numDirections = (int)swungDirections.size();
            for (int j = 0; j < numDirections; ++j) {
                swungDirections[j] = invRelativeRotation * swungDirections[j];
            }
            stConstraint->setSwingLimits(swungDirections);

            constraint = static_cast<RotationConstraint*>(stConstraint);
        }
        if (constraint) {
            _constraints[i] = constraint;
        }
    }
}

void AnimInverseKinematics::setSkeletonInternal(AnimSkeleton::ConstPointer skeleton) {
    AnimNode::setSkeletonInternal(skeleton);

    // invalidate all targetVars
    for (auto& targetVar: _targetVarVec) {
        targetVar.jointIndex = -1;
    }

    _maxTargetIndex = -1;

    for (auto& accumulator: _accumulators) {
        accumulator.clearAndClean();
    }

    if (skeleton) {
        initConstraints();
        _headIndex = _skeleton->nameToJointIndex("Head");
        _hipsIndex = _skeleton->nameToJointIndex("Hips");

        // also cache the _hipsParentIndex for later
        if (_hipsIndex >= 0) {
            _hipsParentIndex = _skeleton->getParentIndex(_hipsIndex);
        } else {
            _hipsParentIndex = -1;
        }
    } else {
        clearConstraints();
        _headIndex = -1;
        _hipsIndex = -1;
        _hipsParentIndex = -1;
    }
}
