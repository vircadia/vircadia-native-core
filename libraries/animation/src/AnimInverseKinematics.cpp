//
//  AnimInverseKinematics.cpp
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimInverseKinematics.h"

#include <NumericalConstants.h>
#include <SharedUtil.h>

#include "ElbowConstraint.h"
#include "SwingTwistConstraint.h"

AnimInverseKinematics::AnimInverseKinematics(const std::string& id) : AnimNode(AnimNode::Type::InverseKinematics, id) {
}

AnimInverseKinematics::~AnimInverseKinematics() {
    clearConstraints();
}

void AnimInverseKinematics::loadDefaultPoses(const AnimPoseVec& poses) {
    _defaultRelativePoses = poses;
    assert(_skeleton && _skeleton->getNumJoints() == (int)poses.size());
}

void AnimInverseKinematics::loadPoses(const AnimPoseVec& poses) {
    assert(_skeleton && _skeleton->getNumJoints() == (int)poses.size());
    if (_skeleton->getNumJoints() == (int)poses.size()) {
        _relativePoses = poses;
    } else {
        _relativePoses.clear();
    }
}

void AnimInverseKinematics::computeAbsolutePoses(AnimPoseVec& absolutePoses) const {
    int numJoints = (int)_relativePoses.size();
    absolutePoses.clear();
    absolutePoses.reserve(numJoints);
    assert(numJoints <= _skeleton->getNumJoints());
    for (int i = 0; i < numJoints; ++i) {
        int parentIndex = _skeleton->getParentIndex(i);
        if (parentIndex < 0) {
            absolutePoses[i] = _relativePoses[i];
        } else {
            absolutePoses[i] = absolutePoses[parentIndex] * _relativePoses[i];
        }
    }
}

void AnimInverseKinematics::updateTarget(int index, const glm::vec3& position, const glm::quat& rotation) {
    std::map<int, IKTarget>::iterator targetItr = _absoluteTargets.find(index);
    if (targetItr != _absoluteTargets.end()) {
        // update existing target
        targetItr->second.pose.rot = rotation;
        targetItr->second.pose.trans = position;
    } else {
        // add new target
        assert(index >= 0 && index < _skeleton->getNumJoints());

        IKTarget target;
        target.pose = AnimPose(glm::vec3(1.0f), rotation, position);

        // walk down the skeleton hierarchy to find the joint's root
        int rootIndex = -1;
        int parentIndex = _skeleton->getParentIndex(index);
        while (parentIndex != -1) {
            rootIndex = parentIndex;
            parentIndex = _skeleton->getParentIndex(parentIndex);
        }
        target.rootIndex = rootIndex;

        _absoluteTargets[index] = target;
        if (index > _maxTargetIndex) {
            _maxTargetIndex = index;
        }
    }
}

void AnimInverseKinematics::updateTarget(const QString& name, const glm::vec3& position, const glm::quat& rotation) {
    int index = _skeleton->nameToJointIndex(name);
    if (index != -1) {
        updateTarget(index, position, rotation);
    }
}

void AnimInverseKinematics::clearTarget(int index) {
    _absoluteTargets.erase(index);

    // recompute _maxTargetIndex
    _maxTargetIndex = 0;
    for (auto& targetPair: _absoluteTargets) {
        int targetIndex = targetPair.first;
        if (targetIndex < _maxTargetIndex) {
            _maxTargetIndex = targetIndex;
        }
    }
}

void AnimInverseKinematics::clearAllTargets() {
    _absoluteTargets.clear();
    _maxTargetIndex = 0;
}

