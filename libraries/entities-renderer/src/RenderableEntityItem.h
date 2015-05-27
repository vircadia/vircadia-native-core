//
//  RenderableEntityItem.h
//  interface/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableEntityItem_h
#define hifi_RenderableEntityItem_h

#include <render/Scene.h>
#include <EntityItem.h>

class RenderableEntityItem {
public:
    RenderableEntityItem(EntityItemPointer entity) : entity(entity) { }
    typedef render::Payload<RenderableEntityItem> Payload;
    typedef Payload::DataPointer Pointer;
    
    EntityItemPointer entity;
};

namespace render {
   template <> const ItemKey payloadGetKey(const RenderableEntityItem::Pointer& payload); 
   template <> const Item::Bound payloadGetBound(const RenderableEntityItem::Pointer& payload);
   template <> void payloadRender(const RenderableEntityItem::Pointer& payload, RenderArgs* args);
}

#endif // hifi_RenderableEntityItem_h
