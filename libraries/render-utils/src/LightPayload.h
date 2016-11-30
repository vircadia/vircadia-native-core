//
//  LightPayload.h
//
//  Created by Sam Gateau on 9/6/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LightPayload_h
#define hifi_LightPayload_h


#include <model/Light.h>
#include <render/Item.h>
#include "LightStage.h"

class LightPayload {
public:
    using Payload = render::Payload<LightPayload>;
    using Pointer = Payload::DataPointer;

    LightPayload();
    ~LightPayload();
    void render(RenderArgs* args);

    model::LightPointer editLight() { _needUpdate = true; return _light; }
    render::Item::Bound& editBound() { _needUpdate = true; return _bound; }

    void setVisible(bool visible) { _isVisible = visible; }
    bool isVisible() const { return _isVisible; }

protected:
    model::LightPointer _light;
    render::Item::Bound _bound;
    LightStagePointer _stage;
    LightStage::Index _index { LightStage::INVALID_INDEX };
    bool _needUpdate { true };
    bool _isVisible{ true };
};

namespace render {
    template <> const ItemKey payloadGetKey(const LightPayload::Pointer& payload);
    template <> const Item::Bound payloadGetBound(const LightPayload::Pointer& payload);
    template <> void payloadRender(const LightPayload::Pointer& payload, RenderArgs* args);
}

#endif