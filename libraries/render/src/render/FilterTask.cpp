//
//  FilterTask.cpp
//  render/src/render
//
//  Created by Sam Gateau on 2/2/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FilterTask.h"

#include <algorithm>
#include <assert.h>

#include <OctreeUtils.h>
#include <PerfStat.h>
#include <ViewFrustum.h>
#include <gpu/Context.h>

using namespace render;

void FilterLayeredItems::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inItems, ItemBounds& outItems) {
    auto& scene = sceneContext->_scene;

    // Clear previous values
    outItems.clear();

    // For each item, filter it into one bucket
    for (auto itemBound : inItems) {
        auto& item = scene->getItem(itemBound.id);
        if (item.getLayer() == _keepLayer) {
            outItems.emplace_back(itemBound);
        }
    }
}

void SliceItems::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inItems, ItemBounds& outItems) {
    auto& scene = sceneContext->_scene;

    // Now we have a selection of items to render
    outItems.clear();
    std::static_pointer_cast<Config>(renderContext->jobConfig)->setNumItems((int)inItems.size());

    if (_rangeOffset < 0) return;

    int maxItemNum = std::min(_rangeOffset + _rangeLength, (int)inItems.size());


    for (int i = _rangeOffset; i < maxItemNum; i++) {
        outItems.emplace_back(inItems[i]);
    }

}

void SelectItems::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inItems, ItemBounds& outItems) {
    auto& scene = sceneContext->_scene;

    outItems.clear();
    auto selection = scene->getSelection(_name);
    const auto& selectedItems = selection.getItems();
    if (!selectedItems.empty()) {
        for (auto src : inItems) {
            if (selection.contains(src.id)) {
                outItems.emplace_back(src);
            }
        }
    }
}

void MetaToSubItems::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inItems, ItemIDs& outItems) {
    auto& scene = sceneContext->_scene;

    // Now we have a selection of items to render
    outItems.clear();

    for (auto idBound : inItems) {
        auto& item = scene->getItem(idBound.id);

        item.fetchMetaSubItems(outItems);
    }
}

