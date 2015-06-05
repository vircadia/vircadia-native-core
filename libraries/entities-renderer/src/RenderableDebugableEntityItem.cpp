//
//  RenderableDebugableEntityItem.cpp
//  libraries/entities-renderer/src/
//
//  Created by Seth Alves on 5/1/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//



#include <glm/gtx/quaternion.hpp>
#include <gpu/GPUConfig.h>
#include <DeferredLightingEffect.h>
#include <PhysicsEngine.h>

#include "RenderableDebugableEntityItem.h"


void RenderableDebugableEntityItem::renderBoundingBox(EntityItem* entity, RenderArgs* args,
                                                      float puffedOut, glm::vec4& color) {
    glm::vec3 position = entity->getPosition();
    glm::vec3 center = entity->getCenter();
    glm::vec3 dimensions = entity->getDimensions();
    glm::quat rotation = entity->getRotation();

    glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glm::vec3 axis = glm::axis(rotation);
        glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
        glPushMatrix();
            glm::vec3 positionToCenter = center - position;
            glTranslatef(positionToCenter.x, positionToCenter.y, positionToCenter.z);
            glScalef(dimensions.x, dimensions.y, dimensions.z);
            DependencyManager::get<DeferredLightingEffect>()->renderWireCube(1.0f + puffedOut, color);
        glPopMatrix();
    glPopMatrix();
}

void RenderableDebugableEntityItem::renderHoverDot(EntityItem* entity, RenderArgs* args) {
    glm::vec3 position = entity->getPosition();
    glm::vec3 center = entity->getCenter();
    glm::vec3 dimensions = entity->getDimensions();
    glm::quat rotation = entity->getRotation();
    glm::vec4 blueColor(0.0f, 0.0f, 1.0f, 1.0f);
    float radius = 0.05f;

    glPushMatrix();
        glTranslatef(position.x, position.y + dimensions.y + radius, position.z);
        glm::vec3 axis = glm::axis(rotation);
        glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);

        glPushMatrix();
            glm::vec3 positionToCenter = center - position;
            glTranslatef(positionToCenter.x, positionToCenter.y, positionToCenter.z);

            glScalef(radius, radius, radius);

            const int SLICES = 8;
            const int STACKS = 8;
            DependencyManager::get<DeferredLightingEffect>()->renderSolidSphere(0.5f, SLICES, STACKS, blueColor);
        glPopMatrix();
    glPopMatrix();
}

void RenderableDebugableEntityItem::render(EntityItem* entity, RenderArgs* args) {
    bool debugSimulationOwnership = args->_debugFlags & RenderArgs::RENDER_DEBUG_SIMULATION_OWNERSHIP;

    if (debugSimulationOwnership) {
        quint64 now = usecTimestampNow();
        if (now - entity->getLastEditedFromRemote() < 0.1f * USECS_PER_SECOND) {
            glm::vec4 redColor(1.0f, 0.0f, 0.0f, 1.0f);
            renderBoundingBox(entity, args, 0.2f, redColor);
        }

        if (now - entity->getLastBroadcast() < 0.2f * USECS_PER_SECOND) {
            glm::vec4 yellowColor(1.0f, 1.0f, 0.2f, 1.0f);
            renderBoundingBox(entity, args, 0.3f, yellowColor);
        }

        ObjectMotionState* motionState = static_cast<ObjectMotionState*>(entity->getPhysicsInfo());
        if (motionState && motionState->isActive()) {
            renderHoverDot(entity, args);
        }
    }
}
