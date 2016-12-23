//
//  RenderShadowTask.cpp
//  render-utils/src/
//
//  Created by Zach Pomerantz on 1/7/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderShadowTask.h"

#include <gpu/Context.h>

#include <ViewFrustum.h>

#include <render/Context.h>
#include <render/CullTask.h>
#include <render/SortTask.h>
#include <render/DrawTask.h>

#include "DeferredLightingEffect.h"
#include "FramebufferCache.h"

#include "model_shadow_vert.h"
#include "skin_model_shadow_vert.h"

#include "model_shadow_frag.h"
#include "skin_model_shadow_frag.h"

using namespace render;

void RenderShadowMap::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext,
                          const render::ShapeBounds& inShapes) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    auto lightStage = DependencyManager::get<DeferredLightingEffect>()->getLightStage();

    LightStage::Index globalLightIndex { 0 };

    const auto globalLight = lightStage->getLight(globalLightIndex);
    const auto shadow = lightStage->getShadow(globalLightIndex);
    if (!shadow) return;

    const auto& fbo = shadow->framebuffer;

    RenderArgs* args = renderContext->args;
    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;

        glm::ivec4 viewport{0, 0, fbo->getWidth(), fbo->getHeight()};
        batch.setViewportTransform(viewport);
        batch.setStateScissorRect(viewport);

        batch.setFramebuffer(fbo);
        batch.clearFramebuffer(
            gpu::Framebuffer::BUFFER_COLOR0 | gpu::Framebuffer::BUFFER_DEPTH,
            vec4(vec3(1.0, 1.0, 1.0), 0.0), 1.0, 0, true);

        batch.setProjectionTransform(shadow->getProjection());
        batch.setViewTransform(shadow->getView(), false);

        auto shadowPipeline = _shapePlumber->pickPipeline(args, ShapeKey());
        auto shadowSkinnedPipeline = _shapePlumber->pickPipeline(args, ShapeKey::Builder().withSkinned());

        std::vector<ShapeKey> skinnedShapeKeys{};

        // Iterate through all inShapes and render the unskinned
        args->_pipeline = shadowPipeline;
        batch.setPipeline(shadowPipeline->pipeline);
        for (auto items : inShapes) {
            if (items.first.isSkinned()) {
                skinnedShapeKeys.push_back(items.first);
            } else {
                renderItems(sceneContext, renderContext, items.second);
            }
        }

        // Reiterate to render the skinned
        args->_pipeline = shadowSkinnedPipeline;
        batch.setPipeline(shadowSkinnedPipeline->pipeline);
        for (const auto& key : skinnedShapeKeys) {
            renderItems(sceneContext, renderContext, inShapes.at(key));
        }

        args->_pipeline = nullptr;
        args->_batch = nullptr;
    });
}

RenderShadowTask::RenderShadowTask(CullFunctor cullFunctor) {
    cullFunctor = cullFunctor ? cullFunctor : [](const RenderArgs*, const AABox&){ return true; };

    // Prepare the ShapePipeline
    ShapePlumberPointer shapePlumber = std::make_shared<ShapePlumber>();
    {
        auto state = std::make_shared<gpu::State>();
        state->setCullMode(gpu::State::CULL_BACK);
        state->setDepthTest(true, true, gpu::LESS_EQUAL);

        auto modelVertex = gpu::Shader::createVertex(std::string(model_shadow_vert));
        auto modelPixel = gpu::Shader::createPixel(std::string(model_shadow_frag));
        gpu::ShaderPointer modelProgram = gpu::Shader::createProgram(modelVertex, modelPixel);
        shapePlumber->addPipeline(
            ShapeKey::Filter::Builder().withoutSkinned(),
            modelProgram, state);

        auto skinVertex = gpu::Shader::createVertex(std::string(skin_model_shadow_vert));
        auto skinPixel = gpu::Shader::createPixel(std::string(skin_model_shadow_frag));
        gpu::ShaderPointer skinProgram = gpu::Shader::createProgram(skinVertex, skinPixel);
        shapePlumber->addPipeline(
            ShapeKey::Filter::Builder().withSkinned(),
            skinProgram, state);
    }

    const auto cachedMode = addJob<RenderShadowSetup>("Setup");

    // CPU jobs:
    // Fetch and cull the items from the scene
    auto shadowFilter = ItemFilter::Builder::visibleWorldItems().withTypeShape().withOpaque().withoutLayered();
    const auto shadowSelection = addJob<FetchSpatialTree>("FetchShadowSelection", shadowFilter);
    const auto culledShadowSelection = addJob<CullSpatialSelection>("CullShadowSelection", shadowSelection, cullFunctor, RenderDetails::SHADOW, shadowFilter);

    // Sort
    const auto sortedPipelines = addJob<PipelineSortShapes>("PipelineSortShadowSort", culledShadowSelection);
    const auto sortedShapes = addJob<DepthSortShapes>("DepthSortShadowMap", sortedPipelines);

    // GPU jobs: Render to shadow map
    addJob<RenderShadowMap>("RenderShadowMap", sortedShapes, shapePlumber);

    addJob<RenderShadowTeardown>("Teardown", cachedMode);
}

void RenderShadowTask::configure(const Config& configuration) {
    DependencyManager::get<DeferredLightingEffect>()->setShadowMapEnabled(configuration.enabled);
    // This is a task, so must still propogate configure() to its Jobs
    Task::configure(configuration);
}

void RenderShadowTask::run(const SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext) {
    assert(sceneContext);
    RenderArgs* args = renderContext->args;

    // sanity checks
    if (!sceneContext->_scene || !args) {
        return;
    }

    for (auto job : _jobs) {
        job.run(sceneContext, renderContext);
    }
}

void RenderShadowSetup::run(const SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, Output& output) {
    auto lightStage = DependencyManager::get<DeferredLightingEffect>()->getLightStage();
    const auto globalShadow = lightStage->getShadow(0);

    // Cache old render args
    RenderArgs* args = renderContext->args;
    output = args->_renderMode;

    auto nearClip = args->getViewFrustum().getNearClip();
    float nearDepth = -args->_boomOffset.z;
    const int SHADOW_FAR_DEPTH = 20;
    globalShadow->setKeylightFrustum(args->getViewFrustum(), nearDepth, nearClip + SHADOW_FAR_DEPTH);

    // Set the keylight render args
    args->pushViewFrustum(*(globalShadow->getFrustum()));
    args->_renderMode = RenderArgs::SHADOW_RENDER_MODE;
}

void RenderShadowTeardown::run(const SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const Input& input) {
    RenderArgs* args = renderContext->args;

    // Reset the render args
    args->popViewFrustum();
    args->_renderMode = input;
};
