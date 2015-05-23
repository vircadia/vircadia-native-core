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
#include <gpu/Batch.h>

#include <DeferredLightingEffect.h>
#include <PhysicsEngine.h>

#include "RenderableDebugableEntityItem.h"


void RenderableDebugableEntityItem::renderBoundingBox(EntityItem* entity, RenderArgs* args,
                                                      float puffedOut, glm::vec4& color) {
    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;
    batch.setModelTransform(entity->getTransformToCenter());
    DependencyManager::get<DeferredLightingEffect>()->renderWireCube(batch, 1.0f + puffedOut, color);
}

void RenderableDebugableEntityItem::renderHoverDot(EntityItem* entity, RenderArgs* args) {
    const int SLICES = 8, STACKS = 8;
    float radius = 0.05f;
    glm::vec4 blueColor(0.0f, 0.0f, 1.0f, 1.0f);
    
    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;
    Transform transform = entity->getTransformToCenter();
    // Cancel true dimensions and set scale to 2 * radius (diameter)
    transform.postScale(2.0f * glm::vec3(radius, radius, radius) / entity->getDimensions());
    batch.setModelTransform(transform);
    DependencyManager::get<DeferredLightingEffect>()->renderSolidSphere(batch, 0.5f, SLICES, STACKS, blueColor);
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

        if (PhysicsEngine::physicsInfoIsActive(entity->getPhysicsInfo())) {
            renderHoverDot(entity, args);
        }
    }
}
