//
//  RenderFetchCullSortTask.cpp
//  render/src/
//
//  Created by Zach Pomerantz on 12/22/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderFetchCullSortTask.h"

#include "CullTask.h"
#include "FilterTask.h"
#include "SortTask.h"

using namespace render;

void RenderFetchCullSortTask::build(JobModel& task, const Varying& input, Varying& output, CullFunctor cullFunctor, uint8_t tagBits, uint8_t tagMask) {
    cullFunctor = cullFunctor ? cullFunctor : [](const RenderArgs*, const AABox&){ return true; };

    // CPU jobs:
    // Fetch and cull the items from the scene
    const ItemFilter filter = ItemFilter::Builder::visibleWorldItems().withoutLayered().withTagBits(tagBits, tagMask);
    const auto spatialFilter = render::Varying(filter);
    const auto fetchInput = FetchSpatialTree::Inputs(filter, glm::ivec2(0,0)).asVarying();
    const auto spatialSelection = task.addJob<FetchSpatialTree>("FetchSceneSelection", fetchInput);
    const auto cullInputs = CullSpatialSelection::Inputs(spatialSelection, spatialFilter).asVarying();
    const auto culledSpatialSelection = task.addJob<CullSpatialSelection>("CullSceneSelection", cullInputs, cullFunctor, false, RenderDetails::ITEM);

    // Layered objects are not culled
    const ItemFilter layeredFilter = ItemFilter::Builder::visibleWorldItems().withTagBits(tagBits, tagMask);
    const auto nonspatialFilter = render::Varying(layeredFilter);
    const auto nonspatialSelection = task.addJob<FetchNonspatialItems>("FetchLayeredSelection", nonspatialFilter);

    // Multi filter visible items into different buckets
    const int NUM_SPATIAL_FILTERS = 4; 
    const int NUM_NON_SPATIAL_FILTERS = 3;
    const int OPAQUE_SHAPE_BUCKET = 0;
    const int TRANSPARENT_SHAPE_BUCKET = 1;
    const int LIGHT_BUCKET = 2;
    const int META_BUCKET = 3;
    const int BACKGROUND_BUCKET = 2;
    MultiFilterItems<NUM_SPATIAL_FILTERS>::ItemFilterArray spatialFilters = { {
            ItemFilter::Builder::opaqueShape(),
            ItemFilter::Builder::transparentShape(),
            ItemFilter::Builder::light(),
            ItemFilter::Builder::meta()
        } };
    MultiFilterItems<NUM_NON_SPATIAL_FILTERS>::ItemFilterArray nonspatialFilters = { {
            ItemFilter::Builder::opaqueShape(),
            ItemFilter::Builder::transparentShape(),
            ItemFilter::Builder::background()
        } };
    const auto filteredSpatialBuckets = 
        task.addJob<MultiFilterItems<NUM_SPATIAL_FILTERS>>("FilterSceneSelection", culledSpatialSelection, spatialFilters)
            .get<MultiFilterItems<NUM_SPATIAL_FILTERS>::ItemBoundsArray>();
    const auto filteredNonspatialBuckets = 
       task.addJob<MultiFilterItems<NUM_NON_SPATIAL_FILTERS>>("FilterLayeredSelection", nonspatialSelection, nonspatialFilters)
            .get<MultiFilterItems<NUM_NON_SPATIAL_FILTERS>::ItemBoundsArray>();

    // Extract opaques / transparents / lights / layered
    const auto opaques = task.addJob<DepthSortItems>("DepthSortOpaque", filteredSpatialBuckets[OPAQUE_SHAPE_BUCKET]);
    const auto transparents = task.addJob<DepthSortItems>("DepthSortTransparent", filteredSpatialBuckets[TRANSPARENT_SHAPE_BUCKET], DepthSortItems(false));
    const auto lights = filteredSpatialBuckets[LIGHT_BUCKET];
    const auto metas = filteredSpatialBuckets[META_BUCKET];

    const auto background = filteredNonspatialBuckets[BACKGROUND_BUCKET];

    // split up the layered objects into 3D front, hud
    const auto layeredOpaques = task.addJob<DepthSortItems>("DepthSortLayaredOpaque", filteredNonspatialBuckets[OPAQUE_SHAPE_BUCKET]);
    const auto layeredTransparents = task.addJob<DepthSortItems>("DepthSortLayeredTransparent", filteredNonspatialBuckets[TRANSPARENT_SHAPE_BUCKET], DepthSortItems(false));
    const auto filteredLayeredOpaque = task.addJob<FilterLayeredItems>("FilterLayeredOpaque", layeredOpaques, ItemKey::Layer::LAYER_1);
    const auto filteredLayeredTransparent = task.addJob<FilterLayeredItems>("FilterLayeredTransparent", layeredTransparents, ItemKey::Layer::LAYER_1);

    task.addJob<ClearContainingZones>("ClearContainingZones");

    output = Output(BucketList{ opaques, transparents, lights, metas,
                    filteredLayeredOpaque.getN<FilterLayeredItems::Outputs>(0), filteredLayeredTransparent.getN<FilterLayeredItems::Outputs>(0),
                    filteredLayeredOpaque.getN<FilterLayeredItems::Outputs>(1), filteredLayeredTransparent.getN<FilterLayeredItems::Outputs>(1),
                    background }, spatialSelection);
}
