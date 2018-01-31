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

    const auto setupOutput = task.addJob<RenderShadowSetup>("ShadowSetup");
    // Fetch and cull the items from the scene
    static const auto shadowCasterFilter = ItemFilter::Builder::visibleWorldItems().withTypeShape().withOpaque().withoutLayered();
    const auto fetchInput = render::Varying(shadowCasterFilter);
    const auto shadowSelection = task.addJob<FetchSpatialTree>("FetchShadowTree", fetchInput);
    const auto selectionInputs = FetchSpatialSelection::Inputs(shadowSelection, shadowCasterFilter).asVarying();
    const auto shadowItems = task.addJob<FetchSpatialSelection>("FetchShadowSelection", selectionInputs);

    // Sort
    const auto sortedPipelines = task.addJob<PipelineSortShapes>("PipelineSortShadow", shadowItems);
    const auto sortedShapes = task.addJob<DepthSortShapes>("DepthSortShadow", sortedPipelines, true);

    for (auto i = 0; i < SHADOW_CASCADE_MAX_COUNT; i++) {
        char jobName[64];
        sprintf(jobName, "ShadowCascadeSetup%d", i);
        const auto shadowFilter = task.addJob<RenderShadowCascadeSetup>(jobName, i);

        // CPU jobs: finer grained culling
        const auto cullInputs = CullShapeBounds::Inputs(sortedShapes, shadowFilter).asVarying();
        const auto culledShadowItemsAndBounds = task.addJob<CullShapeBounds>("CullShadowCascade", cullInputs, cullFunctor, RenderDetails::SHADOW);

        // GPU jobs: Render to shadow map
        sprintf(jobName, "RenderShadowMap%d", i);
        task.addJob<RenderShadowMap>(jobName, culledShadowItemsAndBounds, shapePlumber, i);
        task.addJob<RenderShadowCascadeTeardown>("ShadowCascadeTeardown", shadowFilter);
    }

    task.addJob<RenderShadowTeardown>("ShadowTeardown", setupOutput);
}

void RenderShadowTask::configure(const Config& configuration) {
    DependencyManager::get<DeferredLightingEffect>()->setShadowMapEnabled(configuration.enabled);
    // This is a task, so must still propogate configure() to its Jobs
//    Task::configure(configuration);
}

RenderShadowSetup::RenderShadowSetup() :
    _coarseShadowFrustum{ std::make_shared<ViewFrustum>() } {

}

void RenderShadowSetup::configure(const Config& configuration) {
    setConstantBias(0, configuration.constantBias0);
    setConstantBias(1, configuration.constantBias1);
    setConstantBias(2, configuration.constantBias2);
    setConstantBias(3, configuration.constantBias3);
    setSlopeBias(0, configuration.slopeBias0);
    setSlopeBias(1, configuration.slopeBias1);
    setSlopeBias(2, configuration.slopeBias2);
    setSlopeBias(3, configuration.slopeBias3);
}

void RenderShadowSetup::setConstantBias(int cascadeIndex, float value) {
    _bias[cascadeIndex]._constant = value * value * value * 0.004f;
}

void RenderShadowSetup::setSlopeBias(int cascadeIndex, float value) {
    _bias[cascadeIndex]._slope = value * value * value * 0.01f;
}

