//
//  RenderableLineEntityItem.h
//  libraries/entities-renderer/src/
//
//  Created by Seth Alves on 5/11/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableQuadEntityItem_h
#define hifi_RenderableQuadEntityItem_h

#include <QuadEntityItem.h>
#include "RenderableDebugableEntityItem.h"
#include "RenderableEntityItem.h"
#include <GeometryCache.h>

class RenderableQuadEntityItem : public QuadEntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
    
    RenderableQuadEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
    QuadEntityItem(entityItemID, properties),
    _lineVerticesID(GeometryCache::UNKNOWN_ID)
    { }
    
    virtual void render(RenderArgs* args);
    
    SIMPLE_RENDERABLE();
    
protected:
    void updateGeometry();
    
    int _lineVerticesID;
};


#endif // hifi_RenderableQuadEntityItem_h
