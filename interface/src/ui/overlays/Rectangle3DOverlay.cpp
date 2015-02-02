//
//  Rectangle3DOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>

#include <GeometryCache.h>
#include <GlowEffect.h>
#include <SharedUtil.h>

#include "Rectangle3DOverlay.h"

Rectangle3DOverlay::Rectangle3DOverlay() {
}

Rectangle3DOverlay::Rectangle3DOverlay(const Rectangle3DOverlay* rectangle3DOverlay) :
    Planar3DOverlay(rectangle3DOverlay),
    _geometryCacheID(DependencyManager::get<GeometryCache>()->allocateID())
{
}

Rectangle3DOverlay::~Rectangle3DOverlay() {
}

void Rectangle3DOverlay::render(RenderArgs* args) {
    if (!_visible) {
        return; // do nothing if we're not visible
    }
    
    float alpha = getAlpha();
    xColor color = getColor();
    const float MAX_COLOR = 255.0f;
    glm::vec4 rectangleColor(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);

    glDisable(GL_LIGHTING);
    
    glm::vec3 position = getPosition();
    glm::vec3 center = getCenter();
    glm::vec2 dimensions = getDimensions();
    glm::vec2 halfDimensions = dimensions * 0.5f;
    glm::quat rotation = getRotation();

    float glowLevel = getGlowLevel();
    Glower* glower = NULL;
    if (glowLevel > 0.0f) {
        glower = new Glower(glowLevel);
    }

    glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glm::vec3 axis = glm::axis(rotation);
        glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
        glPushMatrix();
            glm::vec3 positionToCenter = center - position;
            glTranslatef(positionToCenter.x, positionToCenter.y, positionToCenter.z);
            //glScalef(dimensions.x, dimensions.y, 1.0f);

            glLineWidth(_lineWidth);

            auto geometryCache = DependencyManager::get<GeometryCache>();
            
            // for our overlay, is solid means we draw a solid "filled" rectangle otherwise we just draw a border line...
            if (getIsSolid()) {
                glm::vec3 topLeft(-halfDimensions.x, 0.0f, -halfDimensions.y);
                glm::vec3 bottomRight(halfDimensions.x, 0.0f, halfDimensions.y);
                DependencyManager::get<GeometryCache>()->renderQuad(topLeft, bottomRight, rectangleColor);
            } else {
                if (getIsDashedLine()) {

                    glm::vec3 point1(-halfDimensions.x, 0.0f, -halfDimensions.y);
                    glm::vec3 point2(halfDimensions.x, 0.0f, -halfDimensions.y);
                    glm::vec3 point3(halfDimensions.x, 0.0f, halfDimensions.y);
                    glm::vec3 point4(-halfDimensions.x, 0.0f, halfDimensions.y);
                
                    geometryCache->renderDashedLine(point1, point2, rectangleColor);
                    geometryCache->renderDashedLine(point2, point3, rectangleColor);
                    geometryCache->renderDashedLine(point3, point4, rectangleColor);
                    geometryCache->renderDashedLine(point4, point1, rectangleColor);

                } else {
                
                    if (halfDimensions != _previousHalfDimensions) {
                        QVector<glm::vec3> border;
                        border << glm::vec3(-halfDimensions.x, 0.0f, -halfDimensions.y);
                        border << glm::vec3(halfDimensions.x, 0.0f, -halfDimensions.y);
                        border << glm::vec3(halfDimensions.x, 0.0f, halfDimensions.y);
                        border << glm::vec3(-halfDimensions.x, 0.0f, halfDimensions.y);
                        border << glm::vec3(-halfDimensions.x, 0.0f, -halfDimensions.y);
                        geometryCache->updateVertices(_geometryCacheID, border, rectangleColor);

                        _previousHalfDimensions = halfDimensions;
                        
                    }
                    geometryCache->renderVertices(gpu::LINE_STRIP, _geometryCacheID);
                }
            }
 
        glPopMatrix();
    glPopMatrix();
    
    if (glower) {
        delete glower;
    }
}

void Rectangle3DOverlay::setProperties(const QScriptValue &properties) {
    Planar3DOverlay::setProperties(properties);
}

Rectangle3DOverlay* Rectangle3DOverlay::createClone() const {
    return new Rectangle3DOverlay(this);
}
