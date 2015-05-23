//
//  RenderableLineEntityItem.cpp
//  libraries/entities-renderer/src/
//
//  Created by Seth Alves on 5/11/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/quaternion.hpp>

#include <gpu/GPUConfig.h>

#include <DeferredLightingEffect.h>
#include <PerfStat.h>

#include "RenderableLineEntityItem.h"

EntityItemPointer RenderableLineEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return EntityItemPointer(new RenderableLineEntityItem(entityID, properties));
}

void RenderableLineEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableLineEntityItem::render");
    assert(getType() == EntityTypes::Line);
    glm::vec3 position = getPosition();
    glm::vec3 dimensions = getDimensions();
    glm::quat rotation = getRotation();
    glm::vec4 lineColor(toGlm(getXColor()), getLocalRenderAlpha());
    glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glm::vec3 axis = glm::axis(rotation);
        glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
        glm::vec3 p1 = {0.0f, 0.0f, 0.0f};
        glm::vec3& p2 = dimensions;
        DependencyManager::get<DeferredLightingEffect>()->renderLine(p1, p2, lineColor, lineColor);
    glPopMatrix();
    
    RenderableDebugableEntityItem::render(this, args);
};
