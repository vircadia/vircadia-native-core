//
//  Created by Bradley Austin Davis on 2015/05/12
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderableWebEntityItem.h"

#include <glm/gtx/quaternion.hpp>

#include <gpu/GPUConfig.h>

#include <DeferredLightingEffect.h>
#include <GeometryCache.h>
#include <PerfStat.h>
#include <TextRenderer.h>

#include "GLMHelpers.h"

const int FIXED_FONT_POINT_SIZE = 40;

EntityItem* RenderableWebEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return new RenderableWebEntityItem(entityID, properties);
}

void RenderableWebEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableWebEntityItem::render");
    assert(getType() == EntityTypes::Web);
    glm::vec3 position = getPosition();
    glm::vec3 dimensions = getDimensions();
    glm::vec3 halfDimensions = dimensions / 2.0f;
    glm::quat rotation = getRotation();
    float leftMargin = 0.1f;
    float topMargin = 0.1f;

    //qCDebug(entitytree) << "RenderableWebEntityItem::render() id:" << getEntityItemID() << "text:" << getText();

    glPushMatrix(); 
    {
        glTranslatef(position.x, position.y, position.z);
        glm::vec3 axis = glm::axis(rotation);
        glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);

        float alpha = 1.0f; //getBackgroundAlpha();

        glm::vec3 topLeft(-halfDimensions.x, -halfDimensions.y, 0);
        glm::vec3 bottomRight(halfDimensions.x, halfDimensions.y, 0);
        
        // TODO: Determine if we want these entities to have the deferred lighting effect? I think we do, so that the color
        // used for a sphere, or box have the same look as those used on a text entity.
        DependencyManager::get<DeferredLightingEffect>()->bindSimpleProgram();
        DependencyManager::get<GeometryCache>()->renderQuad(topLeft, bottomRight, glm::vec4(0, 1, 0, alpha));
        DependencyManager::get<DeferredLightingEffect>()->releaseSimpleProgram();
    } 
    glPopMatrix();
}

