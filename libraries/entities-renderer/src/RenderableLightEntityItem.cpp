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

RenderableLightEntityItem::RenderableLightEntityItem(const EntityItemID& entityItemID) : LightEntityItem(entityItemID),
    _light(std::make_shared<model::Light>()),
    _lightStamp(-1)
{
}

namespace render {
    template <> const ItemKey payloadGetKey(const LightPayload::Pointer& payload) {
        return ItemKey::Builder::light();
    }

    template <> const Item::Bound payloadGetBound(const LightPayload::Pointer& payload) {
        if (payload) {
            return payload->_bound;
        }
        return render::Item::Bound();
    }
    template <> void payloadRender(const LightPayload::Pointer& payload, RenderArgs* args) {
        if (args) {
            if (payload) {
                payload->render(args);
            }
        }
    }
}

void RenderableLightEntityItem::updateLightFromEntity() {
    _light->setPosition(getPosition());
    _light->setOrientation(getRotation());

    glm::vec3 dimensions = getDimensions();
    float largestDiameter = glm::max(dimensions.x, dimensions.y, dimensions.z);
    _light->setMaximumRadius(largestDiameter / 2.0f);

    _light->setColor(toGlm(getXColor()));

    float intensity = getIntensity() * (_isFading ? Interpolate::calculateFadeRatio(_fadeStartTime) : 1.0f);
    _light->setIntensity(intensity);

    _light->setFalloffRadius(getFalloffRadius());


    float exponent = getExponent();
    float cutoff = glm::radians(getCutoff());
    if (!_isSpotlight) {
        _light->setType(model::Light::POINT);
    } else {
        _light->setType(model::Light::SPOT);

        _light->setSpotAngle(cutoff);
        _light->setSpotExponent(exponent);
    }
}

void RenderableLightEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableLightEntityItem::render");
    assert(getType() == EntityTypes::Light);
    checkFading();


    updateLightFromEntity();

    DependencyManager::get<DeferredLightingEffect>()->addLight(_light);

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
