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
#include <SharedUtil.h>

#include "Rectangle3DOverlay.h"
#include "renderer/GlowEffect.h"

Rectangle3DOverlay::Rectangle3DOverlay() {
}

Rectangle3DOverlay::~Rectangle3DOverlay() {
}

void Rectangle3DOverlay::render() {
    if (!_visible) {
        return; // do nothing if we're not visible
    }
    
    float alpha = getAlpha();
    xColor color = getColor();
    const float MAX_COLOR = 255;
    glColor4f(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);

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
            
            // for our overlay, is solid means we draw a solid "filled" rectangle otherwise we just draw a border line...
            if (getIsSolid()) {
                glBegin(GL_QUADS);

                glVertex3f(-halfDimensions.x, 0.0f, -halfDimensions.y);
                glVertex3f(halfDimensions.x, 0.0f, -halfDimensions.y);
                glVertex3f(halfDimensions.x, 0.0f, halfDimensions.y);
                glVertex3f(-halfDimensions.x, 0.0f, halfDimensions.y);

                glEnd();
            } else {
                if (getIsDashedLine()) {
                
                    // TODO: change this to be dashed!
                    glBegin(GL_LINE_STRIP);

                    glVertex3f(-halfDimensions.x, 0.0f, -halfDimensions.y);
                    glVertex3f(halfDimensions.x, 0.0f, -halfDimensions.y);
                    glVertex3f(halfDimensions.x, 0.0f, halfDimensions.y);
                    glVertex3f(-halfDimensions.x, 0.0f, halfDimensions.y);
                    glVertex3f(-halfDimensions.x, 0.0f, -halfDimensions.y);
                
                    glEnd();
                } else {
                    glBegin(GL_LINE_STRIP);

                    glVertex3f(-halfDimensions.x, 0.0f, -halfDimensions.y);
                    glVertex3f(halfDimensions.x, 0.0f, -halfDimensions.y);
                    glVertex3f(halfDimensions.x, 0.0f, halfDimensions.y);
                    glVertex3f(-halfDimensions.x, 0.0f, halfDimensions.y);
                    glVertex3f(-halfDimensions.x, 0.0f, -halfDimensions.y);
                
                    glEnd();
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








