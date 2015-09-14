//
//  Hand.cpp
//  interface/src/avatar
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Hand.h"

#include <glm/glm.hpp>

#include <GeometryUtil.h>
#include <RenderArgs.h>

#include "Avatar.h"
#include "AvatarManager.h"
#include "MyAvatar.h"
#include "Util.h"
#include "world.h"

using namespace std;

Hand::Hand(Avatar* owningAvatar) :
    HandData((AvatarData*)owningAvatar),
    _owningAvatar(owningAvatar)
{
}

void Hand::simulate(float deltaTime, bool isMine) {
    if (isMine) {
        //  Iterate hand controllers, take actions as needed
        for (size_t i = 0; i < getNumPalms(); ++i) {
            PalmData& palm = getPalms()[i];
            palm.setLastControllerButtons(palm.getControllerButtons());
        }
    }
}

void Hand::renderHandTargets(RenderArgs* renderArgs, bool isMine) {
    float avatarScale = 1.0f;
    if (_owningAvatar) {
        avatarScale = _owningAvatar->getScale();
    }

    const float alpha = 1.0f;
    const glm::vec3 redColor(1.0f, 0.0f, 0.0f); //  Color the hand targets red to be different than skin
    const glm::vec3 greenColor(0.0f, 1.0f, 0.0f); //  Color the hand targets red to be different than skin
    const glm::vec3 blueColor(0.0f, 0.0f, 1.0f); //  Color the hand targets red to be different than skin
    const glm::vec3 grayColor(0.5f);
    const int NUM_FACETS = 8;
    const float SPHERE_RADIUS = 0.03f * avatarScale;

    gpu::Batch& batch = *renderArgs->_batch;
    if (isMine) {
        for (size_t i = 0; i < getNumPalms(); i++) {
            PalmData& palm = getPalms()[i];
            if (!palm.isActive()) {
                continue;
            }
            // draw a gray sphere at the target position of the "Hand" joint
            glm::vec3 position = palm.getPosition();
            Transform transform = Transform();
            transform.setTranslation(position);
            transform.setRotation(palm.getRotation());
            batch.setModelTransform(transform);
            DependencyManager::get<GeometryCache>()->renderSphere(batch, SPHERE_RADIUS,
                    NUM_FACETS, NUM_FACETS, grayColor);
    
            // draw a green sphere at the old "finger tip"
            position = palm.getTipPosition();
            transform.setTranslation(position);
            batch.setModelTransform(transform);
            DependencyManager::get<GeometryCache>()->renderSphere(batch, SPHERE_RADIUS,
                    NUM_FACETS, NUM_FACETS, greenColor, false);
        }
    }
    
    const float AXIS_RADIUS = 0.1f * SPHERE_RADIUS;
    const float AXIS_LENGTH = 10.0f * SPHERE_RADIUS;

    // Draw the coordinate frames of the hand targets
    for (size_t i = 0; i < getNumPalms(); ++i) {
        PalmData& palm = getPalms()[i];
        if (palm.isActive()) {
            glm::vec3 root = palm.getPosition();

            const glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
            glm::quat palmRotation = palm.getRotation();
            Transform transform = Transform();
            transform.setTranslation(glm::vec3());
            batch.setModelTransform(transform);
            glm::vec3 tip = root + palmRotation * glm::vec3(AXIS_LENGTH, 0.0f, 0.0f);
            Avatar::renderJointConnectingCone(batch, root, tip, AXIS_RADIUS, AXIS_RADIUS, glm::vec4(redColor.r, redColor.g, redColor.b, alpha));

            tip = root + palmRotation * glm::vec3(0.0f, AXIS_LENGTH, 0.0f);
            Avatar::renderJointConnectingCone(batch, root, tip, AXIS_RADIUS, AXIS_RADIUS, glm::vec4(greenColor.r, greenColor.g, greenColor.b, alpha));

            tip = root + palmRotation * glm::vec3(0.0f, 0.0f, AXIS_LENGTH);
            Avatar::renderJointConnectingCone(batch, root, tip, AXIS_RADIUS, AXIS_RADIUS, glm::vec4(blueColor.r, blueColor.g, blueColor.b, alpha));
        }
    }
}

