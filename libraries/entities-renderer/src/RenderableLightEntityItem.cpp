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

#include "RenderableLightEntityItem.h"

#include <GLMHelpers.h>
#include <PerfStat.h>

using namespace render;
using namespace render::entities;

void LightEntityRenderer::doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) {
    // Reset the value before reading the data!
    // otherwise there could be a race condition where the value changes
    // after we read it but before we reset, and we never see the change
    entity->resetLightPropertiesChanged();

    auto& lightPayload = *_lightPayload;

    lightPayload.setVisible(_visible);

    auto light = lightPayload.editLight();
    light->setPosition(entity->getPosition());
    light->setOrientation(entity->getRotation());

    bool success;
    lightPayload.editBound() = entity->getAABox(success);
    if (!success) {
        lightPayload.editBound() = render::Item::Bound();
    }

    glm::vec3 dimensions = entity->getDimensions();
    float largestDiameter = glm::compMax(dimensions);
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

ItemKey LightEntityRenderer::getKey() {
    return payloadGetKey(_lightPayload);
}

Item::Bound LightEntityRenderer::getBound() {
    return payloadGetBound(_lightPayload);
}

bool LightEntityRenderer::needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const {
    return entity->lightPropertiesChanged();
}

void LightEntityRenderer::doRender(RenderArgs* args) {
    _lightPayload->render(args);
}

