
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

    // Prepare deferred, generate the shared Deferred Frame Transform
    const auto deferredFrameTransform = addJob<GenerateDeferredFrameTransform>("DeferredFrameTransform");
    const auto lightingModel = addJob<MakeLightingModel>("LightingModel");


    // GPU jobs: Start preparing the primary, deferred and lighting buffer
    const auto primaryFramebuffer = addJob<PreparePrimaryFramebuffer>("PreparePrimaryBuffer");

   // const auto fullFrameRangeTimer = addJob<BeginGPURangeTimer>("BeginRangeTimer");
    const auto opaqueRangeTimer = addJob<BeginGPURangeTimer>("BeginOpaqueRangeTimer", "DrawOpaques");

    const auto prepareDeferredInputs = PrepareDeferred::Inputs(primaryFramebuffer, lightingModel).hasVarying();
    const auto prepareDeferredOutputs = addJob<PrepareDeferred>("PrepareDeferred", prepareDeferredInputs);
    const auto deferredFramebuffer = prepareDeferredOutputs.getN<PrepareDeferred::Outputs>(0);
    const auto lightingFramebuffer = prepareDeferredOutputs.getN<PrepareDeferred::Outputs>(1);

    // Render opaque objects in DeferredBuffer
    const auto opaqueInputs = DrawStateSortDeferred::Inputs(opaques, lightingModel).hasVarying();
    addJob<DrawStateSortDeferred>("DrawOpaqueDeferred", opaqueInputs, shapePlumber);

    // Once opaque is all rendered create stencil background
    addJob<DrawStencilDeferred>("DrawOpaqueStencil", deferredFramebuffer);

    addJob<EndGPURangeTimer>("OpaqueRangeTimer", opaqueRangeTimer);


    // Opaque all rendered

    // Linear Depth Pass
    const auto linearDepthPassInputs = LinearDepthPass::Inputs(deferredFrameTransform, deferredFramebuffer).hasVarying();
    const auto linearDepthPassOutputs = addJob<LinearDepthPass>("LinearDepth", linearDepthPassInputs);
    const auto linearDepthTarget = linearDepthPassOutputs.getN<LinearDepthPass::Outputs>(0);
    
    // Curvature pass
    const auto surfaceGeometryPassInputs = SurfaceGeometryPass::Inputs(deferredFrameTransform, deferredFramebuffer, linearDepthTarget).hasVarying();
    const auto surfaceGeometryPassOutputs = addJob<SurfaceGeometryPass>("SurfaceGeometry", surfaceGeometryPassInputs);
    const auto surfaceGeometryFramebuffer = surfaceGeometryPassOutputs.getN<SurfaceGeometryPass::Outputs>(0);
    const auto curvatureFramebuffer = surfaceGeometryPassOutputs.getN<SurfaceGeometryPass::Outputs>(1);
    const auto midCurvatureNormalFramebuffer = surfaceGeometryPassOutputs.getN<SurfaceGeometryPass::Outputs>(2);
    const auto lowCurvatureNormalFramebuffer = surfaceGeometryPassOutputs.getN<SurfaceGeometryPass::Outputs>(3);

    // Simply update the scattering resource
    const auto scatteringResource = addJob<SubsurfaceScattering>("Scattering");

    // AO job
    const auto ambientOcclusionInputs = AmbientOcclusionEffect::Inputs(deferredFrameTransform, deferredFramebuffer, linearDepthTarget).hasVarying();
    const auto ambientOcclusionOutputs = addJob<AmbientOcclusionEffect>("AmbientOcclusion", ambientOcclusionInputs);
    const auto ambientOcclusionFramebuffer = ambientOcclusionOutputs.getN<AmbientOcclusionEffect::Outputs>(0);
    const auto ambientOcclusionUniforms = ambientOcclusionOutputs.getN<AmbientOcclusionEffect::Outputs>(1);

    
    // Draw Lights just add the lights to the current list of lights to deal with. NOt really gpu job for now.
    addJob<DrawLight>("DrawLight", lights);

    // Light Clustering
    // Create the cluster grid of lights, cpu job for now
    const auto lightClusteringPassInputs = LightClusteringPass::Inputs(deferredFrameTransform, lightingModel, linearDepthTarget).hasVarying();
    const auto lightClusters = addJob<LightClusteringPass>("LightClustering", lightClusteringPassInputs);
    
    
    // DeferredBuffer is complete, now let's shade it into the LightingBuffer
    const auto deferredLightingInputs = RenderDeferred::Inputs(deferredFrameTransform, deferredFramebuffer, lightingModel,
        surfaceGeometryFramebuffer, ambientOcclusionFramebuffer, scatteringResource, lightClusters).hasVarying();
    
    addJob<RenderDeferred>("RenderDeferred", deferredLightingInputs);

    // Use Stencil and draw background in Lighting buffer to complete filling in the opaque
    const auto backgroundInputs = DrawBackgroundDeferred::Inputs(background, lightingModel).hasVarying();
    addJob<DrawBackgroundDeferred>("DrawBackgroundDeferred", backgroundInputs);
   
    
    // Render transparent objects forward in LightingBuffer
    const auto transparentsInputs = DrawDeferred::Inputs(transparents, lightingModel).hasVarying();
    addJob<DrawDeferred>("DrawTransparentDeferred", transparentsInputs, shapePlumber);

    // LIght Cluster Grid Debuging job
    {
        const auto debugLightClustersInputs = DebugLightClusters::Inputs(deferredFrameTransform, deferredFramebuffer, lightingModel, linearDepthTarget, lightClusters).hasVarying();
        addJob<DebugLightClusters>("DebugLightClusters", debugLightClustersInputs);
    }
    
    const auto toneAndPostRangeTimer = addJob<BeginGPURangeTimer>("BeginToneAndPostRangeTimer", "PostToneOverlaysAntialiasing");

    // Lighting Buffer ready for tone mapping
    const auto toneMappingInputs = render::Varying(ToneMappingDeferred::Inputs(lightingFramebuffer, primaryFramebuffer));
    addJob<ToneMappingDeferred>("ToneMapping", toneMappingInputs);

    // Overlays
    const auto overlayOpaquesInputs = DrawOverlay3D::Inputs(overlayOpaques, lightingModel).hasVarying();
    const auto overlayTransparentsInputs = DrawOverlay3D::Inputs(overlayTransparents, lightingModel).hasVarying();
    addJob<DrawOverlay3D>("DrawOverlay3DOpaque", overlayOpaquesInputs, true);
    addJob<DrawOverlay3D>("DrawOverlay3DTransparent", overlayTransparentsInputs, false);

    
    // Debugging stages
    {
        // Debugging Deferred buffer job
        const auto debugFramebuffers = render::Varying(DebugDeferredBuffer::Inputs(deferredFramebuffer, linearDepthTarget, surfaceGeometryFramebuffer, ambientOcclusionFramebuffer));
        addJob<DebugDeferredBuffer>("DebugDeferredBuffer", debugFramebuffers);

        const auto debugSubsurfaceScatteringInputs = DebugSubsurfaceScattering::Inputs(deferredFrameTransform, deferredFramebuffer, lightingModel,
            surfaceGeometryFramebuffer, ambientOcclusionFramebuffer, scatteringResource).hasVarying();
        addJob<DebugSubsurfaceScattering>("DebugScattering", debugSubsurfaceScatteringInputs);

        const auto debugAmbientOcclusionInputs = DebugAmbientOcclusion::Inputs(deferredFrameTransform, deferredFramebuffer, linearDepthTarget, ambientOcclusionUniforms).hasVarying();
        addJob<DebugAmbientOcclusion>("DebugAmbientOcclusion", debugAmbientOcclusionInputs);


        // Scene Octree Debuging job
        {
            addJob<DrawSceneOctree>("DrawSceneOctree", spatialSelection);
            addJob<DrawItemSelection>("DrawItemSelection", spatialSelection);
        }

        // Status icon rendering job
        {
            // Grab a texture map representing the different status icons and assign that to the drawStatsuJob
            auto iconMapPath = PathUtils::resourcesPath() + "icons/statusIconAtlas.svg";
            auto statusIconMap = DependencyManager::get<TextureCache>()->getImageTexture(iconMapPath);
            addJob<DrawStatus>("DrawStatus", opaques, DrawStatus(statusIconMap));
        }
    }


    // AA job to be revisited
    addJob<Antialiasing>("Antialiasing", primaryFramebuffer);

    addJob<EndGPURangeTimer>("ToneAndPostRangeTimer", toneAndPostRangeTimer);

    // Blit!
    addJob<Blit>("Blit", primaryFramebuffer);

 //   addJob<EndGPURangeTimer>("RangeTimer", fullFrameRangeTimer);

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
