//
//  AnimArmIK.h
//
//  Created by Angus Antley on 1/9/19.
//  Copyright (c) 2019 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimArmIK_h
#define hifi_AnimArmIK_h

//#include "AnimNode.h"
#include "AnimTwoBoneIK.h"
//#include "AnimChain.h"

// Simple two bone IK chain
class AnimArmIK : public AnimTwoBoneIK {
public:
    AnimArmIK(const QString& id, float alpha, bool enabled, float interpDuration,
        const QString& baseJointName, const QString& midJointName,
        const QString& tipJointName, const glm::vec3& midHingeAxis,
        const QString& alphaVar, const QString& enabledVar,
        const QString& endEffectorRotationVarVar, const QString& endEffectorPositionVarVar);
    virtual ~AnimArmIK();
    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, AnimVariantMap& triggersOut) override;

protected:

};

#endif // hifi_AnimArmIK_h