//virtual
const AnimPoseVec& AnimInverseKinematics::evaluate(const AnimVariantMap& animVars, float dt, AnimNode::Triggers& triggersOut) {
    // NOTE: we assume that _relativePoses are up to date (e.g. loadPoses() was just called)
    if (_relativePoses.empty()) {
        return _relativePoses;
    }

    relaxTowardDefaults(dt);

    if (_absoluteTargets.empty()) {
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
        // compute absolute poses that correspond to relative target poses
        AnimPoseVec absolutePoses;
        computeAbsolutePoses(absolutePoses);

        float largestError = 0.0f;
        const float ACCEPTABLE_RELATIVE_ERROR = 1.0e-3f;
        int numLoops = 0;
        const int MAX_IK_LOOPS = 16;
        const quint64 MAX_IK_TIME = 10 * USECS_PER_MSEC;
        quint64 expiry = usecTimestampNow() + MAX_IK_TIME;
        do {
            largestError = 0.0f;
            for (auto& targetPair: _absoluteTargets) {
                int lowestMovedIndex = _relativePoses.size() - 1;
                int tipIndex = targetPair.first;
                AnimPose targetPose = targetPair.second.pose;
                int rootIndex = targetPair.second.rootIndex;
                if (rootIndex != -1) {
                    // transform targetPose into skeleton's absolute frame
                    AnimPose& rootPose = _relativePoses[rootIndex];
                    targetPose.trans = rootPose.trans + rootPose.rot * targetPose.trans;
                    targetPose.rot = rootPose.rot * targetPose.rot;
                }

                glm::vec3 tip = absolutePoses[tipIndex].trans;
                float error = glm::length(targetPose.trans - tip);
                if (error < ACCEPTABLE_RELATIVE_ERROR) {
                    if (largestError < error) {
                        largestError = error;
                    }
                    // this targetPose has been met
                    // finally set the relative rotation of the tip to agree with absolute target rotation
                    int parentIndex = _skeleton->getParentIndex(tipIndex);
                    if (parentIndex != -1) {
                        // compute tip's new parent-relative rotation
                        // Q = Qp * q   -->   q' = Qp^ * Q
                        glm::quat newRelativeRotation = glm::inverse(absolutePoses[parentIndex].rot) * targetPose.rot;
                        RotationConstraint* constraint = getConstraint(tipIndex);
                        if (constraint) {
                            constraint->apply(newRelativeRotation);
                            // TODO: ATM the final rotation target may fails but we need to provide
                            // feedback to the IK system so that it can adjust the bones up the skeleton
                            // to help this rotation target get met.
                        }
                        _relativePoses[tipIndex].rot = newRelativeRotation;
                    }
                    break;
                }

                // descend toward root, rotating each joint to get tip closer to target
                int index = _skeleton->getParentIndex(tipIndex);
                while (index != -1 && error > ACCEPTABLE_RELATIVE_ERROR) {
                    // compute the two lines that should be aligned
                    glm::vec3 jointPosition = absolutePoses[index].trans;
                    glm::vec3 leverArm = tip - jointPosition;
                    glm::vec3 pivotLine = targetPose.trans - jointPosition;

                    // compute the axis of the rotation that would align them
                    glm::vec3 axis = glm::cross(leverArm, pivotLine);
                    float axisLength = glm::length(axis);
                    if (axisLength > EPSILON) {
                        // compute deltaRotation for alignment (brings tip closer to target)
                        axis /= axisLength;
                        float angle = acosf(glm::dot(leverArm, pivotLine) / (glm::length(leverArm) * glm::length(pivotLine)));

                        // NOTE: even when axisLength is not zero (e.g. lever-arm and pivot-arm are not quite aligned) it is
                        // still possible for the angle to be zero so we also check that to avoid unnecessary calculations.
                        if (angle > EPSILON) {
                            glm::quat deltaRotation = glm::angleAxis(angle, axis);

                            int parentIndex = _skeleton->getParentIndex(index);
                            if (parentIndex == -1) {
                                // TODO? apply constraints to root?
                                // TODO? harvest the root's transform as movement of entire skeleton?
                            } else {
                                // compute joint's new parent-relative rotation
                                // Q' = dQ * Q   and   Q = Qp * q   -->   q' = Qp^ * dQ * Q
                                glm::quat newRot = glm::normalize(glm::inverse(absolutePoses[parentIndex].rot) * deltaRotation * absolutePoses[index].rot);
                                RotationConstraint* constraint = getConstraint(index);
                                if (constraint) {
                                    constraint->apply(newRot);
                                }
                                _relativePoses[index].rot = newRot;
                            }
                            // this joint has been changed so we check to see if it has the lowest index
                            if (index < lowestMovedIndex) {
                                lowestMovedIndex = index;
                            }

                            // keep track of tip's new position as we descend towards root
                            tip = jointPosition + deltaRotation * leverArm;
                            error = glm::length(targetPose.trans - tip);
                        }
                    }
                    index = _skeleton->getParentIndex(index);
                }
                if (largestError < error) {
                    largestError = error;
                }

                if (lowestMovedIndex <= _maxTargetIndex && lowestMovedIndex < tipIndex) {
                    // only update the absolutePoses that matter: those between lowestMovedIndex and _maxTargetIndex
                    for (int i = lowestMovedIndex; i <= _maxTargetIndex; ++i) {
                        int parentIndex = _skeleton->getParentIndex(i);
                        if (parentIndex != -1) {
                            absolutePoses[i] = absolutePoses[parentIndex] * _relativePoses[i];
                        }
                    }
                }

                // finally set the relative rotation of the tip to agree with absolute target rotation
                int parentIndex = _skeleton->getParentIndex(tipIndex);
                if (parentIndex != -1) {
                    // compute tip's new parent-relative rotation
                    // Q = Qp * q   -->   q' = Qp^ * Q
                    glm::quat newRelativeRotation = glm::inverse(absolutePoses[parentIndex].rot) * targetPose.rot;
                    RotationConstraint* constraint = getConstraint(tipIndex);
                    if (constraint) {
                        constraint->apply(newRelativeRotation);
                        // TODO: ATM the final rotation target just fails but we need to provide
                        // feedback to the IK system so that it can adjust the bones up the skeleton
                        // to help this rotation target get met.
                    }
                    _relativePoses[tipIndex].rot = newRelativeRotation;
                    absolutePoses[tipIndex].rot = targetPose.rot;
                }
            }
            ++numLoops;
        } while (largestError > ACCEPTABLE_RELATIVE_ERROR && numLoops < MAX_IK_LOOPS && usecTimestampNow() < expiry);
    }
    return _relativePoses;
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

const glm::vec3 xAxis(1.0f, 0.0f, 0.0f);
const glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
const glm::vec3 zAxis(0.0f, 0.0f, 1.0f);

void AnimInverseKinematics::initConstraints() {
    if (!_skeleton) {
        return;
    }
    // We create constraints for the joints shown here 
    // (and their Left counterparts if applicable).
    //
    //                                     O RightHand
    //                                    /
    //                          O        /
    //                          |       O RightForeArm
    //                     Neck O      /
    //                          |     /
    //     O-------O-------O----O----O RightArm
    //                    Spine2|
    //                          |
    //                          O Spine1
    //                          |
    //                          |
    //                          O Spine
    //                          |
    //                          |
    //                      O---O---O RightUpLeg
    //                      |       |
    //                      |       |
    //                      |       |
    //                      O       O RightLeg
    //                      |       |
    //        y             |       |
    //        |             |       |
    //        |             O       O RightFoot
    //     z  |            /       /
    //      \ |        O--O    O--O
    //       \|
    // x -----+

    loadDefaultPoses(_skeleton->getRelativeBindPoses());

    // compute corresponding absolute poses
    int numJoints = (int)_defaultRelativePoses.size();
    AnimPoseVec absolutePoses;
    absolutePoses.reserve(numJoints);
    for (int i = 0; i < numJoints; ++i) {
        int parentIndex = _skeleton->getParentIndex(i);
        if (parentIndex < 0) {
            absolutePoses[i] = _defaultRelativePoses[i];
        } else {
            absolutePoses[i] = absolutePoses[parentIndex] * _defaultRelativePoses[i];
        }
    }

    _constraints.clear();
    for (int i = 0; i < numJoints; ++i) {
        QString name = _skeleton->getJointName(i);
        bool isLeft = name.startsWith("Left", Qt::CaseInsensitive);
        float mirror = isLeft ? -1.0f : 1.0f;
        if (isLeft) {
            //name.remove(0, 4);
        } else if (name.startsWith("Right", Qt::CaseInsensitive)) {
            //name.remove(0, 5);
        }

        RotationConstraint* constraint = nullptr;
        if (0 == name.compare("Arm", Qt::CaseInsensitive)) {
            SwingTwistConstraint* stConstraint = new SwingTwistConstraint();
            stConstraint->setReferenceRotation(_defaultRelativePoses[i].rot);
            stConstraint->setTwistLimits(-PI / 2.0f, PI / 2.0f);

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

            constraint = static_cast<RotationConstraint*>(stConstraint);
        } else if (0 == name.compare("UpLeg", Qt::CaseInsensitive)) {
            SwingTwistConstraint* stConstraint = new SwingTwistConstraint();
            stConstraint->setReferenceRotation(_defaultRelativePoses[i].rot);
            stConstraint->setTwistLimits(-PI / 4.0f, PI / 4.0f);

            // these directions are approximate swing limits in root-frame
            // NOTE: they don't need to be normalized
            std::vector<glm::vec3> swungDirections;
            swungDirections.push_back(glm::vec3(mirror * 0.25f, 0.0f, 1.0f));
            swungDirections.push_back(glm::vec3(mirror * -0.5f, 0.0f, 1.0f));
            swungDirections.push_back(glm::vec3(mirror * -1.0f, 0.0f, 1.0f));
            swungDirections.push_back(glm::vec3(mirror * -1.0f, 0.0f, 0.0f));
            swungDirections.push_back(glm::vec3(mirror * -0.5f, -0.5f, -1.0f));
            swungDirections.push_back(glm::vec3(mirror * 0.0f, -0.75f, -1.0f));
            swungDirections.push_back(glm::vec3(mirror * 0.25f, -1.0f, 0.0f));
            swungDirections.push_back(glm::vec3(mirror * 0.25f, -1.0f, 1.0f));

            // rotate directions into joint-frame
            glm::quat invAbsoluteRotation = glm::inverse(absolutePoses[i].rot);
            int numDirections = (int)swungDirections.size();
            for (int j = 0; j < numDirections; ++j) {
                swungDirections[j] = invAbsoluteRotation * swungDirections[j];
            }
            stConstraint->setSwingLimits(swungDirections);

            constraint = static_cast<RotationConstraint*>(stConstraint);
        } else if (0 == name.compare("RightHand", Qt::CaseInsensitive)) {
            SwingTwistConstraint* stConstraint = new SwingTwistConstraint();
            stConstraint->setReferenceRotation(_defaultRelativePoses[i].rot);
            const float MAX_HAND_TWIST = PI / 2.0f;
            stConstraint->setTwistLimits(-MAX_HAND_TWIST, MAX_HAND_TWIST);

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

            /*
            std::vector<float> minDots;
            const float MAX_HAND_SWING = PI / 3.0f;
            minDots.push_back(cosf(MAX_HAND_SWING));
            stConstraint->setSwingLimits(minDots);
            */
            constraint = static_cast<RotationConstraint*>(stConstraint);
        } else if (name.startsWith("SpineXXX", Qt::CaseInsensitive)) {
            SwingTwistConstraint* stConstraint = new SwingTwistConstraint();
            stConstraint->setReferenceRotation(_defaultRelativePoses[i].rot);
            const float MAX_SPINE_TWIST = PI / 8.0f;
            stConstraint->setTwistLimits(-MAX_SPINE_TWIST, MAX_SPINE_TWIST);

            std::vector<float> minDots;
            const float MAX_SPINE_SWING = PI / 14.0f;
            minDots.push_back(cosf(MAX_SPINE_SWING));
            stConstraint->setSwingLimits(minDots);

            constraint = static_cast<RotationConstraint*>(stConstraint);
        } else if (0 == name.compare("NeckXXX", Qt::CaseInsensitive)) {
            SwingTwistConstraint* stConstraint = new SwingTwistConstraint();
            stConstraint->setReferenceRotation(_defaultRelativePoses[i].rot);
            const float MAX_NECK_TWIST = PI / 2.0f;
            stConstraint->setTwistLimits(-MAX_NECK_TWIST, MAX_NECK_TWIST);

            std::vector<float> minDots;
            const float MAX_NECK_SWING = PI / 3.0f;
            minDots.push_back(cosf(MAX_NECK_SWING));
            stConstraint->setSwingLimits(minDots);

            constraint = static_cast<RotationConstraint*>(stConstraint);
        } else if (0 == name.compare("ForeArm", Qt::CaseInsensitive)) {
            // The elbow joint rotates about the parent-frame's zAxis (-zAxis) for the Right (Left) arm.
            ElbowConstraint* eConstraint = new ElbowConstraint();
            glm::quat referenceRotation = _defaultRelativePoses[i].rot;
            eConstraint->setReferenceRotation(referenceRotation);

            // we determine the max/min angles by rotating the swing limit lines from parent- to child-frame
            // then measure the angles to swing the yAxis into alignment
            glm::vec3 hingeAxis = - mirror * zAxis;
            const float MIN_ELBOW_ANGLE = 0.0f;
            const float MAX_ELBOW_ANGLE = 7.0f * PI / 8.0f;
            glm::quat invReferenceRotation = glm::inverse(referenceRotation);
            glm::vec3 minSwingAxis = invReferenceRotation * glm::angleAxis(MIN_ELBOW_ANGLE, hingeAxis) * yAxis;
            glm::vec3 maxSwingAxis = invReferenceRotation * glm::angleAxis(MAX_ELBOW_ANGLE, hingeAxis) * yAxis;

            // for the rest of the math we rotate hingeAxis into the child frame
            hingeAxis = referenceRotation * hingeAxis;
            eConstraint->setHingeAxis(hingeAxis);

            glm::vec3 projectedYAxis = glm::normalize(yAxis - glm::dot(yAxis, hingeAxis) * hingeAxis);
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
        } else if (0 == name.compare("Leg", Qt::CaseInsensitive)) {
            // The knee joint rotates about the parent-frame's -xAxis.
            ElbowConstraint* eConstraint = new ElbowConstraint();
            glm::quat referenceRotation = _defaultRelativePoses[i].rot;
            eConstraint->setReferenceRotation(referenceRotation);
            glm::vec3 hingeAxis = -1.0f * xAxis;

            // we determine the max/min angles by rotating the swing limit lines from parent- to child-frame
            // then measure the angles to swing the yAxis into alignment
            const float MIN_KNEE_ANGLE = 0.0f;
            const float MAX_KNEE_ANGLE = 3.0f * PI / 4.0f;
            glm::quat invReferenceRotation = glm::inverse(referenceRotation);
            glm::vec3 minSwingAxis = invReferenceRotation * glm::angleAxis(MIN_KNEE_ANGLE, hingeAxis) * yAxis;
            glm::vec3 maxSwingAxis = invReferenceRotation * glm::angleAxis(MAX_KNEE_ANGLE, hingeAxis) * yAxis;

            // for the rest of the math we rotate hingeAxis into the child frame
            hingeAxis = referenceRotation * hingeAxis;
            eConstraint->setHingeAxis(hingeAxis);

            glm::vec3 projectedYAxis = glm::normalize(yAxis - glm::dot(yAxis, hingeAxis) * hingeAxis);
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
        } else if (0 == name.compare("Foot", Qt::CaseInsensitive)) {
            SwingTwistConstraint* stConstraint = new SwingTwistConstraint();
            stConstraint->setReferenceRotation(_defaultRelativePoses[i].rot);
            stConstraint->setTwistLimits(-PI / 4.0f, PI / 4.0f);

            // these directions are approximate swing limits in parent-frame
            // NOTE: they don't need to be normalized
            std::vector<glm::vec3> swungDirections;
            swungDirections.push_back(yAxis);
            swungDirections.push_back(xAxis);
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
    if (skeleton) {
        initConstraints();
    } else {
        clearConstraints();
    }
}

void AnimInverseKinematics::relaxTowardDefaults(float dt) {
    // NOTE: for now we just use a single relaxation timescale for all joints, but in the future
    // we could vary the timescale on a per-joint basis or do other fancy things.

    // for each joint: lerp towards the default pose
    const float RELAXATION_TIMESCALE = 0.25f;
    const float alpha = glm::clamp(dt / RELAXATION_TIMESCALE, 0.0f, 1.0f);
    int numJoints = (int)_relativePoses.size();
    for (int i = 0; i < numJoints; ++i) {
        float dotSign = copysignf(1.0f, glm::dot(_relativePoses[i].rot, _defaultRelativePoses[i].rot));
        _relativePoses[i].rot = glm::normalize(glm::lerp(_relativePoses[i].rot, dotSign * _defaultRelativePoses[i].rot, alpha));
    }
}

