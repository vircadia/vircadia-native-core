//
//  AnimBlendDirectonal.h
//
//  Created by Anthony J. Thibault on Augest 30 2019.
//  Copyright (c) 2019 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimBlendDirectional_h
#define hifi_AnimBlendDirectional_h

#include "AnimNode.h"

// blend between up to nine AnimNodes.

class AnimBlendDirectional : public AnimNode {
public:
    friend class AnimTests;

    AnimBlendDirectional(const QString& id, glm::vec3 alpha, const QString& centerId,
                         const QString& upId, const QString& downId, const QString& leftId, const QString& rightId,
                         const QString& upLeftId, const QString& upRightId, const QString& downLeftId, const QString& downRightId);
    virtual ~AnimBlendDirectional() override;

    virtual const AnimPoseVec& evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, AnimVariantMap& triggersOut) override;

    void setAlphaVar(const QString& alphaVar) { _alphaVar = alphaVar; }

    bool lookupChildIds();

protected:
    // for AnimDebugDraw rendering
    virtual const AnimPoseVec& getPosesInternal() const override;

    AnimPoseVec _poses;

    glm::vec3 _alpha;
    QString _centerId;
    QString _upId;
    QString _downId;
    QString _leftId;
    QString _rightId;
    QString _upLeftId;
    QString _upRightId;
    QString _downLeftId;
    QString _downRightId;

    QString _alphaVar;

    int _childIndices[3][3];

    // no copies
    AnimBlendDirectional(const AnimBlendDirectional&) = delete;
    AnimBlendDirectional& operator=(const AnimBlendDirectional&) = delete;
};

#endif // hifi_AnimBlendDirectional_h
