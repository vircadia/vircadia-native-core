//
//  CullTask.cpp
//  render/src/render
//
//  Created by Sam Gateau on 2/2/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CullTask.h"

#include <algorithm>
#include <assert.h>

#include <PerfStat.h>
#include <ViewFrustum.h>
#include <gpu/Context.h>

using namespace render;

void render::cullItems(const RenderContextPointer& renderContext, const CullFunctor& cullFunctor, RenderDetails::Item& details,
                       const ItemBounds& inItems, ItemBounds& outItems) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);

    RenderArgs* args = renderContext->args;
    ViewFrustum* frustum = args->_viewFrustum;

    details._considered += inItems.size();
    
    // Culling / LOD
    for (auto item : inItems) {
        if (item.bound.isNull()) {
            outItems.emplace_back(item); // One more Item to render
            continue;
        }

        // TODO: some entity types (like lights) might want to be rendered even
        // when they are outside of the view frustum...
        bool outOfView;
        {
            PerformanceTimer perfTimer("boxInFrustum");
            outOfView = frustum->boxInFrustum(item.bound) == ViewFrustum::OUTSIDE;
        }
        if (!outOfView) {
            bool bigEnoughToRender;
            {
                PerformanceTimer perfTimer("shouldRender");
                bigEnoughToRender = cullFunctor(args, item.bound);
            }
            if (bigEnoughToRender) {
                outItems.emplace_back(item); // One more Item to render
            } else {
                details._tooSmall++;
            }
        } else {
            details._outOfView++;
        }
    }
    details._rendered += outItems.size();
}

struct ItemBoundSort {
    float _centerDepth = 0.0f;
    float _nearDepth = 0.0f;
    float _farDepth = 0.0f;
    ItemID _id = 0;
    AABox _bounds;

    ItemBoundSort() {}
    ItemBoundSort(float centerDepth, float nearDepth, float farDepth, ItemID id, const AABox& bounds) : _centerDepth(centerDepth), _nearDepth(nearDepth), _farDepth(farDepth), _id(id), _bounds(bounds) {}
};

struct FrontToBackSort {
    bool operator() (const ItemBoundSort& left, const ItemBoundSort& right) {
        return (left._centerDepth < right._centerDepth);
    }
};

struct BackToFrontSort {
    bool operator() (const ItemBoundSort& left, const ItemBoundSort& right) {
        return (left._centerDepth > right._centerDepth);
    }
};

void render::depthSortItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, bool frontToBack, const ItemBounds& inItems, ItemBounds& outItems) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);
    
    auto& scene = sceneContext->_scene;
    RenderArgs* args = renderContext->args;
    

    // Allocate and simply copy
    outItems.clear();
    outItems.reserve(inItems.size());


    // Make a local dataset of the center distance and closest point distance
    std::vector<ItemBoundSort> itemBoundSorts;
    itemBoundSorts.reserve(outItems.size());

    for (auto itemDetails : inItems) {
        auto item = scene->getItem(itemDetails.id);
        auto bound = itemDetails.bound; // item.getBound();
        float distance = args->_viewFrustum->distanceToCamera(bound.calcCenter());

        itemBoundSorts.emplace_back(ItemBoundSort(distance, distance, distance, itemDetails.id, bound));
    }

    // sort against Z
    if (frontToBack) {
        FrontToBackSort frontToBackSort;
        std::sort(itemBoundSorts.begin(), itemBoundSorts.end(), frontToBackSort);
    } else {
        BackToFrontSort  backToFrontSort;
        std::sort(itemBoundSorts.begin(), itemBoundSorts.end(), backToFrontSort);
    }

    // FInally once sorted result to a list of itemID
    for (auto& item : itemBoundSorts) {
       outItems.emplace_back(ItemBound(item._id, item._bounds));
    }
}

void DepthSortItems::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inItems, ItemBounds& outItems) {
    depthSortItems(sceneContext, renderContext, _frontToBack, inItems, outItems);
}


void FetchItems::configure(const Config& config) {
    _justFrozeFrustum = (config.freezeFrustum && !_freezeFrustum);
    _freezeFrustum = config.freezeFrustum;
}

void FetchItems::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, ItemBounds& outItems) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);
    RenderArgs* args = renderContext->args;
    auto& scene = sceneContext->_scene;

    outItems.clear();

   /* const auto& bucket = scene->getMasterBucket();
    const auto& items = bucket.find(_filter);
    if (items != bucket.end()) {
        outItems.reserve(items->second.size());
        for (auto& id : items->second) {
            auto& item = scene->getItem(id);
            outItems.emplace_back(ItemBound(id, item.getBound()));
        }
    }
    */

    auto queryFrustum = *args->_viewFrustum;
    if (_freezeFrustum) {
        if (_justFrozeFrustum) {
            _justFrozeFrustum = false;
            _frozenFrutstum = *args->_viewFrustum;
        }
        queryFrustum = _frozenFrutstum;
    }

    // Try that:
    ItemIDs fetchedItems;
    scene->getSpatialTree().fetch(fetchedItems, _filter, queryFrustum);

    // Now get the bound, and filter individually against the _filter
    outItems.reserve(fetchedItems.size());
    for (auto id : fetchedItems) {
        auto& item = scene->getItem(id);
        if (_filter.test(item.getKey())) {
            outItems.emplace_back(ItemBound(id, item.getBound()));
        }
    }

    std::static_pointer_cast<Config>(renderContext->jobConfig)->numItems = (int)outItems.size();
}
