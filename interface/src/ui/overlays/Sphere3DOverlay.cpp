//
//  Sphere3DOverlay.cpp
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

#include "Sphere3DOverlay.h"
#include "renderer/GlowEffect.h"

Sphere3DOverlay::Sphere3DOverlay() {
}

Sphere3DOverlay::~Sphere3DOverlay() {
}

void Sphere3DOverlay::render() {
    if (!_visible) {
        return; // do nothing if we're not visible
    }

    const int slices = 15;
    float alpha = getAlpha();
    xColor color = getColor();
    const float MAX_COLOR = 255;
    glColor4f(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);


    glDisable(GL_LIGHTING);
    
    glm::vec3 position = getPosition();
    glm::vec3 center = getCenter();
    glm::vec3 dimensions = getDimensions();
    //glm::vec3 halfDimensions = dimensions / 2.0f;
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
            glScalef(dimensions.x, dimensions.y, dimensions.z);
            //Application::getInstance()->getDeferredLightingEffect()->renderSolidCube(1.0f);
            if (_isSolid) {
                glutSolidSphere(1.0f, slices, slices);
            } else {
                glutWireSphere(1.0f, slices, slices);
            }
        glPopMatrix();
    glPopMatrix();
    
    if (glower) {
        delete glower;
    }

}
