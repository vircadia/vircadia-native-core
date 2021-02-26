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

void FilterLayeredItems::run(const RenderContextPointer& renderContext, const ItemBounds& inItems, Outputs& outputs) {
    auto& scene = renderContext->_scene;

    ItemBounds matchedItems;
    ItemBounds nonMatchItems;

    // For each item, filter it into one bucket
    for (auto& itemBound : inItems) {
        auto& item = scene->getItem(itemBound.id);
        if (item.getLayer() == _keepLayer) {
            matchedItems.emplace_back(itemBound);
        } else {
            nonMatchItems.emplace_back(itemBound);
        }
    }

    outputs.edit0() = matchedItems;
    outputs.edit1() = nonMatchItems;
}

void SliceItems::run(const RenderContextPointer& renderContext, const ItemBounds& inItems, ItemBounds& outItems) {
    outItems.clear();
    std::static_pointer_cast<Config>(renderContext->jobConfig)->setNumItems((int)inItems.size());

    if (_rangeOffset < 0) return;

    int maxItemNum = std::min(_rangeOffset + _rangeLength, (int)inItems.size());


    for (int i = _rangeOffset; i < maxItemNum; i++) {
        outItems.emplace_back(inItems[i]);
    }

}

void SelectItems::run(const RenderContextPointer& renderContext, const Inputs& inputs, ItemBounds& outItems) {
    auto selectionName{ _name };
    if (!inputs.get2().empty()) {
        selectionName = inputs.get2();
    }

    auto selection = renderContext->_scene->getSelection(selectionName);
    const auto& selectedItems = selection.getItems();
    const auto& inItems = inputs.get0();
    const auto itemsToAppend = inputs[1];

    if (itemsToAppend.isNull()) {
        outItems.clear();
    } else {
        outItems = itemsToAppend.get<ItemBounds>();
    }

    if (!selectedItems.empty()) {
        outItems.reserve(outItems.size()+selectedItems.size());

        for (auto src : inItems) {
            if (selection.contains(src.id)) {
                outItems.emplace_back(src);
            }
        }
    }
}

void SelectSortItems::run(const RenderContextPointer& renderContext, const ItemBounds& inItems, ItemBounds& outItems) {
    auto selection = renderContext->_scene->getSelection(_name);
    const auto& selectedItems = selection.getItems();
    outItems.clear();

    if (!selectedItems.empty()) {
        struct Pair { int src; int dst; };
        std::vector<Pair> indices;
        indices.reserve(selectedItems.size());

        // Collect
        for (int srcIndex = 0; ((std::size_t) srcIndex < inItems.size()) && (indices.size() < selectedItems.size()) ; srcIndex++ ) {
            int index = selection.find(inItems[srcIndex].id);
            if (index != Selection::NOT_FOUND) {
                indices.emplace_back( Pair{ srcIndex, index } );
            }
        }

        // Then sort
        if (!indices.empty()) {
            std::sort(indices.begin(), indices.end(), [] (Pair a, Pair b) { 
                return (a.dst < b.dst);
            });

            for (auto& pair: indices) {
               outItems.emplace_back(inItems[pair.src]);
            }
        }
    }
}

void MetaToSubItems::run(const RenderContextPointer& renderContext, const ItemBounds& inItems, ItemIDs& outItems) {
    auto& scene = renderContext->_scene;

    // Now we have a selection of items to render
    outItems.clear();

    for (auto idBound : inItems) {
        auto& item = scene->getItem(idBound.id);
        if (item.exist()) {
            item.fetchMetaSubItems(outItems);
        }
    }
}

void IDsToBounds::run(const RenderContextPointer& renderContext, const ItemIDs& inItems, ItemBounds& outItems) {
    auto& scene = renderContext->_scene;

    // Now we have a selection of items to render
    outItems.clear();

    if (!_disableAABBs) {
        for (auto id : inItems) {
            auto& item = scene->getItem(id);
            if (item.exist()) {
                outItems.emplace_back(ItemBound{ id, item.getBound(renderContext->args) });
            }
        }
    } else {
        for (auto id : inItems) {
            outItems.emplace_back(ItemBound{ id });
        }
    }
}
