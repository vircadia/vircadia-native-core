//
//  SkeletonkRagdoll.h
//  interface/src/avatar
//
//  Created by Andrew Meadows 2014.08.14
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SkeletonRagdoll_h
#define hifi_SkeletonRagdoll_h

#include <QVector>

#include <Ragdoll.h>

#include "../renderer/JointState.h"

class MuscleConstraint;
class Model;

class SkeletonRagdoll : public Ragdoll {
public:

    SkeletonRagdoll(Model* model);
    virtual ~SkeletonRagdoll();

    void slamPointPositions();
    virtual void stepRagdollForward(float deltaTime);

    virtual void initRagdollPoints();
    virtual void buildRagdollConstraints();

    void updateMuscles();
private:
    Model* _model;
    QVector<MuscleConstraint*> _muscleConstraints;
};

#endif // hifi_SkeletonRagdoll_h
