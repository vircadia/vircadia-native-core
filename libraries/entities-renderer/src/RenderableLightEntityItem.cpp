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

#include <gpu/Batch.h>

#include <DeferredLightingEffect.h>
#include <GLMHelpers.h>
#include <PerfStat.h>

#include "RenderableLightEntityItem.h"

EntityItemPointer RenderableLightEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity{ new RenderableLightEntityItem(entityID) };
    entity->setProperties(properties);
    return entity;
}

void RenderableLightEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableLightEntityItem::render");
    assert(getType() == EntityTypes::Light);
    checkFading();

    glm::vec3 position = getPosition();
    glm::vec3 dimensions = getDimensions();
    glm::quat rotation = getRotation();
    float largestDiameter = glm::compMax(dimensions);

    glm::vec3 color = toGlm(getXColor());

    float intensity = getIntensity() * (_isFading ? Interpolate::calculateFadeRatio(_fadeStartTime) : 1.0f);
    float falloffRadius = getFalloffRadius();
    float exponent = getExponent();
    float cutoff = glm::radians(getCutoff());

    if (_isSpotlight) {
        DependencyManager::get<DeferredLightingEffect>()->addSpotLight(position, largestDiameter / 2.0f,
            color, intensity, falloffRadius, rotation, exponent, cutoff);
    } else {
        DependencyManager::get<DeferredLightingEffect>()->addPointLight(position, largestDiameter / 2.0f,
            color, intensity, falloffRadius);
    }
    
#ifdef WANT_DEBUG
    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;
    batch.setModelTransform(getTransformToCenter());
    DependencyManager::get<GeometryCache>()->renderWireSphere(batch, 0.5f, 15, 15, glm::vec4(color, 1.0f));
#endif
};

bool RenderableLightEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                        bool& keepSearching, OctreeElementPointer& element, float& distance, 
                        BoxFace& face, glm::vec3& surfaceNormal,
                        void** intersectedObject, bool precisionPicking) const {

    // TODO: consider if this is really what we want to do. We've made it so that "lights are pickable" is a global state
    // this is probably reasonable since there's typically only one tree you'd be picking on at a time. Technically we could 
    // be on the clipboard and someone might be trying to use the ray intersection API there. Anyway... if you ever try to 
    // do ray intersection testing off of trees other than the main tree of the main entity renderer, then we'll need to 
    // fix this mechanism.
    return _lightsArePickable;
}
