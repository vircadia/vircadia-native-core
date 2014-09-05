//
//  SkeletonRagdoll.cpp
//  interface/src/avatar
//
//  Created by Andrew Meadows 2014.08.14
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <DistanceConstraint.h>
#include <FixedConstraint.h>

#include "SkeletonRagdoll.h"
#include "MuscleConstraint.h"
#include "../renderer/Model.h"

SkeletonRagdoll::SkeletonRagdoll(Model* model) : Ragdoll(), _model(model) {
    assert(_model);
}

SkeletonRagdoll::~SkeletonRagdoll() {
}

// virtual 
void SkeletonRagdoll::stepForward(float deltaTime) {
    setTransform(_model->getTranslation(), _model->getRotation());
    Ragdoll::stepForward(deltaTime);
    updateMuscles();
    int numConstraints = _muscleConstraints.size();
    for (int i = 0; i < numConstraints; ++i) {
        _muscleConstraints[i]->enforce();
    }
}

void SkeletonRagdoll::slamPointPositions() {
    QVector<JointState>& jointStates = _model->getJointStates();
    const int numPoints = _points.size();
    assert(numPoints == jointStates.size());
    for (int i = _rootIndex; i < numPoints; ++i) {
        _points[i].initPosition(jointStates.at(i).getPosition());
    }
}

// virtual
void SkeletonRagdoll::initPoints() {
    clearConstraintsAndPoints();
    _muscleConstraints.clear();

    initTransform();
    // one point for each joint
    int numStates = _model->getJointStates().size();
    _points.fill(VerletPoint(), numStates);
    slamPointPositions();
}

// virtual
void SkeletonRagdoll::buildConstraints() {
    QVector<JointState>& jointStates = _model->getJointStates();

    // NOTE: the length of DistanceConstraints is computed and locked in at this time
    // so make sure the ragdoll positions are in a normal configuration before here.
    const int numPoints = _points.size();
    assert(numPoints == jointStates.size());

    float minBone = FLT_MAX;
    float maxBone = -FLT_MAX;
    QMultiMap<int, int> families;
    for (int i = _rootIndex; i < numPoints; ++i) {
        const JointState& state = jointStates.at(i);
        int parentIndex = state.getParentIndex();
        if (parentIndex != -1) {
            DistanceConstraint* bone = new DistanceConstraint(&(_points[i]), &(_points[parentIndex]));
            bone->setDistance(state.getDistanceToParent());
            _boneConstraints.push_back(bone);
            families.insert(parentIndex, i);
        }
        float boneLength = glm::length(state.getPositionInParentFrame());
        if (boneLength > maxBone) {
            maxBone = boneLength;
        } else if (boneLength < minBone) {
            minBone = boneLength;
        }
    }
    // Joints that have multiple children effectively have rigid constraints between the children
    // in the parent frame, so we add DistanceConstraints between children in the same family.
    QMultiMap<int, int>::iterator itr = families.begin();
    while (itr != families.end()) {
        QList<int> children = families.values(itr.key());
        int numChildren = children.size();
        if (numChildren > 1) {
            for (int i = 1; i < numChildren; ++i) {
                DistanceConstraint* bone = new DistanceConstraint(&(_points[children[i-1]]), &(_points[children[i]]));
                _boneConstraints.push_back(bone);
            }
            if (numChildren > 2) {
                DistanceConstraint* bone = new DistanceConstraint(&(_points[children[numChildren-1]]), &(_points[children[0]]));
                _boneConstraints.push_back(bone);
            }
        }
        ++itr;
    }

    float MAX_STRENGTH = 0.6f;
    float MIN_STRENGTH = 0.05f;
    // each joint gets a MuscleConstraint to its parent
    for (int i = _rootIndex + 1; i < numPoints; ++i) {
        const JointState& state = jointStates.at(i);
        int p = state.getParentIndex();
        if (p == -1) {
            continue;
        }
        MuscleConstraint* constraint = new MuscleConstraint(&(_points[p]), &(_points[i]));
        _muscleConstraints.push_back(constraint);

        // Short joints are more susceptible to wiggle so we modulate the strength based on the joint's length: 
        // long = weak and short = strong.
        constraint->setIndices(p, i);
        float boneLength = glm::length(state.getPositionInParentFrame());

        float strength = MIN_STRENGTH + (MAX_STRENGTH - MIN_STRENGTH) * (maxBone - boneLength) / (maxBone - minBone);
        if (!families.contains(i)) {
            // Although muscles only pull on the children not parents, nevertheless those joints that have
            // parents AND children are more stable than joints at the end such as fingers.  For such joints we
            // bestow maximum strength which helps reduce wiggle.
            strength = MAX_MUSCLE_STRENGTH;
        }
        constraint->setStrength(strength);
    }
}

void SkeletonRagdoll::updateMuscles() {
    QVector<JointState>& jointStates = _model->getJointStates();
    int numConstraints = _muscleConstraints.size();
    glm::quat rotation = _model->getRotation();
    for (int i = 0; i < numConstraints; ++i) {
        MuscleConstraint* constraint = _muscleConstraints[i];
        int j = constraint->getParentIndex();
        int k = constraint->getChildIndex();
        assert(j != -1 && k != -1);
        // ragdollPoints are in simulation-frame but jointStates are in model-frame
        constraint->setChildOffset(rotation * (jointStates.at(k).getPosition() - jointStates.at(j).getPosition()));
    }
}
