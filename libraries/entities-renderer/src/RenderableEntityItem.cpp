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
    template <> const ItemKey payloadGetKey(const RenderableEntityItem::Pointer& payload) { 
        qDebug() << "payloadGetKey()... for payload:" << payload.get();
        ItemKey key = ItemKey::Builder::opaqueShape(); 
        qDebug() << "    key.isOpaque():" << key.isOpaque();
        return key;
    }
    
    template <> const Item::Bound payloadGetBound(const RenderableEntityItem::Pointer& payload) { 
        qDebug() << "payloadGetBound()... for payload:" << payload.get();
        if (payload && payload->entity) {
            return payload->entity->getAABox();
        }
        return render::Item::Bound();
    }
    template <> void payloadRender(const RenderableEntityItem::Pointer& payload, RenderArgs* args) {
        qDebug() << "payloadRender()... for payload:" << payload.get();
        if (args) {
            qDebug() << "rendering payload!! for payload:" << payload.get();
            qDebug() << "rendering payload!! for entity:" << payload->entity.get();
            args->_elementsTouched++;
            if (payload && payload->entity) {
                qDebug() << "rendering payload!! for entity:" << payload->entity->getEntityItemID();
                payload->entity->render(args);
            }
        }
    }
}
