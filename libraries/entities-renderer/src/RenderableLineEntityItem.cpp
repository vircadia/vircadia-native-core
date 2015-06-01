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
#include <GeometryCache.h>

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
    glm::vec3 dimensions = getDimensions();
    glm::quat rotation = getRotation();
    glm::vec4 lineColor(toGlm(getXColor()), getLocalRenderAlpha());
    glPushMatrix();
        glLineWidth(getLineWidth());
        auto geometryCache = DependencyManager::get<GeometryCache>();
        if(_lineVerticesID == GeometryCache::UNKNOWN_ID){
            _lineVerticesID = geometryCache ->allocateID();
        }
         //TODO: Figure out clean , efficient way to do relative line positioning. For now we'll just use absolute positioning.
        //glTranslatef(position.x, position.y, position.z);
        glm::vec3 axis = glm::axis(rotation);
        glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
        QVector<glm::vec3> points;
        if(_pointsChanged) {
          geometryCache->updateVertices(_lineVerticesID, getLinePoints(), lineColor);
            _pointsChanged = false;
        }
        geometryCache->renderVertices(gpu::LINE_STRIP, _lineVerticesID);
    glPopMatrix();
    RenderableDebugableEntityItem::render(this, args);
};
