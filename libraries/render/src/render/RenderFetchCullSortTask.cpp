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
#include "SortTask.h"

using namespace render;

RenderFetchCullSortTask::RenderFetchCullSortTask(CullFunctor cullFunctor) {
    cullFunctor = cullFunctor ? cullFunctor : [](const RenderArgs*, const AABox&){ return true; };

    // CPU jobs:
    // Fetch and cull the items from the scene
    auto spatialFilter = ItemFilter::Builder::visibleWorldItems().withoutLayered();
    const auto spatialSelection = addJob<FetchSpatialTree>("FetchSceneSelection", spatialFilter);
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

    // Extract opaques / transparents / lights / overlays
    const auto opaques = addJob<DepthSortItems>("DepthSortOpaque", filteredSpatialBuckets[OPAQUE_SHAPE_BUCKET]);
    const auto transparents = addJob<DepthSortItems>("DepthSortTransparent", filteredSpatialBuckets[TRANSPARENT_SHAPE_BUCKET], DepthSortItems(false));
    const auto lights = filteredSpatialBuckets[LIGHT_BUCKET];

    const auto overlayOpaques = addJob<DepthSortItems>("DepthSortOverlayOpaque", filteredNonspatialBuckets[OPAQUE_SHAPE_BUCKET]);
    const auto overlayTransparents = addJob<DepthSortItems>("DepthSortOverlayTransparent", filteredNonspatialBuckets[TRANSPARENT_SHAPE_BUCKET], DepthSortItems(false));
    const auto background = filteredNonspatialBuckets[BACKGROUND_BUCKET];

    setOutput(Output{{
            opaques, transparents, lights, overlayOpaques, overlayTransparents, background, spatialSelection }});
}
