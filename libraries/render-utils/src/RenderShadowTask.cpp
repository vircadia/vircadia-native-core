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

void RenderShadowTask::configure(const Config& configuration) {
    DependencyManager::get<DeferredLightingEffect>()->setShadowMapEnabled(configuration.enabled);
    // This is a task, so must still propogate configure() to its Jobs
    //    Task::configure(configuration);
}

void RenderShadowTask::build(JobModel& task, const render::Varying& input, render::Varying& output, render::CullFunctor cameraCullFunctor, uint8_t tagBits, uint8_t tagMask) {
    ::CullFunctor shadowCullFunctor = [this](const RenderArgs* args, const AABox& bounds) {
        return _cullFunctor(args, bounds);
    };

    // Prepare the ShapePipeline
    ShapePlumberPointer shapePlumber = std::make_shared<ShapePlumber>();
    {
        auto state = std::make_shared<gpu::State>();
        state->setCullMode(gpu::State::CULL_BACK);
        state->setDepthTest(true, true, gpu::LESS_EQUAL);

        initZPassPipelines(*shapePlumber, state);
    }

    const auto setupOutput = task.addJob<RenderShadowSetup>("ShadowSetup");
    const auto queryResolution = setupOutput.getN<RenderShadowSetup::Outputs>(1);
    // Fetch and cull the items from the scene

    static const auto shadowCasterReceiverFilter = ItemFilter::Builder::visibleWorldItems().withTypeShape().withOpaque().withoutLayered().withTagBits(tagBits, tagMask);

    const auto fetchInput = FetchSpatialTree::Inputs(shadowCasterReceiverFilter, queryResolution).asVarying();
    const auto shadowSelection = task.addJob<FetchSpatialTree>("FetchShadowTree", fetchInput);
    const auto selectionInputs = FilterSpatialSelection::Inputs(shadowSelection, shadowCasterReceiverFilter).asVarying();
    const auto shadowItems = task.addJob<FilterSpatialSelection>("FilterShadowSelection", selectionInputs);

    // Cull objects that are not visible in camera view. Hopefully the cull functor only performs LOD culling, not
    // frustum culling or this will make shadow casters out of the camera frustum disappear.
    const auto cameraFrustum = setupOutput.getN<RenderShadowSetup::Outputs>(2);
    const auto applyFunctorInputs = ApplyCullFunctorOnItemBounds::Inputs(shadowItems, cameraFrustum).asVarying();
    const auto culledShadowItems = task.addJob<ApplyCullFunctorOnItemBounds>("ShadowCullCamera", applyFunctorInputs, cameraCullFunctor);

    // Sort
    const auto sortedPipelines = task.addJob<PipelineSortShapes>("PipelineSortShadow", culledShadowItems);
    const auto sortedShapes = task.addJob<DepthSortShapes>("DepthSortShadow", sortedPipelines, true);

    render::Varying cascadeFrustums[SHADOW_CASCADE_MAX_COUNT] = {
        ViewFrustumPointer()
#if SHADOW_CASCADE_MAX_COUNT>1
        ,ViewFrustumPointer(),
        ViewFrustumPointer(),
        ViewFrustumPointer()
#endif
    };

    Output cascadeSceneBBoxes;

    for (auto i = 0; i < SHADOW_CASCADE_MAX_COUNT; i++) {
        char jobName[64];
        sprintf(jobName, "ShadowCascadeSetup%d", i);
        const auto cascadeSetupOutput = task.addJob<RenderShadowCascadeSetup>(jobName, i, _cullFunctor, tagBits, tagMask);
        const auto shadowFilter = cascadeSetupOutput.getN<RenderShadowCascadeSetup::Outputs>(0);
        auto antiFrustum = render::Varying(ViewFrustumPointer());
        cascadeFrustums[i] = cascadeSetupOutput.getN<RenderShadowCascadeSetup::Outputs>(1);
        if (i > 1) {
            antiFrustum = cascadeFrustums[i - 2];
        }

        // CPU jobs: finer grained culling
        const auto cullInputs = CullShadowBounds::Inputs(sortedShapes, shadowFilter, antiFrustum).asVarying();
        sprintf(jobName, "CullShadowCascade%d", i);
        const auto culledShadowItemsAndBounds = task.addJob<CullShadowBounds>(jobName, cullInputs, shadowCullFunctor);

        // GPU jobs: Render to shadow map
        sprintf(jobName, "RenderShadowMap%d", i);
        task.addJob<RenderShadowMap>(jobName, culledShadowItemsAndBounds, shapePlumber, i);
        sprintf(jobName, "ShadowCascadeTeardown%d", i);
        task.addJob<RenderShadowCascadeTeardown>(jobName, shadowFilter);

        cascadeSceneBBoxes[i] = culledShadowItemsAndBounds.getN<CullShadowBounds::Outputs>(1);
    }

    output = render::Varying(cascadeSceneBBoxes);

    task.addJob<RenderShadowTeardown>("ShadowTeardown", setupOutput);
}

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
    if (!inShapeBounds.isNull()) {
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
        if (near > far) {
            near = far;
        }

        const auto depthEpsilon = 0.1f;
        auto projMatrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, near - depthEpsilon, far + depthEpsilon);
        auto shadowProjection = shadowFrustum.getProjection();

        shadowProjection[2][2] = projMatrix[2][2];
        shadowProjection[3][2] = projMatrix[3][2];
        shadowFrustum.setProjection(shadowProjection);
        shadowFrustum.calculate();
    }
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

    gpu::doInBatch("RenderShadowMap::run", args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;
        batch.enableStereo(false);

        glm::ivec4 viewport{0, 0, fbo->getWidth(), fbo->getHeight()};
        batch.setViewportTransform(viewport);
        batch.setStateScissorRect(viewport);

        batch.setFramebuffer(fbo);
        batch.clearDepthFramebuffer(1.0, false);

        if (!inShapeBounds.isNull()) {
            glm::mat4 projMat;
            Transform viewMat;
            args->getViewFrustum().evalProjectionMatrix(projMat);
            args->getViewFrustum().evalViewTransform(viewMat);

            batch.setProjectionTransform(projMat);
            batch.setViewTransform(viewMat, false);

            auto shadowPipeline = _shapePlumber->pickPipeline(args, defaultKeyBuilder);
            auto shadowSkinnedPipeline = _shapePlumber->pickPipeline(args, defaultKeyBuilder.withSkinned());
            auto shadowSkinnedDQPipeline = _shapePlumber->pickPipeline(args, defaultKeyBuilder.withSkinned().withDualQuatSkinned());

            std::vector<ShapeKey> skinnedShapeKeys{};
            std::vector<ShapeKey> skinnedDQShapeKeys{};
            std::vector<ShapeKey> ownPipelineShapeKeys{};

            // Iterate through all inShapes and render the unskinned
            args->_shapePipeline = shadowPipeline;
            batch.setPipeline(shadowPipeline->pipeline);
            for (auto items : inShapes) {
                if (items.first.isSkinned()) {
                    if (items.first.isDualQuatSkinned()) {
                        skinnedDQShapeKeys.push_back(items.first);
                    } else {
                        skinnedShapeKeys.push_back(items.first);
                    }
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

            // Reiterate to render the DQ skinned
            args->_shapePipeline = shadowSkinnedDQPipeline;
            batch.setPipeline(shadowSkinnedDQPipeline->pipeline);
            for (const auto& key : skinnedDQShapeKeys) {
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
        }

        args->_batch = nullptr;
    });
}

RenderShadowSetup::RenderShadowSetup() :
    _cameraFrustum{ std::make_shared<ViewFrustum>() },
    _coarseShadowFrustum{ std::make_shared<ViewFrustum>() } {

}

void RenderShadowSetup::configure(const Config& configuration) {
    setConstantBias(0, configuration.constantBias0);
    setSlopeBias(0, configuration.slopeBias0);
#if SHADOW_CASCADE_MAX_COUNT>1
    setConstantBias(1, configuration.constantBias1);
    setSlopeBias(1, configuration.slopeBias1);
    setConstantBias(2, configuration.constantBias2);
    setSlopeBias(2, configuration.slopeBias2);
    setConstantBias(3, configuration.constantBias3);
    setSlopeBias(3, configuration.slopeBias3);
#endif
}

void RenderShadowSetup::setConstantBias(int cascadeIndex, float value) {
    _bias[cascadeIndex]._constant = value * value * value * 0.004f;
}

void RenderShadowSetup::setSlopeBias(int cascadeIndex, float value) {
    _bias[cascadeIndex]._slope = value * value * value * 0.01f;
}

void RenderShadowSetup::run(const render::RenderContextPointer& renderContext, Outputs& output) {
    // Abort all jobs if not casting shadows
    auto lightStage = renderContext->_scene->getStage<LightStage>();
    assert(lightStage);
    if (!lightStage->getCurrentKeyLight() || !lightStage->getCurrentKeyLight()->getCastShadows()) {
        renderContext->taskFlow.abortTask();
        return;
    }

    // Cache old render args
    RenderArgs* args = renderContext->args;

    output.edit0() = args->_renderMode;
    output.edit1() = glm::ivec2(0, 0);
    // Save main camera frustum
    *_cameraFrustum = args->getViewFrustum();
    output.edit2() = _cameraFrustum;

    const auto globalShadow = lightStage->getCurrentKeyShadow();
    if (globalShadow) {
        globalShadow->setKeylightFrustum(args->getViewFrustum(), SHADOW_FRUSTUM_NEAR, SHADOW_FRUSTUM_FAR);

        auto& firstCascade = globalShadow->getCascade(0);
        auto& firstCascadeFrustum = firstCascade.getFrustum();
        unsigned int cascadeIndex;

        // Adjust each cascade frustum
        for (cascadeIndex = 0; cascadeIndex < globalShadow->getCascadeCount(); ++cascadeIndex) {
            auto& bias = _bias[cascadeIndex];
            globalShadow->setKeylightCascadeFrustum(cascadeIndex, args->getViewFrustum(),
                                                    SHADOW_FRUSTUM_NEAR, SHADOW_FRUSTUM_FAR,
                                                    bias._constant, bias._slope);
        }

        // Now adjust coarse frustum bounds
        auto frustumPosition = firstCascadeFrustum->getPosition();
        auto farTopLeft = firstCascadeFrustum->getFarTopLeft() - frustumPosition;
        auto farBottomRight = firstCascadeFrustum->getFarBottomRight() - frustumPosition;

        auto left = glm::dot(farTopLeft, firstCascadeFrustum->getRight());
        auto right = glm::dot(farBottomRight, firstCascadeFrustum->getRight());
        auto top = glm::dot(farTopLeft, firstCascadeFrustum->getUp());
        auto bottom = glm::dot(farBottomRight, firstCascadeFrustum->getUp());
        auto near = firstCascadeFrustum->getNearClip();
        auto far = firstCascadeFrustum->getFarClip();

        for (cascadeIndex = 1; cascadeIndex < globalShadow->getCascadeCount(); ++cascadeIndex) {
            auto& cascadeFrustum = globalShadow->getCascade(cascadeIndex).getFrustum();

            farTopLeft = cascadeFrustum->getFarTopLeft() - frustumPosition;
            farBottomRight = cascadeFrustum->getFarBottomRight() - frustumPosition;

            auto cascadeLeft = glm::dot(farTopLeft, cascadeFrustum->getRight());
            auto cascadeRight = glm::dot(farBottomRight, cascadeFrustum->getRight());
            auto cascadeTop = glm::dot(farTopLeft, cascadeFrustum->getUp());
            auto cascadeBottom = glm::dot(farBottomRight, cascadeFrustum->getUp());
            auto cascadeNear = cascadeFrustum->getNearClip();
            auto cascadeFar = cascadeFrustum->getFarClip();
            left = glm::min(left, cascadeLeft);
            right = glm::max(right, cascadeRight);
            bottom = glm::min(bottom, cascadeBottom);
            top = glm::max(top, cascadeTop);
            near = glm::min(near, cascadeNear);
            far = glm::max(far, cascadeFar);
        }

        _coarseShadowFrustum->setPosition(firstCascadeFrustum->getPosition());
        _coarseShadowFrustum->setOrientation(firstCascadeFrustum->getOrientation());
        _coarseShadowFrustum->setProjection(glm::ortho<float>(left, right, bottom, top, near, far));
        _coarseShadowFrustum->calculate();

        // Push frustum for further culling and selection
        args->pushViewFrustum(*_coarseShadowFrustum);

        args->_renderMode = RenderArgs::SHADOW_RENDER_MODE;

        // We want for the octree query enough resolution to catch the details in the lowest cascade. So compute
        // the desired resolution for the first cascade frustum and extrapolate it to the coarse frustum.
        glm::ivec2 queryResolution = firstCascade.framebuffer->getSize();
        queryResolution.x = int(queryResolution.x * _coarseShadowFrustum->getWidth() / firstCascadeFrustum->getWidth());
        queryResolution.y = int(queryResolution.y * _coarseShadowFrustum->getHeight() / firstCascadeFrustum->getHeight());
        output.edit1() = queryResolution;
    }
}

void RenderShadowCascadeSetup::run(const render::RenderContextPointer& renderContext, Outputs& output) {
    auto lightStage = renderContext->_scene->getStage<LightStage>();
    assert(lightStage);

    // Cache old render args
    RenderArgs* args = renderContext->args;

    const auto globalShadow = lightStage->getCurrentKeyShadow();
    if (globalShadow && _cascadeIndex<globalShadow->getCascadeCount()) {
        // Second item filter is to filter items to keep in shadow frustum computation (here we need to keep shadow receivers)
        output.edit0() = ItemFilter::Builder::visibleWorldItems().withTypeShape().withOpaque().withoutLayered().withTagBits(_tagBits, _tagMask);

        // Set the keylight render args
        auto& cascade = globalShadow->getCascade(_cascadeIndex);
        auto& cascadeFrustum = cascade.getFrustum();
        args->pushViewFrustum(*cascadeFrustum);
        auto texelSize = glm::min(cascadeFrustum->getHeight(), cascadeFrustum->getWidth()) / cascade.framebuffer->getSize().x;
        // Set the cull threshold to 24 shadow texels. This is totally arbitrary
        const auto minTexelCount = 24.0f;
        // TODO : maybe adapt that with LOD management system?
        texelSize *= minTexelCount;
        _cullFunctor._minSquareSize = texelSize * texelSize;

        output.edit1() = cascadeFrustum;
    } else {
        output.edit0() = ItemFilter::Builder::nothing();
        output.edit1() = ViewFrustumPointer();
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
}

static AABox& merge(AABox& box, const AABox& otherBox, const glm::vec3& dir) {
    if (!otherBox.isInvalid()) {
        int vertexIndex = 0;
        vertexIndex |= ((dir.z > 0.0f) & 1) << 2;
        vertexIndex |= ((dir.y > 0.0f) & 1) << 1;
        vertexIndex |= ((dir.x < 0.0f) & 1);
        auto vertex = otherBox.getVertex((BoxVertex)vertexIndex);
        if (!box.isInvalid()) {
            const auto boxCenter = box.calcCenter();
            vertex -= boxCenter;
            vertex = dir * glm::max(0.0f, glm::dot(vertex, dir));
            vertex += boxCenter;
        }
        box += vertex;
    }
    return box;
}

void CullShadowBounds::run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    RenderArgs* args = renderContext->args;

    const auto& inShapes = inputs.get0();
    const auto& filter = inputs.get1();
    ViewFrustumPointer antiFrustum;
    auto& outShapes = outputs.edit0();
    auto& outBounds = outputs.edit1();

    if (!inputs[3].isNull()) {
        antiFrustum = inputs.get2();
    }
    outShapes.clear();
    outBounds = AABox();

    if (!filter.selectsNothing()) {
        auto& details = args->_details.edit(RenderDetails::SHADOW);
        render::CullTest test(_cullFunctor, args, details, antiFrustum);
        auto scene = args->_scene;
        auto lightStage = renderContext->_scene->getStage<LightStage>();
        assert(lightStage);
        const auto globalLightDir = lightStage->getCurrentKeyLight()->getDirection();
        auto castersFilter = render::ItemFilter::Builder(filter).withShadowCaster().build();
        const auto& receiversFilter = filter;

        for (auto& inItems : inShapes) {
            auto key = inItems.first;
            auto outItems = outShapes.find(key);
            if (outItems == outShapes.end()) {
                outItems = outShapes.insert(std::make_pair(key, ItemBounds{})).first;
                outItems->second.reserve(inItems.second.size());
            }

            details._considered += (int)inItems.second.size();

            if (antiFrustum == nullptr) {
                for (auto& item : inItems.second) {
                    if (test.solidAngleTest(item.bound) && test.frustumTest(item.bound)) {
                        const auto shapeKey = scene->getItem(item.id).getKey();
                        if (castersFilter.test(shapeKey)) {
                            outItems->second.emplace_back(item);
                            outBounds += item.bound;
                        } else if (receiversFilter.test(shapeKey)) {
                            // Receivers are not rendered but they still increase the bounds of the shadow scene
                            // although only in the direction of the light direction so as to have a correct far
                            // distance without decreasing the near distance.
                            merge(outBounds, item.bound, globalLightDir);
                        }
                    }
                }
            } else {
                for (auto& item : inItems.second) {
                    if (test.solidAngleTest(item.bound) && test.frustumTest(item.bound) && test.antiFrustumTest(item.bound)) {
                        const auto shapeKey = scene->getItem(item.id).getKey();
                        if (castersFilter.test(shapeKey)) {
                            outItems->second.emplace_back(item);
                            outBounds += item.bound;
                        } else if (receiversFilter.test(shapeKey)) {
                            // Receivers are not rendered but they still increase the bounds of the shadow scene
                            // although only in the direction of the light direction so as to have a correct far
                            // distance without decreasing the near distance.
                            merge(outBounds, item.bound, globalLightDir);
                        }
                    }
                }
            }
            details._rendered += (int)outItems->second.size();
        }

        for (auto& items : outShapes) {
            items.second.shrink_to_fit();
        }
    }
}
