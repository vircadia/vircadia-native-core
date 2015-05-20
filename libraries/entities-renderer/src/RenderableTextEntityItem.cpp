//
//  RenderableTextEntityItem.cpp
//  interface/src
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/quaternion.hpp>

#include <gpu/GPUConfig.h>

#include <DeferredLightingEffect.h>
#include <GeometryCache.h>
#include <PerfStat.h>
#include <TextRenderer.h>

#include "RenderableTextEntityItem.h"
#include "GLMHelpers.h"

const int FIXED_FONT_POINT_SIZE = 40;

EntityItem* RenderableTextEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return new RenderableTextEntityItem(entityID, properties);
}

void RenderableTextEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableTextEntityItem::render");
    assert(getType() == EntityTypes::Text);
    glm::vec3 position = getPosition();
    glm::vec3 dimensions = getDimensions();
    glm::vec3 halfDimensions = dimensions / 2.0f;
    glm::quat rotation = getRotation();
    float leftMargin = 0.1f;
    float topMargin = 0.1f;

    //qCDebug(entitytree) << "RenderableTextEntityItem::render() id:" << getEntityItemID() << "text:" << getText();

    glPushMatrix(); 
    {
        glTranslatef(position.x, position.y, position.z);
        glm::vec3 axis = glm::axis(rotation);
        glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);

        float alpha = 1.0f; //getBackgroundAlpha();
        static const float SLIGHTLY_BEHIND =  -0.005f;

        glm::vec3 topLeft(-halfDimensions.x, -halfDimensions.y, SLIGHTLY_BEHIND);
        glm::vec3 bottomRight(halfDimensions.x, halfDimensions.y, SLIGHTLY_BEHIND);
        
        // TODO: Determine if we want these entities to have the deferred lighting effect? I think we do, so that the color
        // used for a sphere, or box have the same look as those used on a text entity.
        DependencyManager::get<DeferredLightingEffect>()->bindSimpleProgram();
        DependencyManager::get<GeometryCache>()->renderQuad(topLeft, bottomRight, glm::vec4(toGlm(getBackgroundColorX()), alpha));
        DependencyManager::get<DeferredLightingEffect>()->releaseSimpleProgram();

        TextRenderer* textRenderer = TextRenderer::getInstance(SANS_FONT_FAMILY, FIXED_FONT_POINT_SIZE / 2.0f);

        glTranslatef(-(halfDimensions.x - leftMargin), halfDimensions.y - topMargin, 0.0f);
        glm::vec4 textColor(toGlm(getTextColorX()), alpha);
        // this is a ratio determined through experimentation
        const float scaleFactor = 0.08f * _lineHeight;
        glScalef(scaleFactor, -scaleFactor, scaleFactor);
        glm::vec2 bounds(dimensions.x / scaleFactor, dimensions.y / scaleFactor);
        textRenderer->draw(0, 0, _text, textColor, bounds);
    } 
    glPopMatrix();
}

void RenderableTextEntityItem::enableClipPlane(GLenum plane, float x, float y, float z, float w) {
    GLdouble coefficients[] = { x, y, z, w };
    glClipPlane(plane, coefficients);
    glEnable(plane);
}



