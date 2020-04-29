//
//  AnimBlendDirectional.cpp
//
//  Created by Anthony J. Thibault on Augest 30 2019.
//  Copyright (c) 2019 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimBlendDirectional.h"
#include "GLMHelpers.h"
#include "AnimationLogging.h"
#include "AnimUtil.h"

AnimBlendDirectional::AnimBlendDirectional(const QString& id, glm::vec3 alpha, const QString& centerId,
                                           const QString& upId, const QString& downId, const QString& leftId, const QString& rightId,
                                           const QString& upLeftId, const QString& upRightId, const QString& downLeftId, const QString& downRightId) :
    AnimNode(AnimNode::Type::BlendDirectional, id),
    _alpha(alpha),
    _centerId(centerId),
    _upId(upId),
    _downId(downId),
    _leftId(leftId),
    _rightId(rightId),
    _upLeftId(upLeftId),
    _upRightId(upRightId),
    _downLeftId(downLeftId),
    _downRightId(downRightId) {

}

AnimBlendDirectional::~AnimBlendDirectional() {

}

const AnimPoseVec& AnimBlendDirectional::evaluate(const AnimVariantMap& animVars, const AnimContext& context, float dt, AnimVariantMap& triggersOut) {

    // lookupRaw don't transform the vector.
    _alpha = animVars.lookupRaw(_alphaVar, _alpha);
    float parentDebugAlpha = context.getDebugAlpha(_id);

    if (_children.size() == 9) {

        // try to keep the order the same as quadrants, for _childIndices.
        // +---+---+
        // | 1 | 0 |
        // +---+---+
        // | 2 | 3 |
        // +---+---+

        std::array<int, 4> indices;
        glm::vec2 alpha = _alpha;
        if (_alpha.x > 0.0f) {
            if (_alpha.y > 0.0f) {
                // quadrant 0
                indices = {{_childIndices[0][2], _childIndices[0][1], _childIndices[1][1], _childIndices[1][2]}};
            } else {
                // quadrant 3
                indices = {{_childIndices[1][2], _childIndices[1][1], _childIndices[2][1], _childIndices[2][2]}};
                // shift alpha up so both alpha.x and alpha.y are in the (0, 1) range.
                alpha.y += 1.0f;
            }
        } else {
            if (_alpha.y > 0.0f) {
                // quadrant 1
                indices = {{_childIndices[0][1], _childIndices[0][0], _childIndices[1][0], _childIndices[1][1]}};
                // shift alpha right so both alpha.x and alpha.y are in the (0, 1) range.
                alpha.x += 1.0f;
            } else {
                // quadrant 2
                indices = {{_childIndices[1][1], _childIndices[1][0], _childIndices[2][0], _childIndices[2][1]}};
                // shift alpha up and right so both alpha.x and alpha.y are in the (0, 1) range.
                alpha.x += 1.0f;
                alpha.y += 1.0f;
            }
        }
        std::array<float, 4> alphas = {{
            alpha.x * alpha.y,
            (1.0f - alpha.x) * alpha.y,
            (1.0f - alpha.x) * (1.0f - alpha.y),
            alpha.x * (1.0f - alpha.y)
        }};

        // evaluate children
        std::array<AnimPoseVec, 4> poseVecs;
        for (int i = 0; i < 4; i++) {
            poseVecs[i] = _children[indices[i]]->evaluate(animVars, context, dt, triggersOut);
        }

        // blend children
        size_t minSize = INT_MAX;
        for (int i = 0; i < 4; i++) {
            if (poseVecs[i].size() < minSize) {
                minSize = poseVecs[i].size();
            }
        }
        _poses.resize(minSize);
        if (minSize > 0) {
            blend4(minSize, &poseVecs[0][0], &poseVecs[1][0], &poseVecs[2][0], &poseVecs[3][0], &alphas[0], &_poses[0]);
        }

        // animation stack debug stats
        for (int i = 0; i < 9; i++) {
            context.setDebugAlpha(_children[i]->getID(), 0.0f, _children[i]->getType());
        }
        for (int i = 0; i < 4; i++) {
            context.setDebugAlpha(_children[indices[i]]->getID(), alphas[i] * parentDebugAlpha, _children[indices[i]]->getType());
        }

    } else {
        for (auto&& pose : _poses) {
            pose = AnimPose::identity;
        }
    }

    return _poses;
}

// for AnimDebugDraw rendering
const AnimPoseVec& AnimBlendDirectional::getPosesInternal() const {
    return _poses;
}

bool AnimBlendDirectional::lookupChildIds() {
    _childIndices[0][0] = findChildIndexByName(_upLeftId);
    _childIndices[0][1] = findChildIndexByName(_upId);
    _childIndices[0][2] = findChildIndexByName(_upRightId);

    _childIndices[1][0] = findChildIndexByName(_leftId);
    _childIndices[1][1] = findChildIndexByName(_centerId);
    _childIndices[1][2] = findChildIndexByName(_rightId);

    _childIndices[2][0] = findChildIndexByName(_downLeftId);
    _childIndices[2][1] = findChildIndexByName(_downId);
    _childIndices[2][2] = findChildIndexByName(_downRightId);

    // manditory children
    // TODO: currently all are manditory.
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (_childIndices[i][j] == -1) {
                qDebug(animation) << "Error in AnimBlendDirectional::lookupChildIds() could not find child[" << i << "," << j << "]";
                return false;
            }
        }
    }

    return true;
}
