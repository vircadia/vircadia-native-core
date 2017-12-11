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

#include "RenderUtilsLogging.h"

// These values are used for culling the objects rendered in the shadow map
// but are readjusted afterwards
#define SHADOW_FRUSTUM_NEAR 1.0f
#define SHADOW_FRUSTUM_FAR  500.0f

using namespace render;

extern void initZPassPipelines(ShapePlumber& plumber, gpu::StatePointer state);

static void computeNearFar(const Triangle& triangle, const Plane shadowClipPlanes[4], float& near, float& far) {
    static const int MAX_TRIANGLE_COUNT = 16;
    Triangle clippedTriangles[MAX_TRIANGLE_COUNT];
    auto clippedTriangleCount = clipTriangleWithPlanes(triangle, shadowClipPlanes, 4, clippedTriangles, MAX_TRIANGLE_COUNT);

    for (auto i = 0; i < clippedTriangleCount; i++) {
        const auto& clippedTriangle = clippedTriangles[i];

        near = glm::min(near, -clippedTriangle.v0.z);
        near = glm::min(near, -clippedTriangle.v1.z);
        near = glm::min(near, -clippedTriangle.v2.z);

        far = glm::max(far, -clippedTriangle.v0.z);
        far = glm::max(far, -clippedTriangle.v1.z);
        far = glm::max(far, -clippedTriangle.v2.z);
    }
}

static void computeNearFar(const glm::vec3 sceneBoundVertices[8], const Plane shadowClipPlanes[4], float& near, float& far) {
    // This code is inspired from Microsoft's CascadedShadowMaps11 sample which is under MIT licence.
    // See https://code.msdn.microsoft.com/windowsdesktop/Direct3D-Shadow-Win32-2d72a4f2/sourcecode?fileId=121915&pathId=1645833187
    // Basically it decomposes the object bounding box in triangles and clips each triangle with the shadow
    // frustum planes. Finally it computes the minimum and maximum depth of the clipped triangle vertices 
    // in shadow space to extract the near and far distances of the shadow frustum.
    static const std::array<int[4], 6> boxQuadVertexIndices = { {
        { TOP_LEFT_FAR, BOTTOM_LEFT_FAR, BOTTOM_RIGHT_FAR, TOP_RIGHT_FAR },
        { TOP_LEFT_NEAR, BOTTOM_LEFT_NEAR, BOTTOM_RIGHT_NEAR, TOP_RIGHT_NEAR },
        { TOP_RIGHT_FAR, BOTTOM_RIGHT_FAR, BOTTOM_RIGHT_NEAR, TOP_RIGHT_NEAR },
        { TOP_LEFT_FAR, BOTTOM_LEFT_FAR, BOTTOM_LEFT_NEAR, TOP_LEFT_NEAR },
        { BOTTOM_LEFT_FAR, BOTTOM_RIGHT_FAR, BOTTOM_RIGHT_NEAR, BOTTOM_LEFT_NEAR },
        { TOP_LEFT_FAR, TOP_RIGHT_FAR, TOP_RIGHT_NEAR, TOP_LEFT_NEAR }
    }  };
    Triangle triangle;

    for (auto quadVertexIndices : boxQuadVertexIndices) {
        triangle.v0 = sceneBoundVertices[quadVertexIndices[0]];
        triangle.v1 = sceneBoundVertices[quadVertexIndices[1]];
        triangle.v2 = sceneBoundVertices[quadVertexIndices[2]];
        computeNearFar(triangle, shadowClipPlanes, near, far);
        triangle.v1 = sceneBoundVertices[quadVertexIndices[3]];
        computeNearFar(triangle, shadowClipPlanes, near, far);
    }
}

