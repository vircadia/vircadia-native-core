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

#include <render/CullTask.h>
#include <render/SortTask.h>
#include <render/DrawTask.h>

#include "DeferredLightingEffect.h"
#include "FramebufferCache.h"

using namespace render;

extern void initZPassPipelines(ShapePlumber& plumber, gpu::StatePointer state);

void RenderShadowMap::run(const render::RenderContextPointer& renderContext,
                          const render::ShapeBounds& inShapes) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    auto lightStage = renderContext->_scene->getStage<LightStage>();
    assert(lightStage);
    LightStage::Index globalLightIndex { 0 };

    const auto globalLight = lightStage->getLight(globalLightIndex);
    const auto shadow = lightStage->getShadow(globalLightIndex);
    if (!shadow) return;

    const auto& fbo = shadow->framebuffer;

    RenderArgs* args = renderContext->args;
    ShapeKey::Builder defaultKeyBuilder;

    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;
        batch.enableStereo(false);

        glm::ivec4 viewport{0, 0, fbo->getWidth(), fbo->getHeight()};
        batch.setViewportTransform(viewport);
        batch.setStateScissorRect(viewport);

        batch.setFramebuffer(fbo);
        batch.clearFramebuffer(
            gpu::Framebuffer::BUFFER_COLOR0 | gpu::Framebuffer::BUFFER_DEPTH,
            vec4(vec3(1.0, 1.0, 1.0), 0.0), 1.0, 0, true);

        batch.setProjectionTransform(shadow->getProjection());
        batch.setViewTransform(shadow->getView(), false);

        auto shadowPipeline = _shapePlumber->pickPipeline(args, defaultKeyBuilder);
        auto shadowSkinnedPipeline = _shapePlumber->pickPipeline(args, defaultKeyBuilder.withSkinned());

        std::vector<ShapeKey> skinnedShapeKeys{};

        // Iterate through all inShapes and render the unskinned
        args->_shapePipeline = shadowPipeline;
        batch.setPipeline(shadowPipeline->pipeline);
        for (auto items : inShapes) {
            if (items.first.isSkinned()) {
                skinnedShapeKeys.push_back(items.first);
            } else {
                renderItems(renderContext, items.second);
            }
        }

        // Reiterate to render the skinned
        args->_shapePipeline = shadowSkinnedPipeline;
        batch.setPipeline(shadowSkinnedPipeline->pipeline);
        for (const auto& key : skinnedShapeKeys) {
            renderItems(renderContext, inShapes.at(key));
        }

        args->_shapePipeline = nullptr;
        args->_batch = nullptr;
    });
}

void RenderShadowTask::build(JobModel& task, const render::Varying& input, render::Varying& output, CullFunctor cullFunctor) {
    cullFunctor = cullFunctor ? cullFunctor : [](const RenderArgs*, const AABox&){ return true; };

    // Prepare the ShapePipeline
    ShapePlumberPointer shapePlumber = std::make_shared<ShapePlumber>();
    {
        auto state = std::make_shared<gpu::State>();
        state->setCullMode(gpu::State::CULL_BACK);
        state->setDepthTest(true, true, gpu::LESS_EQUAL);

        initZPassPipelines(*shapePlumber, state);
    }

    const auto cachedMode = task.addJob<RenderShadowSetup>("ShadowSetup");

    // CPU jobs:
    // Fetch and cull the items from the scene
    auto shadowFilter = ItemFilter::Builder::visibleWorldItems().withTypeShape().withOpaque().withoutLayered();
    const auto shadowSelection = task.addJob<FetchSpatialTree>("FetchShadowSelection", shadowFilter);
    const auto culledShadowSelection = task.addJob<CullSpatialSelection>("CullShadowSelection", shadowSelection, cullFunctor, RenderDetails::SHADOW, shadowFilter);

    // Sort
    const auto sortedPipelines = task.addJob<PipelineSortShapes>("PipelineSortShadowSort", culledShadowSelection);
    const auto sortedShapes = task.addJob<DepthSortShapes>("DepthSortShadowMap", sortedPipelines);

    // GPU jobs: Render to shadow map
    task.addJob<RenderShadowMap>("RenderShadowMap", sortedShapes, shapePlumber);

    task.addJob<RenderShadowTeardown>("ShadowTeardown", cachedMode);
}

void RenderShadowTask::configure(const Config& configuration) {
    DependencyManager::get<DeferredLightingEffect>()->setShadowMapEnabled(configuration.enabled);
    // This is a task, so must still propogate configure() to its Jobs
//    Task::configure(configuration);
}

void RenderShadowSetup::run(const render::RenderContextPointer& renderContext, Output& output) {
    auto lightStage = renderContext->_scene->getStage<LightStage>();
    assert(lightStage);
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

void RenderShadowTeardown::run(const render::RenderContextPointer& renderContext, const Input& input) {
    RenderArgs* args = renderContext->args;

    // Reset the render args
    args->popViewFrustum();
    args->_renderMode = input;
};
