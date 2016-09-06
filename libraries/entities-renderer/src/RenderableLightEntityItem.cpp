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


#include <GLMHelpers.h>
#include <PerfStat.h>

#include "RenderableLightEntityItem.h"

EntityItemPointer RenderableLightEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity{ new RenderableLightEntityItem(entityID) };
    entity->setProperties(properties);
    return entity;
}

RenderableLightEntityItem::RenderableLightEntityItem(const EntityItemID& entityItemID) : LightEntityItem(entityItemID)
{
}

bool RenderableLightEntityItem::addToScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges) {
    _myItem = scene->allocateID();

    auto renderItem = std::make_shared<LightPayload>();
    updateRenderItemFromEntity(renderItem.get());

    auto renderPayload = std::make_shared<LightPayload::Payload>(renderItem);

    render::Item::Status::Getters statusGetters;
    makeEntityItemStatusGetters(self, statusGetters);
    renderPayload->addStatusGetters(statusGetters);

    pendingChanges.resetItem(_myItem, renderPayload);

    return true;
}

void RenderableLightEntityItem::somethingChangedNotification() {
    if (_lightPropertiesChanged) {
        notifyChanged();
    }
    LightEntityItem::somethingChangedNotification();
}

void RenderableLightEntityItem::removeFromScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges) {
    pendingChanges.removeItem(_myItem);
    render::Item::clearID(_myItem);
}

void RenderableLightEntityItem::locationChanged(bool tellPhysics) {
    EntityItem::locationChanged(tellPhysics);
    notifyChanged();
}

void RenderableLightEntityItem::dimensionsChanged() {
    EntityItem::dimensionsChanged();
    notifyChanged();
}

void RenderableLightEntityItem::checkFading() {
    bool transparent = isTransparent();
    if (transparent != _prevIsTransparent) {
        notifyChanged();
        _isFading = false;
        _prevIsTransparent = transparent;
    }
}

void RenderableLightEntityItem::notifyChanged() {

    if (!render::Item::isValidID(_myItem)) {
        return;
    }

    render::PendingChanges pendingChanges;
    render::ScenePointer scene = AbstractViewStateInterface::instance()->getMain3DScene();

    updateLightFromEntity(pendingChanges);

    scene->enqueuePendingChanges(pendingChanges);
}

/*
void RenderableLightEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableLightEntityItem::render");
    assert(getType() == EntityTypes::Light);
    checkFading();



   // DependencyManager::get<DeferredLightingEffect>()->addLight(_light);

#ifdef WANT_DEBUG
    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;
    batch.setModelTransform(getTransformToCenter());
    DependencyManager::get<GeometryCache>()->renderWireSphere(batch, 0.5f, 15, 15, glm::vec4(color, 1.0f));
#endif
};
*/

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


void RenderableLightEntityItem::updateLightFromEntity(render::PendingChanges& pendingChanges) {
    if (!render::Item::isValidID(_myItem)) {
        return;
    }


    pendingChanges.updateItem<LightPayload>(_myItem, [this](LightPayload& data) {
        this->updateRenderItemFromEntity(&data);
    });
}

void RenderableLightEntityItem::updateRenderItemFromEntity(LightPayload* lightPayload) {
    auto entity = this;
    auto light = lightPayload->editLight();
    light->setPosition(entity->getPosition());
    light->setOrientation(entity->getRotation());

    bool success;
    lightPayload->editBound() = entity->getAABox(success);
    if (!success) {
        lightPayload->editBound() = render::Item::Bound();
    }

    glm::vec3 dimensions = entity->getDimensions();
    float largestDiameter = glm::max(dimensions.x, dimensions.y, dimensions.z);
    light->setMaximumRadius(largestDiameter / 2.0f);

    light->setColor(toGlm(entity->getXColor()));

    float intensity = entity->getIntensity();//* entity->getFadingRatio();
    light->setIntensity(intensity);

    light->setFalloffRadius(entity->getFalloffRadius());


    float exponent = entity->getExponent();
    float cutoff = glm::radians(entity->getCutoff());
    if (!entity->getIsSpotlight()) {
        light->setType(model::Light::POINT);
    } else {
        light->setType(model::Light::SPOT);

        light->setSpotAngle(cutoff);
        light->setSpotExponent(exponent);
    }

}