static void adjustNearFar(const AABox& inShapeBounds, ViewFrustum& shadowFrustum) {
    const Transform shadowView{ shadowFrustum.getView() };
    const Transform shadowViewInverse{ shadowView.getInverseMatrix() };

    glm::vec3 sceneBoundVertices[8];
    // Keep only the left, right, top and bottom shadow frustum planes as we wish to determine
    // the near and far
    Plane shadowClipPlanes[4];
    int i;

    // The vertices of the scene bounding box are expressed in the shadow frustum's local space
    for (i = 0; i < 8; i++) {
        sceneBoundVertices[i] = shadowViewInverse.transform(inShapeBounds.getVertex(static_cast<BoxVertex>(i)));
    }
    shadowFrustum.getUniformlyTransformedSidePlanes(shadowViewInverse, shadowClipPlanes);

    float near = std::numeric_limits<float>::max();
    float far = 0.0f;

    computeNearFar(sceneBoundVertices, shadowClipPlanes, near, far);
    // Limit the far range to the one used originally.
    far = glm::min(far, shadowFrustum.getFarClip());

    const auto depthEpsilon = 0.1f;
    auto projMatrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, near - depthEpsilon, far + depthEpsilon);
    auto shadowProjection = shadowFrustum.getProjection();

    shadowProjection[2][2] = projMatrix[2][2];
    shadowProjection[3][2] = projMatrix[3][2];
    shadowFrustum.setProjection(shadowProjection);
    shadowFrustum.calculate();
}

void RenderShadowMap::run(const render::RenderContextPointer& renderContext, const Inputs& inputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    const auto& inShapes = inputs.get0();
    const auto& inShapeBounds = inputs.get1();

    auto lightStage = renderContext->_scene->getStage<LightStage>();
    assert(lightStage);

    auto shadow = lightStage->getCurrentKeyShadow();
    if (!shadow || _cascadeIndex >= shadow->getCascadeCount()) {
        return;
    }

    auto& cascade = shadow->getCascade(_cascadeIndex);
    auto& fbo = cascade.framebuffer;

    RenderArgs* args = renderContext->args;
    ShapeKey::Builder defaultKeyBuilder;
    auto adjustedShadowFrustum = args->getViewFrustum();

    // Adjust the frustum near and far depths based on the rendered items bounding box to have
    // the minimal Z range.
    adjustNearFar(inShapeBounds, adjustedShadowFrustum);
    // Reapply the frustum as it has been adjusted
    shadow->setCascadeFrustum(_cascadeIndex, adjustedShadowFrustum);
    args->popViewFrustum();
    args->pushViewFrustum(adjustedShadowFrustum);

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

        glm::mat4 projMat;
        Transform viewMat;
        args->getViewFrustum().evalProjectionMatrix(projMat);
        args->getViewFrustum().evalViewTransform(viewMat);

        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat, false);

        auto shadowPipeline = _shapePlumber->pickPipeline(args, defaultKeyBuilder);
        auto shadowSkinnedPipeline = _shapePlumber->pickPipeline(args, defaultKeyBuilder.withSkinned());

        std::vector<ShapeKey> skinnedShapeKeys{};
        std::vector<ShapeKey> ownPipelineShapeKeys{};

        // Iterate through all inShapes and render the unskinned
        args->_shapePipeline = shadowPipeline;
        batch.setPipeline(shadowPipeline->pipeline);
        for (auto items : inShapes) {
            if (items.first.isSkinned()) {
                skinnedShapeKeys.push_back(items.first);
            } else if (!items.first.hasOwnPipeline()) {
                renderItems(renderContext, items.second);
            } else {
                ownPipelineShapeKeys.push_back(items.first);
            }
        }

        // Reiterate to render the skinned
        args->_shapePipeline = shadowSkinnedPipeline;
        batch.setPipeline(shadowSkinnedPipeline->pipeline);
        for (const auto& key : skinnedShapeKeys) {
            renderItems(renderContext, inShapes.at(key));
        }

        // Finally render the items with their own pipeline last to prevent them from breaking the
        // render state. This is probably a temporary code as there is probably something better
        // to do in the render call of objects that have their own pipeline.
        args->_shapePipeline = nullptr;
        for (const auto& key : ownPipelineShapeKeys) {
            args->_itemShapeKey = key._flags.to_ulong();
            renderItems(renderContext, inShapes.at(key));
        }

        args->_batch = nullptr;
    });
}

