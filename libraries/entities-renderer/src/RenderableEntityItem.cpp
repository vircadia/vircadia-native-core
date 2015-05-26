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

// For Ubuntu, the compiler want's the Payload's functions to be specialized in the "render" namespace explicitely...
namespace render {
    template <> const ItemKey payloadGetKey(const RenderableEntityItem::Pointer& entity) { 
        return ItemKey::Builder::opaqueShape(); 
    }
    
    template <> const Item::Bound payloadGetBound(const RenderableEntityItem::Pointer& entity) { 
        if (entity) {
            return entity->getAABox();
        }
        return render::Item::Bound();
    }
    
    template <> void payloadRender(const RenderableEntityItem::Pointer& entity, RenderArgs* args) {
        if (args) {
            args->_elementsTouched++;
            if (entity) {
                entity->render(args);
            }
        }
    }
}
