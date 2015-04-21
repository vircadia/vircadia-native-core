//
//  RenderableZoneEntityItem.h
//  interface/src/entities
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableZoneEntityItem_h
#define hifi_RenderableZoneEntityItem_h

#include <ZoneEntityItem.h>

class RenderableZoneEntityItem : public ZoneEntityItem  {
public:
    static EntityItem* factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    RenderableZoneEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        ZoneEntityItem(entityItemID, properties)
        { }

    virtual void render(RenderArgs* args);
};


#endif // hifi_RenderableZoneEntityItem_h
