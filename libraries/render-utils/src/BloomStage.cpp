//
//  BloomStage.cpp
//
//  Created by Sam Gondelman on 8/7/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "BloomStage.h"

#include "DeferredLightingEffect.h"

#include <gpu/Context.h>

std::string BloomStage::_stageName { "BLOOM_STAGE"};
const BloomStage::Index BloomStage::INVALID_INDEX { render::indexed_container::INVALID_INDEX };

FetchBloomStage::FetchBloomStage() {
    _bloom = std::make_shared<graphics::Bloom>();
}

void FetchBloomStage::configure(const Config& config) {
    _bloom->setBloomIntensity(config.bloomIntensity);
    _bloom->setBloomThreshold(config.bloomThreshold);
    _bloom->setBloomSize(config.bloomSize);
}

BloomStage::Index BloomStage::findBloom(const BloomPointer& bloom) const {
    auto found = _bloomMap.find(bloom);
    if (found != _bloomMap.end()) {
        return INVALID_INDEX;
    } else {
        return (*found).second;
    }
}

BloomStage::Index BloomStage::addBloom(const BloomPointer& bloom) {
    auto found = _bloomMap.find(bloom);
    if (found == _bloomMap.end()) {
        auto bloomId = _blooms.newElement(bloom);
        // Avoid failing to allocate a bloom, just pass
        if (bloomId != INVALID_INDEX) {
            // Insert the bloom and its index in the reverse map
            _bloomMap.insert(BloomMap::value_type(bloom, bloomId));
        }
        return bloomId;
    } else {
        return (*found).second;
    }
}

BloomStage::BloomPointer BloomStage::removeBloom(Index index) {
    BloomPointer removed = _blooms.freeElement(index);
    if (removed) {
        _bloomMap.erase(removed);
    }
    return removed;
}

BloomStageSetup::BloomStageSetup() {}

void BloomStageSetup::run(const render::RenderContextPointer& renderContext) {
    auto stage = renderContext->_scene->getStage(BloomStage::getName());
    if (!stage) {
        renderContext->_scene->resetStage(BloomStage::getName(), std::make_shared<BloomStage>());
    }
}

void FetchBloomStage::run(const render::RenderContextPointer& renderContext, graphics::BloomPointer& bloom) {
    auto bloomStage = renderContext->_scene->getStage<BloomStage>();
    assert(bloomStage);

    bloom = nullptr;
    if (bloomStage->_currentFrame._blooms.size() != 0) {
        auto bloomId = bloomStage->_currentFrame._blooms.front();
        bloom = bloomStage->getBloom(bloomId);
    }
}
