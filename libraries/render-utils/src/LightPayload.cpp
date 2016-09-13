//
//  LightPayload.cpp
//
//  Created by Sam Gateau on 9/6/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "LightPayload.h"


#include <gpu/Batch.h>
#include "DeferredLightingEffect.h"


namespace render {
    template <> const ItemKey payloadGetKey(const LightPayload::Pointer& payload) {
        return ItemKey::Builder::light();
    }

    template <> const Item::Bound payloadGetBound(const LightPayload::Pointer& payload) {
        if (payload) {
            return payload->editBound();
        }
        return render::Item::Bound();
    }
    template <> void payloadRender(const LightPayload::Pointer& payload, RenderArgs* args) {
        if (args) {
            if (payload) {
                payload->render(args);
            }
        }
    }
}

LightPayload::LightPayload() :
    _light(std::make_shared<model::Light>())
{
}


LightPayload::~LightPayload() {
    if (!LightStage::isIndexInvalid(_index)) {
        if (_stage) {
            _stage->removeLight(_index);
        }
    }
}

void LightPayload::render(RenderArgs* args) {
    if (!_stage) {
        _stage = DependencyManager::get<DeferredLightingEffect>()->getLightStage();
    }
    // Do we need to allocate the light in the stage ?
    if (LightStage::isIndexInvalid(_index)) {
        _index = _stage->addLight(_light);
        _needUpdate = false;
    }
    // Need an update ?
    if (_needUpdate) {
        _stage->updateLightArrayBuffer(_index);
        _needUpdate = false;
    }
    
    // FInally, push the light visible in the frame
    _stage->_currentFrame.pushLight(_index, _light->getType());
}

