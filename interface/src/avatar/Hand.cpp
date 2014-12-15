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

#include <NodeList.h>

#include <GeometryUtil.h>
#include <ProgramObject.h>

#include "Application.h"
#include "Avatar.h"
#include "Hand.h"
#include "Menu.h"
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

// We create a static CollisionList that is recycled for each collision test.
const float MAX_COLLISIONS_PER_AVATAR = 32;
static CollisionList handCollisions(MAX_COLLISIONS_PER_AVATAR);

void Hand::collideAgainstAvatar(Avatar* avatar, bool isMyHand) {
    if (!avatar || avatar == _owningAvatar) {
        // don't collide hands against ourself (that is done elsewhere)
        return;
    }

    const SkeletonModel& skeletonModel = _owningAvatar->getSkeletonModel();
    int jointIndices[2];
    jointIndices[0] = skeletonModel.getLeftHandJointIndex();
    jointIndices[1] = skeletonModel.getRightHandJointIndex();

    for (size_t i = 0; i < 2; i++) {
        int jointIndex = jointIndices[i];
        if (jointIndex < 0) {
            continue;
        }

        handCollisions.clear();
        QVector<const Shape*> shapes;
        skeletonModel.getHandShapes(jointIndex, shapes);

        if (avatar->findCollisions(shapes, handCollisions)) {
            glm::vec3 totalPenetration(0.0f);
            glm::vec3 averageContactPoint;
            for (int j = 0; j < handCollisions.size(); ++j) {
                CollisionInfo* collision = handCollisions.getCollision(j);
                totalPenetration += collision->_penetration;
                averageContactPoint += collision->_contactPoint;
            }
            if (isMyHand) {
                // our hand against other avatar 
                // TODO: resolve this penetration when we don't think the other avatar will yield
                //palm.addToPenetration(averagePenetration);
            } else {
                // someone else's hand against MyAvatar
                // TODO: submit collision info to MyAvatar which should lean accordingly
                averageContactPoint /= (float)handCollisions.size();
                avatar->applyCollision(averageContactPoint, totalPenetration);

                CollisionInfo collision;
                collision._penetration = totalPenetration;
                collision._contactPoint = averageContactPoint;
                emit avatar->collisionWithAvatar(avatar->getSessionUUID(), _owningAvatar->getSessionUUID(), collision);
            }
        }
    }
}

void Hand::resolvePenetrations() {
    for (size_t i = 0; i < getNumPalms(); ++i) {
        PalmData& palm = getPalms()[i];
        palm.resolvePenetrations();
    }
}

void Hand::render(bool isMine, Model::RenderMode renderMode) {
    if (renderMode != Model::SHADOW_RENDER_MODE && 
                Menu::getInstance()->isOptionChecked(MenuOption::RenderSkeletonCollisionShapes)) {
        // draw a green sphere at hand joint location, which is actually near the wrist)
        for (size_t i = 0; i < getNumPalms(); i++) {
            PalmData& palm = getPalms()[i];
            if (!palm.isActive()) {
                continue;
            }
            glm::vec3 position = palm.getPosition();
            glPushMatrix();
            glTranslatef(position.x, position.y, position.z);
            glColor3f(0.0f, 1.0f, 0.0f);
            DependencyManager::get<GeometryCache>()->renderSphere(PALM_COLLISION_RADIUS * _owningAvatar->getScale(), 10, 10);
            glPopMatrix();
        }
    }
    
    if (renderMode != Model::SHADOW_RENDER_MODE && Menu::getInstance()->isOptionChecked(MenuOption::DisplayHands)) {
        renderHandTargets(isMine);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_RESCALE_NORMAL);
} 

void Hand::renderHandTargets(bool isMine) {
    glPushMatrix();

    MyAvatar* myAvatar = Application::getInstance()->getAvatar();
    const float avatarScale = Application::getInstance()->getAvatar()->getScale();

    const float alpha = 1.0f;
    const glm::vec3 handColor(1.0, 0.0, 0.0); //  Color the hand targets red to be different than skin
    
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    if (isMine && Menu::getInstance()->isOptionChecked(MenuOption::DisplayHandTargets)) {
        for (size_t i = 0; i < getNumPalms(); ++i) {
            PalmData& palm = getPalms()[i];
            if (!palm.isActive()) {
                continue;
            }
            glm::vec3 targetPosition = palm.getTipPosition();
            glPushMatrix();
            glTranslatef(targetPosition.x, targetPosition.y, targetPosition.z);
        
            const float collisionRadius = 0.05f;
            glColor4f(0.5f,0.5f,0.5f, alpha);
            glutWireSphere(collisionRadius, 10.0f, 10.0f);
            glPopMatrix();
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
            glColor4f(handColor.r, handColor.g, handColor.b, alpha);
            glm::vec3 tip = palm.getTipPosition();
            glm::vec3 root = palm.getPosition();

            //Scale the positions based on avatar scale
            myAvatar->scaleVectorRelativeToPosition(tip);
            myAvatar->scaleVectorRelativeToPosition(root);

            Avatar::renderJointConnectingCone(root, tip, PALM_FINGER_ROD_RADIUS, PALM_FINGER_ROD_RADIUS);
            //  Render sphere at palm/finger root
            glm::vec3 offsetFromPalm = root + palm.getNormal() * PALM_DISK_THICKNESS;
            Avatar::renderJointConnectingCone(root, offsetFromPalm, PALM_DISK_RADIUS, 0.0f);
            glPushMatrix();
            glTranslatef(root.x, root.y, root.z);
            DependencyManager::get<GeometryCache>()->renderSphere(PALM_BALL_RADIUS, 20.0f, 20.0f);
            glPopMatrix();
        }
    }

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
}

