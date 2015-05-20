//
//  RenderableLightEntityItem.cpp
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
#include <GLMHelpers.h>
#include <PerfStat.h>

#include "RenderableLightEntityItem.h"

EntityItem* RenderableLightEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return new RenderableLightEntityItem(entityID, properties);
}

void RenderableLightEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableLightEntityItem::render");
    assert(getType() == EntityTypes::Light);
    glm::vec3 position = getPosition();
    glm::vec3 dimensions = getDimensions();
    glm::quat rotation = getRotation();
    float largestDiameter = glm::max(dimensions.x, dimensions.y, dimensions.z);

    const float MAX_COLOR = 255.0f;
    float colorR = getColor()[RED_INDEX] / MAX_COLOR;
    float colorG = getColor()[GREEN_INDEX] / MAX_COLOR;
    float colorB = getColor()[BLUE_INDEX] / MAX_COLOR;

    glm::vec3 color = glm::vec3(colorR, colorG, colorB);

    float intensity = getIntensity();
    float exponent = getExponent();
    float cutoff = glm::radians(getCutoff());

    if (_isSpotlight) {
        DependencyManager::get<DeferredLightingEffect>()->addSpotLight(position, largestDiameter / 2.0f,
            color, intensity, rotation, exponent, cutoff);
    } else {
        DependencyManager::get<DeferredLightingEffect>()->addPointLight(position, largestDiameter / 2.0f,
            color, intensity);
    }

#ifdef WANT_DEBUG
    glm::vec4 color(diffuseR, diffuseG, diffuseB, 1.0f);
    glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glm::vec3 axis = glm::axis(rotation);
        glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
        glPushMatrix();
            glm::vec3 positionToCenter = center - position;
            glTranslatef(positionToCenter.x, positionToCenter.y, positionToCenter.z);

            glScalef(dimensions.x, dimensions.y, dimensions.z);
            DependencyManager::get<DeferredLightingEffect>()->renderWireSphere(0.5f, 15, 15, color);
        glPopMatrix();
    glPopMatrix();
#endif
};

bool RenderableLightEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face, 
                         void** intersectedObject, bool precisionPicking) const {

    // TODO: consider if this is really what we want to do. We've made it so that "lights are pickable" is a global state
    // this is probably reasonable since there's typically only one tree you'd be picking on at a time. Technically we could 
    // be on the clipboard and someone might be trying to use the ray intersection API there. Anyway... if you ever try to 
    // do ray intersection testing off of trees other than the main tree of the main entity renderer, then we'll need to 
    // fix this mechanism.
    return _lightsArePickable;
}
