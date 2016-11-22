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

#ifndef hifi_RenderableLineEntityItem_h
#define hifi_RenderableLineEntityItem_h

#include <LineEntityItem.h>
#include "RenderableEntityItem.h"
#include <GeometryCache.h>

class RenderableLineEntityItem : public LineEntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
    RenderableLineEntityItem(const EntityItemID& entityItemID) :
        LineEntityItem(entityItemID),
       _lineVerticesID(GeometryCache::UNKNOWN_ID)
    { }
    ~RenderableLineEntityItem();

    virtual void render(RenderArgs* args) override;

    SIMPLE_RENDERABLE();

protected:
    void updateGeometry();
    
    int _lineVerticesID;
};


#endif // hifi_RenderableLineEntityItem_h