void RenderShadowSetup::run(const render::RenderContextPointer& renderContext, Outputs& output) {
    auto lightStage = renderContext->_scene->getStage<LightStage>();
    assert(lightStage);
    // Cache old render args
    RenderArgs* args = renderContext->args;

    output.edit0() = args->_renderMode;
    output.edit1() = args->_sizeScale;

    const auto globalShadow = lightStage->getCurrentKeyShadow();
    if (globalShadow) {
        globalShadow->setKeylightFrustum(args->getViewFrustum(), SHADOW_FRUSTUM_NEAR, SHADOW_FRUSTUM_FAR);
        auto& firstCascadeFrustum = globalShadow->getCascade(0).getFrustum();
        unsigned int cascadeIndex;
        _coarseShadowFrustum->setPosition(firstCascadeFrustum->getPosition());
        _coarseShadowFrustum->setOrientation(firstCascadeFrustum->getOrientation());

        // Adjust each cascade frustum
        for (cascadeIndex = 0; cascadeIndex < globalShadow->getCascadeCount(); ++cascadeIndex) {
            auto& bias = _bias[cascadeIndex];
            globalShadow->setKeylightCascadeFrustum(cascadeIndex, args->getViewFrustum(),
                                                    SHADOW_FRUSTUM_NEAR, SHADOW_FRUSTUM_FAR,
                                                    bias._constant, bias._slope);
        }
        // Now adjust coarse frustum bounds
        auto left = glm::dot(firstCascadeFrustum->getFarTopLeft(), firstCascadeFrustum->getRight());
        auto right = glm::dot(firstCascadeFrustum->getFarTopRight(), firstCascadeFrustum->getRight());
        auto top = glm::dot(firstCascadeFrustum->getFarTopLeft(), firstCascadeFrustum->getUp());
        auto bottom = glm::dot(firstCascadeFrustum->getFarBottomRight(), firstCascadeFrustum->getUp());
        auto near = firstCascadeFrustum->getNearClip();
        auto far = firstCascadeFrustum->getFarClip();
        for (cascadeIndex = 1; cascadeIndex < globalShadow->getCascadeCount(); ++cascadeIndex) {
            auto& cascadeFrustum = globalShadow->getCascade(cascadeIndex).getFrustum();
            auto cascadeLeft = glm::dot(cascadeFrustum->getFarTopLeft(), cascadeFrustum->getRight());
            auto cascadeRight = glm::dot(cascadeFrustum->getFarTopRight(), cascadeFrustum->getRight());
            auto cascadeTop = glm::dot(cascadeFrustum->getFarTopLeft(), cascadeFrustum->getUp());
            auto cascadeBottom = glm::dot(cascadeFrustum->getFarBottomRight(), cascadeFrustum->getUp());
            auto cascadeNear = cascadeFrustum->getNearClip();
            auto cascadeFar = cascadeFrustum->getFarClip();
            left = glm::min(left, cascadeLeft);
            right = glm::max(right, cascadeRight);
            bottom = glm::min(bottom, cascadeBottom);
            top = glm::max(top, cascadeTop);
            near = glm::min(near, cascadeNear);
            far = glm::max(far, cascadeFar);
        }
        _coarseShadowFrustum->setProjection(glm::ortho<float>(left, right, bottom, top, near, far));
        _coarseShadowFrustum->calculate();

        // Push frustum for further culling and selection
        args->pushViewFrustum(*_coarseShadowFrustum);

        args->_renderMode = RenderArgs::SHADOW_RENDER_MODE;
        if (lightStage->getCurrentKeyLight()->getType() == graphics::Light::SUN) {
            // Set to ridiculously high amount to prevent solid angle culling in octree selection
            args->_sizeScale = 1e16f;
        }
    }
}

void RenderShadowCascadeSetup::run(const render::RenderContextPointer& renderContext, Outputs& output) {
    auto lightStage = renderContext->_scene->getStage<LightStage>();
    assert(lightStage);
    // Cache old render args
    RenderArgs* args = renderContext->args;

    const auto globalShadow = lightStage->getCurrentKeyShadow();
    if (globalShadow && _cascadeIndex<globalShadow->getCascadeCount()) {
        output = ItemFilter::Builder::visibleWorldItems().withTypeShape().withOpaque().withoutLayered();

        // Set the keylight render args
        auto& cascade = globalShadow->getCascade(_cascadeIndex);
        auto& cascadeFrustum = cascade.getFrustum();
        args->pushViewFrustum(*cascadeFrustum);
        // Set the cull threshold to 2 shadow texels.
        auto texelSize = glm::max(cascadeFrustum->getHeight(), cascadeFrustum->getWidth()) / cascade.framebuffer->getSize().x;
        texelSize *= 2.0f;
        // SizeScale is used in the shadow cull function defined ine RenderViewTask
        args->_sizeScale = texelSize * texelSize;
    } else {
        output = ItemFilter::Builder::nothing();
    }
}

void RenderShadowCascadeTeardown::run(const render::RenderContextPointer& renderContext, const Input& input) {
    RenderArgs* args = renderContext->args;

    if (args->_renderMode == RenderArgs::SHADOW_RENDER_MODE && !input.selectsNothing()) {
        args->popViewFrustum();
    }
    assert(args->hasViewFrustum());
}

void RenderShadowTeardown::run(const render::RenderContextPointer& renderContext, const Input& input) {
    RenderArgs* args = renderContext->args;

    if (args->_renderMode == RenderArgs::SHADOW_RENDER_MODE) {
        args->popViewFrustum();
    }
    assert(args->hasViewFrustum());
    // Reset the render args
    args->_renderMode = input.get0();
    args->_sizeScale = input.get1();
}
