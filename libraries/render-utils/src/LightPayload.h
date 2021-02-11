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


#include <graphics/Light.h>
#include <render/Item.h>
#include "LightStage.h"
#include "TextureCache.h"

class LightPayload {
public:
    using Payload = render::Payload<LightPayload>;
    using Pointer = Payload::DataPointer;

    LightPayload();
    ~LightPayload();
    void render(RenderArgs* args);

    graphics::LightPointer editLight() { _needUpdate = true; return _light; }
    render::Item::Bound& editBound() { _needUpdate = true; return _bound; }

    void setVisible(bool visible) { _isVisible = visible; }
    bool isVisible() const { return _isVisible; }

protected:
    graphics::LightPointer _light;
    render::Item::Bound _bound;
    LightStagePointer _stage;
    LightStage::Index _index { LightStage::INVALID_INDEX };
    bool _needUpdate { true };
    bool _isVisible{ true };
};

namespace render {
    template <> const ItemKey payloadGetKey(const LightPayload::Pointer& payload);
    template <> const Item::Bound payloadGetBound(const LightPayload::Pointer& payload, RenderArgs* args);
    template <> void payloadRender(const LightPayload::Pointer& payload, RenderArgs* args);
}

class KeyLightPayload {
public:
    using Payload = render::Payload<KeyLightPayload>;
    using Pointer = Payload::DataPointer;

    KeyLightPayload();
    ~KeyLightPayload();
    void render(RenderArgs* args);

    graphics::LightPointer editLight() { _needUpdate = true; return _light; }
    render::Item::Bound& editBound() { _needUpdate = true; return _bound; }

    void setVisible(bool visible) { _isVisible = visible; }
    bool isVisible() const { return _isVisible; }


    // More attributes used for rendering:
    NetworkTexturePointer _ambientTexture;
    QString _ambientTextureURL;
    bool _pendingAmbientTexture { false };

protected:
    graphics::LightPointer _light;
    render::Item::Bound _bound;
    LightStagePointer _stage;
    LightStage::Index _index { LightStage::INVALID_INDEX };
    bool _needUpdate { true };
    bool _isVisible { true };
};

namespace render {
    template <> const ItemKey payloadGetKey(const KeyLightPayload::Pointer& payload);
    template <> const Item::Bound payloadGetBound(const KeyLightPayload::Pointer& payload, RenderArgs* args);
    template <> void payloadRender(const KeyLightPayload::Pointer& payload, RenderArgs* args);
}

#endif