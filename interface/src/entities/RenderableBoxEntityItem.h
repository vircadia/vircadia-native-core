//
//  RenderableBoxEntityItem.h
//  interface/src/entities
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableBoxEntityItem_h
#define hifi_RenderableBoxEntityItem_h

#include <glm/glm.hpp>
#include <stdint.h>

#include <EntityTree.h>
#include <Octree.h>
#include <OctreePacketData.h>
#include <OctreeRenderer.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <ViewFrustum.h>

#include <BoxEntityItem.h>

class RenderableBoxEntityItem : public BoxEntityItem  {
public:
    static EntityItem* factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    RenderableBoxEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        BoxEntityItem(entityItemID, properties)
        { };

    virtual void render(RenderArgs* args);
};


#endif // hifi_RenderableBoxEntityItem_h
