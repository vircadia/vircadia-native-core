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

EntityItem* RenderableLineEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return new RenderableLineEntityItem(entityID, properties);
}

void RenderableLineEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableLineEntityItem::render");
    assert(getType() == EntityTypes::Line);
    glm::vec3 position = getPosition();
    // glm::vec3 center = getCenter();
    glm::vec3 dimensions = getDimensions();
    glm::quat rotation = getRotation();

    const float MAX_COLOR = 255.0f;

    glm::vec4 lineColor(getColor()[RED_INDEX] / MAX_COLOR, getColor()[GREEN_INDEX] / MAX_COLOR,
                        getColor()[BLUE_INDEX] / MAX_COLOR, getLocalRenderAlpha());

    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    glm::vec3 axis = glm::axis(rotation);
    glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
    // glPushMatrix();
    // glm::vec3 positionToCenter = center - position;
    // glTranslatef(positionToCenter.x, positionToCenter.y, positionToCenter.z);
    // glScalef(dimensions.x, dimensions.y, dimensions.z);

    glm::vec3 p1 = {0.0f, 0.0f, 0.0f};
    glm::vec3& p2 = dimensions;

    DependencyManager::get<DeferredLightingEffect>()->renderLine(p1, p2, lineColor, lineColor);

    // glPopMatrix();
    glPopMatrix();

    RenderableDebugableEntityItem::render(this, args);
};
