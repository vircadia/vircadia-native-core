//
//  AnimPoleVectorConstraint.h
//
//  Created by Anthony J. Thibault on 5/25/18.
//  Copyright (c) 2018 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimPoleVectorConstraint_h
#define hifi_AnimPoleVectorConstraint_h

#include "AnimNode.h"
#include "AnimChain.h"

// Three bone IK chain

class AnimPoleVectorConstraint : public AnimNode {
public:
    friend class AnimTests;

    AnimPoleVectorConstraint(const QString& id, bool enabled, glm::vec3 referenceVector,
                             const QString& baseJointName, const QString& midJointName, const QString& tipJointName,
                             const QString& enabledVar, const QString& poleVectorVar);
    virtual ~AnimPoleVectorConstraint() override;

    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, AnimVariantMap& triggersOut) override;

protected:

    enum class InterpType {
        None = 0,
        SnapshotToUnderPoses,
        SnapshotToSolve,
        NumTypes
    };

    // for AnimDebugDraw rendering
    virtual const AnimPoseVec& getPosesInternal() const override;
    virtual void setSkeletonInternal(AnimSkeleton::ConstPointer skeleton) override;

    void lookUpIndices();
    void beginInterp(InterpType interpType, const AnimChain& chain);

    AnimPoseVec _poses;

    bool _enabled;
    glm::vec3 _referenceVector;

    QString _baseJointName;
    QString _midJointName;
    QString _tipJointName;

    QString _enabledVar;
    QString _poleVectorVar;

    int _baseParentJointIndex { -1 };
    int _baseJointIndex { -1 };
    int _midJointIndex { -1 };
    int _tipJointIndex { -1 };

    InterpType _interpType { InterpType::None };
    float _interpAlphaVel { 0.0f };
    float _interpAlpha { 0.0f };

    AnimChain _snapshotChain;

    // no copies
    AnimPoleVectorConstraint(const AnimPoleVectorConstraint&) = delete;
    AnimPoleVectorConstraint& operator=(const AnimPoleVectorConstraint&) = delete;
};

#endif // hifi_AnimPoleVectorConstraint_h
