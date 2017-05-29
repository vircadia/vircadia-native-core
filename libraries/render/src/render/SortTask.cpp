//
//  SortTask.cpp
//  render/src/render
//
//  Created by Sam Gateau on 2/2/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SortTask.h"
#include "ShapePipeline.h"

#include <assert.h>
#include <ViewFrustum.h>

using namespace render;

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

void render::depthSortItems(const RenderContextPointer& renderContext, bool frontToBack, const ItemBounds& inItems, ItemBounds& outItems) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    auto& scene = renderContext->_scene;
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
        float distance = args->getViewFrustum().distanceToCamera(bound.calcCenter());

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

    // Finally once sorted result to a list of itemID
    for (auto& item : itemBoundSorts) {
       outItems.emplace_back(ItemBound(item._id, item._bounds));
    }
}

void PipelineSortShapes::run(const RenderContextPointer& renderContext, const ItemBounds& inItems, ShapeBounds& outShapes) {
    auto& scene = renderContext->_scene;
    outShapes.clear();

    for (const auto& item : inItems) {
        auto key = scene->getItem(item.id).getShapeKey();
        auto outItems = outShapes.find(key);
        if (outItems == outShapes.end()) {
            outItems = outShapes.insert(std::make_pair(key, ItemBounds{})).first;
            outItems->second.reserve(inItems.size());
        }

        outItems->second.push_back(item);
    }

    for (auto& items : outShapes) {
        items.second.shrink_to_fit();
    }
}

void DepthSortShapes::run(const RenderContextPointer& renderContext, const ShapeBounds& inShapes, ShapeBounds& outShapes) {
    outShapes.clear();
    outShapes.reserve(inShapes.size());

    for (auto& pipeline : inShapes) {
        auto& inItems = pipeline.second;
        auto outItems = outShapes.find(pipeline.first);
        if (outItems == outShapes.end()) {
            outItems = outShapes.insert(std::make_pair(pipeline.first, ItemBounds{})).first;
        }

        depthSortItems(renderContext, _frontToBack, inItems, outItems->second);
    }
}

void DepthSortItems::run(const RenderContextPointer& renderContext, const ItemBounds& inItems, ItemBounds& outItems) {
    depthSortItems(renderContext, _frontToBack, inItems, outItems);
}
