
//
//  RenderForwardTask.cpp
//  render-utils/src/
//
//  Created by Zach Pomerantz on 12/13/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderForwardTask.h"
#include "RenderDeferredTask.h"

#include <PerfStat.h>
#include <PathUtils.h>
#include <RenderArgs.h>
#include <ViewFrustum.h>
#include <gpu/Context.h>

#include <render/CullTask.h>
#include <render/SortTask.h>
#include <render/DrawTask.h>
#include <render/DrawStatus.h>
#include <render/DrawSceneOctree.h>
#include <render/BlurTask.h>

#include "LightingModel.h"
#include "DebugDeferredBuffer.h"
#include "DeferredFramebuffer.h"
#include "DeferredLightingEffect.h"
#include "SurfaceGeometryPass.h"
#include "FramebufferCache.h"
#include "HitEffect.h"
#include "TextureCache.h"

#include "AmbientOcclusionEffect.h"
#include "AntialiasingEffect.h"
#include "ToneMappingEffect.h"
#include "SubsurfaceScattering.h"

#include <gpu/StandardShaderLib.h>

#include "drawOpaqueStencil_frag.h"


using namespace render;
extern void initOverlay3DPipelines(render::ShapePlumber& plumber);
extern void initDeferredPipelines(render::ShapePlumber& plumber);

RenderForwardTask::RenderForwardTask(CullFunctor cullFunctor) {
    // Prepare the ShapePipelines
    ShapePlumberPointer shapePlumber = std::make_shared<ShapePlumber>();
    initDeferredPipelines(*shapePlumber);

    // CPU jobs:
    // Fetch and cull the items from the scene
    const auto spatialSelection = addJob<FetchSpatialTree>("FetchSceneSelection");

    cullFunctor = cullFunctor ? cullFunctor : [](const RenderArgs*, const AABox&){ return true; };
    auto spatialFilter = ItemFilter::Builder::visibleWorldItems().withoutLayered();
    const auto culledSpatialSelection = addJob<CullSpatialSelection>("CullSceneSelection", spatialSelection, cullFunctor, RenderDetails::ITEM, spatialFilter);

    // Overlays are not culled
    const auto nonspatialSelection = addJob<FetchNonspatialItems>("FetchOverlaySelection");

    // Multi filter visible items into different buckets
    const int NUM_FILTERS = 3;
    const int OPAQUE_SHAPE_BUCKET = 0;
    const int TRANSPARENT_SHAPE_BUCKET = 1;
    const int LIGHT_BUCKET = 2;
    const int BACKGROUND_BUCKET = 2;
    MultiFilterItem<NUM_FILTERS>::ItemFilterArray spatialFilters = { {
            ItemFilter::Builder::opaqueShape(),
            ItemFilter::Builder::transparentShape(),
            ItemFilter::Builder::light()
    } };
    MultiFilterItem<NUM_FILTERS>::ItemFilterArray nonspatialFilters = { {
            ItemFilter::Builder::opaqueShape(),
            ItemFilter::Builder::transparentShape(),
            ItemFilter::Builder::background()
    } };
    const auto filteredSpatialBuckets = addJob<MultiFilterItem<NUM_FILTERS>>("FilterSceneSelection", culledSpatialSelection, spatialFilters).get<MultiFilterItem<NUM_FILTERS>::ItemBoundsArray>();
    const auto filteredNonspatialBuckets = addJob<MultiFilterItem<NUM_FILTERS>>("FilterOverlaySelection", nonspatialSelection, nonspatialFilters).get<MultiFilterItem<NUM_FILTERS>::ItemBoundsArray>();

    // Extract / Sort opaques / Transparents / Lights / Overlays
    const auto opaques = addJob<DepthSortItems>("DepthSortOpaque", filteredSpatialBuckets[OPAQUE_SHAPE_BUCKET]);
    const auto transparents = addJob<DepthSortItems>("DepthSortTransparent", filteredSpatialBuckets[TRANSPARENT_SHAPE_BUCKET], DepthSortItems(false));
    const auto lights = filteredSpatialBuckets[LIGHT_BUCKET];

    const auto overlayOpaques = addJob<DepthSortItems>("DepthSortOverlayOpaque", filteredNonspatialBuckets[OPAQUE_SHAPE_BUCKET]);
    const auto overlayTransparents = addJob<DepthSortItems>("DepthSortOverlayTransparent", filteredNonspatialBuckets[TRANSPARENT_SHAPE_BUCKET], DepthSortItems(false));
    const auto background = filteredNonspatialBuckets[BACKGROUND_BUCKET];

    const auto framebuffer = addJob<PrepareFramebuffer>("PrepareFramebuffer");

    // Blit!
    addJob<Blit>("Blit", framebuffer);
}

void RenderForwardTask::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    // sanity checks
    assert(sceneContext);
    if (!sceneContext->_scene) {
        return;
    }


    // Is it possible that we render without a viewFrustum ?
    if (!(renderContext->args && renderContext->args->hasViewFrustum())) {
        return;
    }

    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);

    for (auto job : _jobs) {
        job.run(sceneContext, renderContext);
    }
}

void PrepareFramebuffer::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, gpu::FramebufferPointer& framebuffer) {
    auto framebufferCache = DependencyManager::get<FramebufferCache>();
    auto framebufferSize = framebufferCache->getFrameBufferSize();
    glm::uvec2 frameSize(framebufferSize.width(), framebufferSize.height());

    // Resizing framebuffers instead of re-building them seems to cause issues with threaded rendering
    if (_framebuffer && _framebuffer->getSize() != frameSize) {
        _framebuffer.reset();
    }

    if (!_framebuffer) {
        _framebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("forward"));

        auto colorFormat = gpu::Element::COLOR_SRGBA_32;
        auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT);
        auto colorTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, frameSize.x, frameSize.y, defaultSampler));
        _framebuffer->setRenderBuffer(0, colorTexture);

        auto depthFormat = gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::DEPTH_STENCIL); // Depth24_Stencil8 texel format
        auto depthTexture = gpu::TexturePointer(gpu::Texture::create2D(depthFormat, frameSize.x, frameSize.y, defaultSampler));
        _framebuffer->setDepthStencilBuffer(depthTexture, depthFormat);
    }

    auto args = renderContext->args;
    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);

        batch.setFramebuffer(_framebuffer);
        batch.clearFramebuffer(
            gpu::Framebuffer::BUFFER_COLOR0 |
            gpu::Framebuffer::BUFFER_DEPTH |
            gpu::Framebuffer::BUFFER_STENCIL,
            vec4(vec3(0), 0), 1.0, 0.0, true);
    });

    framebuffer = _framebuffer;
}
