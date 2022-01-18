//
//  WorldBox.h
//
//  Created by Sam Gateau on 01/07/2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_WorldBox_h
#define hifi_WorldBox_h

#include <PerfStat.h>

#include <gpu/Batch.h>
#include <render/Forward.h>

#include <render/Item.h>
#include <GeometryCache.h>
#include "Menu.h"

class WorldBoxRenderData {
public:
    typedef render::Payload<WorldBoxRenderData> Payload;
    typedef Payload::DataPointer Pointer;

    int _val = 0;
    static render::ItemID _item; // unique WorldBoxRenderData

    static void renderWorldBox(RenderArgs* args, gpu::Batch& batch);
};

namespace render {
    template <> const ItemKey payloadGetKey(const WorldBoxRenderData::Pointer& stuff);
    template <> const Item::Bound payloadGetBound(const WorldBoxRenderData::Pointer& stuff, RenderArgs* args);
    template <> void payloadRender(const WorldBoxRenderData::Pointer& stuff, RenderArgs* args);
}

#endif // hifi_WorldBox_h