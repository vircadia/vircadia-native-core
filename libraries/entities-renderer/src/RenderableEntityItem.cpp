//
//  RenderableEntityItem.cpp
//  interface/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "RenderableEntityItem.h"

namespace render {
    template <> const ItemKey payloadGetKey(const RenderableEntityItemProxy::Pointer& payload) { 
        if (payload && payload->entity) {
            if (payload->entity->getType() == EntityTypes::Light) {
                return ItemKey::Builder::light();
            }
            if (payload && payload->entity->getType() == EntityTypes::PolyLine) {
                return ItemKey::Builder::transparentShape();
            }
        }
        return ItemKey::Builder::opaqueShape();
    }
    
    template <> const Item::Bound payloadGetBound(const RenderableEntityItemProxy::Pointer& payload) { 
        if (payload && payload->entity) {
            return payload->entity->getAABox();
        }
        return render::Item::Bound();
    }
    template <> void payloadRender(const RenderableEntityItemProxy::Pointer& payload, RenderArgs* args) {
        if (args) {
            if (payload && payload->entity && payload->entity->getVisible()) {
                payload->entity->render(args);
            }
        }
    }
}



