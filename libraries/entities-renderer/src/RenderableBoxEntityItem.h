//
//  RenderableBoxEntityItem.h
//  libraries/entities-renderer/src/
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableBoxEntityItem_h
#define hifi_RenderableBoxEntityItem_h

#include <BoxEntityItem.h>
#include "RenderableEntityItem.h"
#include "RenderableProceduralItem.h"

class RenderableBoxEntityItem : public BoxEntityItem, RenderableProceduralItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    RenderableBoxEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        BoxEntityItem(entityItemID, properties)
        { }

    virtual void render(RenderArgs* args);
    virtual void setUserData(const QString& value);

    SIMPLE_RENDERABLE()
};


#endif // hifi_RenderableBoxEntityItem_h
