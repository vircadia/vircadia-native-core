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
#include <PerfStat.h>

#include "RenderableBoxEntityItem.h"

EntityItem* RenderableBoxEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return new RenderableBoxEntityItem(entityID, properties);
}

void RenderableBoxEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableBoxEntityItem::render");
    Q_ASSERT(getType() == EntityTypes::Box);
    glm::vec4 cubeColor(toGlm(getXColor()), getLocalRenderAlpha());

    bool debugSimulationOwnership = args->_debugFlags & RenderArgs::RENDER_DEBUG_SIMULATION_OWNERSHIP;
    bool highlightSimulationOwnership = false;
    if (debugSimulationOwnership) {
        auto nodeList = DependencyManager::get<NodeList>();
        const QUuid& myNodeID = nodeList->getSessionUUID();
        highlightSimulationOwnership = (getSimulatorID() == myNodeID);
    }
    
    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;
    batch.setModelTransform(getTransformToCenter());
    if (highlightSimulationOwnership) {
        DependencyManager::get<DeferredLightingEffect>()->renderWireCube(batch, 1.0f, cubeColor);
    } else {
        DependencyManager::get<DeferredLightingEffect>()->renderSolidCube(batch, 1.0f, cubeColor);
    }

    RenderableDebugableEntityItem::render(this, args);
};