void RenderShadowTask::build(JobModel& task, const render::Varying& input, render::Varying& output, CullFunctor cullFunctor) {
    cullFunctor = cullFunctor ? cullFunctor : [](const RenderArgs*, const AABox&) { return true; };

    // Prepare the ShapePipeline
    ShapePlumberPointer shapePlumber = std::make_shared<ShapePlumber>();
    {
        auto state = std::make_shared<gpu::State>();
        state->setCullMode(gpu::State::CULL_BACK);
        state->setDepthTest(true, true, gpu::LESS_EQUAL);

        initZPassPipelines(*shapePlumber, state);
    }

    task.addJob<RenderShadowSetup>("ShadowSetup");

    for (auto i = 0; i < SHADOW_CASCADE_MAX_COUNT; i++) {
        const auto setupOutput = task.addJob<RenderShadowCascadeSetup>("ShadowCascadeSetup", i);
        const auto shadowFilter = setupOutput.getN<RenderShadowCascadeSetup::Outputs>(1);

        // CPU jobs:
        // Fetch and cull the items from the scene
        const auto shadowSelection = task.addJob<FetchSpatialTree>("FetchShadowSelection", shadowFilter);
        const auto cullInputs = CullSpatialSelection::Inputs(shadowSelection, shadowFilter).asVarying();
        const auto culledShadowSelection = task.addJob<CullSpatialSelection>("CullShadowSelection", cullInputs, cullFunctor, RenderDetails::SHADOW);

        // Sort
        const auto sortedPipelines = task.addJob<PipelineSortShapes>("PipelineSortShadowSort", culledShadowSelection);
        const auto sortedShapesAndBounds = task.addJob<DepthSortShapesAndComputeBounds>("DepthSortShadowMap", sortedPipelines, true);

        // GPU jobs: Render to shadow map
        task.addJob<RenderShadowMap>("RenderShadowMap", sortedShapesAndBounds, shapePlumber, i);
        task.addJob<RenderShadowCascadeTeardown>("ShadowCascadeTeardown", setupOutput);
    }
}

void RenderShadowTask::configure(const Config& configuration) {
    DependencyManager::get<DeferredLightingEffect>()->setShadowMapEnabled(configuration.enabled);
    // This is a task, so must still propogate configure() to its Jobs
//    Task::configure(configuration);
}

void RenderShadowSetup::run(const render::RenderContextPointer& renderContext) {
    auto lightStage = renderContext->_scene->getStage<LightStage>();
    assert(lightStage);
    // Cache old render args
    RenderArgs* args = renderContext->args;

    const auto globalShadow = lightStage->getCurrentKeyShadow();
    if (globalShadow) {
        globalShadow->setKeylightFrustum(args->getViewFrustum(), SHADOW_FRUSTUM_NEAR, SHADOW_FRUSTUM_FAR);
    }
}

void RenderShadowCascadeSetup::run(const render::RenderContextPointer& renderContext, Outputs& output) {
    auto lightStage = renderContext->_scene->getStage<LightStage>();
    assert(lightStage);
    // Cache old render args
    RenderArgs* args = renderContext->args;

    output.edit0() = args->_renderMode;
    output.edit2() = args->_sizeScale;

    const auto globalShadow = lightStage->getCurrentKeyShadow();
    if (globalShadow && _cascadeIndex<globalShadow->getCascadeCount()) {
        output.edit1() = ItemFilter::Builder::visibleWorldItems().withTypeShape().withOpaque().withoutLayered();

        globalShadow->setKeylightCascadeFrustum(_cascadeIndex, args->getViewFrustum(), SHADOW_FRUSTUM_NEAR, SHADOW_FRUSTUM_FAR);

        // Set the keylight render args
        args->pushViewFrustum(*(globalShadow->getCascade(_cascadeIndex).getFrustum()));
        args->_renderMode = RenderArgs::SHADOW_RENDER_MODE;
        if (lightStage->getCurrentKeyLight()->getType() == model::Light::SUN) {
            const float shadowSizeScale = 1e16f;
            // Set the size scale to a ridiculously high value to prevent small object culling which assumes
            // the view frustum is a perspective projection. But this isn't the case for the sun which
            // is an orthographic projection.
            args->_sizeScale = shadowSizeScale;
        }

    } else {
        output.edit1() = ItemFilter::Builder::nothing();
    }
}

void RenderShadowCascadeTeardown::run(const render::RenderContextPointer& renderContext, const Input& input) {
    RenderArgs* args = renderContext->args;

    if (args->_renderMode == RenderArgs::SHADOW_RENDER_MODE && !input.get1().selectsNothing()) {
        args->popViewFrustum();
    }
    assert(args->hasViewFrustum());
    // Reset the render args
    args->_renderMode = input.get0();
    args->_sizeScale = input.get2();
};
