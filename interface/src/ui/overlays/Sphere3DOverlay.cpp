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

#include <GlowEffect.h>
#include <SharedUtil.h>

#include "Sphere3DOverlay.h"
#include "Application.h"


Sphere3DOverlay::Sphere3DOverlay(const Sphere3DOverlay* Sphere3DOverlay) :
    Volume3DOverlay(Sphere3DOverlay)
{
}

void Sphere3DOverlay::render(RenderArgs* args) {
    if (!_visible) {
        return; // do nothing if we're not visible
    }

    const int SLICES = 15;
    float alpha = getAlpha();
    xColor color = getColor();
    const float MAX_COLOR = 255.0f;
    glm::vec4 sphereColor(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);

    auto batch = args->_batch;

    if (batch) {
        Transform transform = _transform;
        transform.postScale(getDimensions());
        batch->setModelTransform(transform);
        DependencyManager::get<GeometryCache>()->renderSphere(*batch, 1.0f, SLICES, SLICES, sphereColor, _isSolid);
    } else {
        glDisable(GL_LIGHTING);
        
        glm::vec3 position = getPosition();
        glm::vec3 center = getCenter();
        glm::vec3 dimensions = getDimensions();
        glm::quat rotation = getRotation();

        float glowLevel = getGlowLevel();
        Glower* glower = NULL;
        if (glowLevel > 0.0f) {
            glower = new Glower(glowLevel);
        }

        Transform transform = Transform();
        transform.setTranslation(position);
        glm::vec3 axis = glm::axis(rotation);
        transform.setRotation(glm::angleAxis(glm::angle(rotation), axis));
    
        glm::vec3 positionToCenter = center - position;
        transform.setTranslation(positionToCenter);
        transform.setScale(dimensions);
        DependencyManager::get<GeometryCache>()->renderSphere(*batch, 1.0f, SLICES, SLICES, sphereColor, _isSolid);
  
        
        if (glower) {
            delete glower;
        }
    }

}

Sphere3DOverlay* Sphere3DOverlay::createClone() const {
    return new Sphere3DOverlay(this);
}
