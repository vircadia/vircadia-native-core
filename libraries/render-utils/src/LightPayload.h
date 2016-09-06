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


class LightPayload {
public:
    using Payload = render::Payload<LightPayload>;
    using Pointer = Payload::DataPointer;

    LightPayload();
    void render(RenderArgs* args);

    model::LightPointer editLight() { return _light; }
    render::Item::Bound& editBound() { return _bound; }

protected:
    model::LightPointer _light;
    render::Item::Bound _bound;
};

namespace render {
    template <> const ItemKey payloadGetKey(const LightPayload::Pointer& payload);
    template <> const Item::Bound payloadGetBound(const LightPayload::Pointer& payload);
    template <> void payloadRender(const LightPayload::Pointer& payload, RenderArgs* args);
}

#endif