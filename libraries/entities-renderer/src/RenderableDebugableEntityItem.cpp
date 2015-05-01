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

#include "RenderableDebugableEntityItem.h"


void RenderableDebugableEntityItem::renderBoundingBox(EntityItem* entity, RenderArgs* args, bool puffedOut) {
    glm::vec3 position = entity->getPosition();
    glm::vec3 center = entity->getCenter();
    glm::vec3 dimensions = entity->getDimensions();
    glm::quat rotation = entity->getRotation();
    glm::vec4 greenColor(0.0f, 1.0f, 0.0f, 1.0f);

    glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glm::vec3 axis = glm::axis(rotation);
        glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
        glPushMatrix();
            glm::vec3 positionToCenter = center - position;
            glTranslatef(positionToCenter.x, positionToCenter.y, positionToCenter.z);
            glScalef(dimensions.x, dimensions.y, dimensions.z);
            if (puffedOut) {
                DependencyManager::get<DeferredLightingEffect>()->renderWireCube(1.2f, greenColor);
            } else {
                DependencyManager::get<DeferredLightingEffect>()->renderWireCube(1.0f, greenColor);
            }
        glPopMatrix();
    glPopMatrix();
}


void RenderableDebugableEntityItem::render(EntityItem* entity, RenderArgs* args) {
    bool debugSimulationOwnership = args->_debugFlags & RenderArgs::RENDER_DEBUG_SIMULATION_OWNERSHIP;

    if (debugSimulationOwnership) {
        quint64 now = usecTimestampNow();
        if (now - entity->getLastEditedFromRemote() < 0.1f * USECS_PER_SECOND) {
            renderBoundingBox(entity, args, true);
        }
    }
}
