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
#include "RenderableDebugableEntityItem.h"

class RenderableLineEntityItem : public LineEntityItem {
public:
    static EntityItem* factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    RenderableLineEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        LineEntityItem(entityItemID, properties) { }

    virtual void render(RenderArgs* args);
};


#endif // hifi_RenderableLineEntityItem_h
