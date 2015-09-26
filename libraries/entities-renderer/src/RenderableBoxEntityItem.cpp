//
//  RenderableBoxEntityItem.cpp
//  libraries/entities-renderer/src/
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderableBoxEntityItem.h"

#include <glm/gtx/quaternion.hpp>

#include <gpu/Batch.h>

#include <DeferredLightingEffect.h>
#include <GeometryCache.h>
#include <ObjectMotionState.h>
#include <PerfStat.h>

#include "RenderableDebugableEntityItem.h"
#include "../render-utils/simple_vert.h"
#include "../render-utils/simple_frag.h"

EntityItemPointer RenderableBoxEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return std::make_shared<RenderableBoxEntityItem>(entityID, properties);
}

void RenderableBoxEntityItem::setUserData(const QString& value) {
    if (value != getUserData()) {
        BoxEntityItem::setUserData(value);
        _procedural.reset();
    }
}

void RenderableBoxEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableBoxEntityItem::render");
    Q_ASSERT(getType() == EntityTypes::Box);
    Q_ASSERT(args->_batch);

    if (!_procedural) {
        _procedural.reset(new Procedural(this->getUserData()));
        _procedural->_vertexSource = simple_vert;
        _procedural->_fragmentSource = simple_frag;
        _procedural->_state->setCullMode(gpu::State::CULL_NONE);
        _procedural->_state->setDepthTest(true, true, gpu::LESS_EQUAL);
        _procedural->_state->setBlendFunction(false,
            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
    }

    gpu::Batch& batch = *args->_batch;
    glm::vec4 cubeColor(toGlm(getXColor()), getLocalRenderAlpha());

    if (_procedural->ready()) {
        batch.setModelTransform(getTransformToCenter()); // we want to include the scale as well
        _procedural->prepare(batch, this->getDimensions());
        auto color = _procedural->getColor(cubeColor);
        batch._glColor4f(color.r, color.g, color.b, color.a);
        DependencyManager::get<GeometryCache>()->renderCube(batch);
    } else {
        DependencyManager::get<DeferredLightingEffect>()->renderSolidCubeInstance(batch, getTransformToCenter(), cubeColor);
    }

    RenderableDebugableEntityItem::render(this, args);
};
