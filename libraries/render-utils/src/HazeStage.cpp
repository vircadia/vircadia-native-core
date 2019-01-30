//
//  HazeStage.cpp
//
//  Created by Nissim Hadar on 9/26/2017.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "HazeStage.h"

#include "DeferredLightingEffect.h"

#include <gpu/Context.h>

std::string HazeStage::_stageName { "HAZE_STAGE"};
const HazeStage::Index HazeStage::INVALID_INDEX { render::indexed_container::INVALID_INDEX };

HazeStage::Index HazeStage::findHaze(const HazePointer& haze) const {
    auto found = _hazeMap.find(haze);
    if (found != _hazeMap.end()) {
        return INVALID_INDEX;
    } else {
        return (*found).second;
    }
}

HazeStage::Index HazeStage::addHaze(const HazePointer& haze) {
    auto found = _hazeMap.find(haze);
    if (found == _hazeMap.end()) {
        auto hazeId = _hazes.newElement(haze);
        // Avoid failing to allocate a haze, just pass
        if (hazeId != INVALID_INDEX) {

            // Insert the haze and its index in the reverse map
            _hazeMap.insert(HazeMap::value_type(haze, hazeId));
        }
        return hazeId;
    } else {
        return (*found).second;
    }
}

HazeStage::HazePointer HazeStage::removeHaze(Index index) {
    HazePointer removed = _hazes.freeElement(index);
    
    if (removed) {
        _hazeMap.erase(removed);
    }
    return removed;
}

HazeStageSetup::HazeStageSetup() {
}

void HazeStageSetup::run(const render::RenderContextPointer& renderContext) {
    auto stage = renderContext->_scene->getStage(HazeStage::getName());
    if (!stage) {
        renderContext->_scene->resetStage(HazeStage::getName(), std::make_shared<HazeStage>());
    }
}