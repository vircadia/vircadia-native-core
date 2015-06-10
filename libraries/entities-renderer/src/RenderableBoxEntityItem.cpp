//
//  RenderableBoxEntityItem.cpp
//  libraries/entities-renderer/src/
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/quaternion.hpp>

#include <gpu/GPUConfig.h>
#include <gpu/Batch.h>

#include <DeferredLightingEffect.h>
#include <ObjectMotionState.h>
#include <PerfStat.h>

#include "RenderableBoxEntityItem.h"

EntityItemPointer RenderableBoxEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return EntityItemPointer(new RenderableBoxEntityItem(entityID, properties));
}

void RenderableBoxEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableBoxEntityItem::render");
    Q_ASSERT(getType() == EntityTypes::Box);
    glm::vec4 cubeColor(toGlm(getXColor()), getLocalRenderAlpha());
    
    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;
    batch.setModelTransform(getTransformToCenter());
    DependencyManager::get<DeferredLightingEffect>()->renderSolidCube(batch, 1.0f, cubeColor);

    // TODO: use RenderableDebugableEntityItem::render (instead of the hack below) 
    // when we fix the scaling bug that breaks RenderableDebugableEntityItem for boxes.
    if (args->_debugFlags & RenderArgs::RENDER_DEBUG_SIMULATION_OWNERSHIP) {
        Q_ASSERT(args->_batch);
        gpu::Batch& batch = *args->_batch;
        Transform transform = getTransformToCenter();
        //transform.postScale(entity->getDimensions()); // HACK: this line breaks for BoxEntityItem
        batch.setModelTransform(transform);

        auto nodeList = DependencyManager::get<NodeList>();
        const QUuid& myNodeID = nodeList->getSessionUUID();
        bool highlightSimulationOwnership = (getSimulatorID() == myNodeID);
        if (highlightSimulationOwnership) {
            glm::vec4 greenColor(0.0f, 1.0f, 0.2f, 1.0f);
            DependencyManager::get<DeferredLightingEffect>()->renderWireCube(batch, 1.08f, greenColor);
        }

        quint64 now = usecTimestampNow();
        if (now - getLastEditedFromRemote() < 0.1f * USECS_PER_SECOND) {
            glm::vec4 redColor(1.0f, 0.0f, 0.0f, 1.0f);
            DependencyManager::get<DeferredLightingEffect>()->renderWireCube(batch, 1.16f, redColor);
        }

        if (now - getLastBroadcast() < 0.2f * USECS_PER_SECOND) {
            glm::vec4 yellowColor(1.0f, 1.0f, 0.2f, 1.0f);
            DependencyManager::get<DeferredLightingEffect>()->renderWireCube(batch, 1.24f, yellowColor);
        }

        ObjectMotionState* motionState = static_cast<ObjectMotionState*>(getPhysicsInfo());
        if (motionState && motionState->isActive()) {
            glm::vec4 blueColor(0.0f, 0.0f, 1.0f, 1.0f);
            DependencyManager::get<DeferredLightingEffect>()->renderWireCube(batch, 1.32f, blueColor);
        }
    }
    //RenderableDebugableEntityItem::render(this, args); // TODO: use this instead of the hack above
};
