//
//  Hand.cpp
//  interface/src/avatar
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QImage>

#include <GeometryUtil.h>
#include <NodeList.h>

#include "AvatarManager.h"
#include "Hand.h"
#include "Menu.h"
#include "MyAvatar.h"
#include "Util.h"

using namespace std;

const float PALM_COLLISION_RADIUS = 0.03f;


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

void Hand::render(RenderArgs* renderArgs, bool isMine) {
    gpu::Batch& batch = *renderArgs->_batch;
    if (renderArgs->_renderMode != RenderArgs::SHADOW_RENDER_MODE &&
                Menu::getInstance()->isOptionChecked(MenuOption::RenderSkeletonCollisionShapes)) {
        // draw a green sphere at hand joint location, which is actually near the wrist)
        for (size_t i = 0; i < getNumPalms(); i++) {
            PalmData& palm = getPalms()[i];
            if (!palm.isActive()) {
                continue;
            }
            glm::vec3 position = palm.getPosition();
            Transform transform = Transform();
            transform.setTranslation(position);
            batch.setModelTransform(transform);
            DependencyManager::get<GeometryCache>()->renderSphere(batch, PALM_COLLISION_RADIUS * _owningAvatar->getScale(), 10, 10, glm::vec3(0.0f, 1.0f, 0.0f));
        }
    }
    
    if (renderArgs->_renderMode != RenderArgs::SHADOW_RENDER_MODE && Menu::getInstance()->isOptionChecked(MenuOption::DisplayHands)) {
        renderHandTargets(renderArgs, isMine);
    }

}

void Hand::renderHandTargets(RenderArgs* renderArgs, bool isMine) {
    gpu::Batch& batch = *renderArgs->_batch;
    const float avatarScale = DependencyManager::get<AvatarManager>()->getMyAvatar()->getScale();

    const float alpha = 1.0f;
    const glm::vec3 handColor(1.0, 0.0, 0.0); //  Color the hand targets red to be different than skin

    if (isMine && Menu::getInstance()->isOptionChecked(MenuOption::DisplayHandTargets)) {
        for (size_t i = 0; i < getNumPalms(); ++i) {
            PalmData& palm = getPalms()[i];
            if (!palm.isActive()) {
                continue;
            }
            glm::vec3 targetPosition = palm.getTipPosition();
            Transform transform = Transform();
            transform.setTranslation(targetPosition);
            batch.setModelTransform(transform);
        
            const float collisionRadius = 0.05f;
            DependencyManager::get<GeometryCache>()->renderSphere(batch, collisionRadius, 10, 10, glm::vec4(0.5f,0.5f,0.5f, alpha), false);
        }
    }
    
    const float PALM_BALL_RADIUS = 0.03f * avatarScale;
    const float PALM_DISK_RADIUS = 0.06f * avatarScale;
    const float PALM_DISK_THICKNESS = 0.01f * avatarScale;
    const float PALM_FINGER_ROD_RADIUS = 0.003f * avatarScale;
    
    // Draw the palm ball and disk
    for (size_t i = 0; i < getNumPalms(); ++i) {
        PalmData& palm = getPalms()[i];
        if (palm.isActive()) {
            glm::vec3 tip = palm.getTipPosition();
            glm::vec3 root = palm.getPosition();
            Transform transform = Transform();
            transform.setTranslation(glm::vec3());
            batch.setModelTransform(transform);
            Avatar::renderJointConnectingCone(batch, root, tip, PALM_FINGER_ROD_RADIUS, PALM_FINGER_ROD_RADIUS, glm::vec4(handColor.r, handColor.g, handColor.b, alpha));

            //  Render sphere at palm/finger root
            glm::vec3 offsetFromPalm = root + palm.getNormal() * PALM_DISK_THICKNESS;
            Avatar::renderJointConnectingCone(batch, root, offsetFromPalm, PALM_DISK_RADIUS, 0.0f, glm::vec4(handColor.r, handColor.g, handColor.b, alpha));
            transform = Transform();
            transform.setTranslation(root);
            batch.setModelTransform(transform);
            DependencyManager::get<GeometryCache>()->renderSphere(batch, PALM_BALL_RADIUS, 20.0f, 20.0f, glm::vec4(handColor.r, handColor.g, handColor.b, alpha));
        }
    }
}

