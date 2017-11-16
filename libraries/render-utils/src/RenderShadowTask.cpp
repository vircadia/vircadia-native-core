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
    // This indirection array is just a protection in case the ViewFrustum::PlaneIndex enum
    // changes order especially as we don't need to test the NEAR and FAR planes.
    static const ViewFrustum::PlaneIndex planeIndices[4] = {
        ViewFrustum::TOP_PLANE,
        ViewFrustum::BOTTOM_PLANE,
        ViewFrustum::LEFT_PLANE,
        ViewFrustum::RIGHT_PLANE
    };
    // Same goes for the shadow frustum planes.
    for (i = 0; i < 4; i++) {
        const auto& worldPlane = shadowFrustum.getPlanes()[planeIndices[i]];
        // We assume the transform doesn't have a non uniform scale component to apply the 
        // transform to the normal without using the correct transpose of inverse, which should be the
        // case for a view matrix.
        auto planeNormal = shadowViewInverse.transformDirection(worldPlane.getNormal());
        auto planePoint = shadowViewInverse.transform(worldPlane.getPoint());
        shadowClipPlanes[i].setNormalAndPoint(planeNormal, planePoint);
    }

    float near = std::numeric_limits<float>::max();
    float far = 0.0f;

    computeNearFar(sceneBoundVertices, shadowClipPlanes, near, far);
    // Limit the far range to the one used originally. There's no point in rendering objects
    // that are not in the view frustum.
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

    const auto& fbo = shadow->getCascade(_cascadeIndex).framebuffer;

    RenderArgs* args = renderContext->args;
    ShapeKey::Builder defaultKeyBuilder;
    auto adjustedShadowFrustum = args->getViewFrustum();

    // Adjust the frustum near and far depths based on the rendered items bounding box to have
    // the minimal Z range.
    adjustNearFar(inShapeBounds, adjustedShadowFrustum);
    // Reapply the frustum as it has been adjusted
    shadow->setFrustum(_cascadeIndex, adjustedShadowFrustum);
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
    cullFunctor = cullFunctor ? cullFunctor : [](const RenderArgs*, const AABox&) { return true; };

    // Prepare the ShapePipeline
    ShapePlumberPointer shapePlumber = std::make_shared<ShapePlumber>();
    {
        auto state = std::make_shared<gpu::State>();
        state->setCullMode(gpu::State::CULL_BACK);
        state->setDepthTest(true, true, gpu::LESS_EQUAL);

        initZPassPipelines(*shapePlumber, state);
    }

    for (auto i = 0; i < SHADOW_CASCADE_MAX_COUNT; i++) {
        const auto setupOutput = task.addJob<RenderShadowSetup>("ShadowSetup", i);
        const auto shadowFilter = setupOutput.getN<RenderShadowSetup::Outputs>(1);

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

        task.addJob<RenderShadowTeardown>("ShadowTeardown", setupOutput);
    }
}

void RenderShadowTask::configure(const Config& configuration) {
    DependencyManager::get<DeferredLightingEffect>()->setShadowMapEnabled(configuration.enabled);
    // This is a task, so must still propogate configure() to its Jobs
//    Task::configure(configuration);
}

void RenderShadowSetup::run(const render::RenderContextPointer& renderContext, Outputs& output) {
    auto lightStage = renderContext->_scene->getStage<LightStage>();
    assert(lightStage);
    // Cache old render args
    RenderArgs* args = renderContext->args;

    output.edit0() = args->_renderMode;

    const auto globalShadow = lightStage->getCurrentKeyShadow();
    if (globalShadow && _cascadeIndex<globalShadow->getCascadeCount()) {
        output.edit1() = ItemFilter::Builder::visibleWorldItems().withTypeShape().withOpaque().withoutLayered();

        const auto nearClip = args->getViewFrustum().getNearClip();
        const auto farClip = args->getViewFrustum().getFarClip();
        const auto nearDepth = -args->_boomOffset.z;

        static const float HIGH_CASCADE_MAX_DISTANCE = 20.0f;
        float maxCascadeDistance = HIGH_CASCADE_MAX_DISTANCE;
        float minCascadeDistance = nearClip;
        float shadowOverlapDistance = 0.0f;

        if (globalShadow->getCascadeCount() > 1) {
            static const float LOW_CASCADE_MAX_DISTANCE = 2.0f;
            const float cascadeLevelScale = powf(HIGH_CASCADE_MAX_DISTANCE / LOW_CASCADE_MAX_DISTANCE, 1.0f / (globalShadow->getCascadeCount() - 1));

            maxCascadeDistance = HIGH_CASCADE_MAX_DISTANCE / powf(cascadeLevelScale, globalShadow->getCascadeCount() - 1 - _cascadeIndex);
            minCascadeDistance = maxCascadeDistance / cascadeLevelScale;
        }

        shadowOverlapDistance = (maxCascadeDistance - minCascadeDistance) / 3.0f;
        maxCascadeDistance += shadowOverlapDistance;
        if (_cascadeIndex == 0) {
            minCascadeDistance = nearDepth;
        }
        minCascadeDistance = std::max(minCascadeDistance, nearDepth);
        maxCascadeDistance = std::min(maxCascadeDistance, farClip);
        globalShadow->setKeylightFrustum(_cascadeIndex, args->getViewFrustum(), minCascadeDistance, maxCascadeDistance, shadowOverlapDistance, SHADOW_FRUSTUM_NEAR, SHADOW_FRUSTUM_FAR);

        // Set the keylight render args
        args->pushViewFrustum(*(globalShadow->getCascade(_cascadeIndex).getFrustum()));
        args->_renderMode = RenderArgs::SHADOW_RENDER_MODE;
    } else {
        output.edit1() = ItemFilter::Builder::nothing();
    }
}

void RenderShadowTeardown::run(const render::RenderContextPointer& renderContext, const Input& input) {
    RenderArgs* args = renderContext->args;

    if (args->_renderMode == RenderArgs::SHADOW_RENDER_MODE && !input.get1().selectsNothing()) {
        args->popViewFrustum();
    }
    assert(args->hasViewFrustum());
    // Reset the render args
    args->_renderMode = input.get0();
};
