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


#include "RenderableDebugableEntityItem.h"

#include <glm/gtx/quaternion.hpp>

#include <gpu/Batch.h>
#include <DeferredLightingEffect.h>
#include <ObjectMotionState.h>


void RenderableDebugableEntityItem::renderBoundingBox(EntityItem* entity, RenderArgs* args,
                                                      float puffedOut, glm::vec4& color) {
    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;
    batch.setModelTransform(entity->getTransformToCenter()); // we want to include the scale as well
    DependencyManager::get<DeferredLightingEffect>()->renderWireCube(batch, 1.0f + puffedOut, color);
}

void RenderableDebugableEntityItem::render(EntityItem* entity, RenderArgs* args) {
    if (args->_debugFlags & RenderArgs::RENDER_DEBUG_SIMULATION_OWNERSHIP) {
        Q_ASSERT(args->_batch);
        gpu::Batch& batch = *args->_batch;

        batch.setModelTransform(entity->getTransformToCenter()); // we want to include the scale as well
        
        auto nodeList = DependencyManager::get<NodeList>();
        const QUuid& myNodeID = nodeList->getSessionUUID();
        bool highlightSimulationOwnership = (entity->getSimulatorID() == myNodeID);
        if (highlightSimulationOwnership) {
            glm::vec4 greenColor(0.0f, 1.0f, 0.2f, 1.0f);
            renderBoundingBox(entity, args, 0.08f, greenColor);
        }

        quint64 now = usecTimestampNow();
        if (now - entity->getLastEditedFromRemote() < 0.1f * USECS_PER_SECOND) {
            glm::vec4 redColor(1.0f, 0.0f, 0.0f, 1.0f);
            renderBoundingBox(entity, args, 0.16f, redColor);
        }

        if (now - entity->getLastBroadcast() < 0.2f * USECS_PER_SECOND) {
            glm::vec4 yellowColor(1.0f, 1.0f, 0.2f, 1.0f);
            renderBoundingBox(entity, args, 0.24f, yellowColor);
        }

        ObjectMotionState* motionState = static_cast<ObjectMotionState*>(entity->getPhysicsInfo());
        if (motionState && motionState->isActive()) {
            glm::vec4 blueColor(0.0f, 0.0f, 1.0f, 1.0f);
            renderBoundingBox(entity, args, 0.32f, blueColor);
        }
    }
}
