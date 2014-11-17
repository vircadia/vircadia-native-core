//
//  RenderableTextEntityItem.h
//  interface/src/entities
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableTextEntityItem_h
#define hifi_RenderableTextEntityItem_h

#include <glm/glm.hpp>
#include <stdint.h>

#include <EntityTree.h>
#include <Octree.h>
#include <OctreePacketData.h>
#include <OctreeRenderer.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <ViewFrustum.h>

#include <TextEntityItem.h>

class RenderableTextEntityItem : public TextEntityItem  {
public:
    static EntityItem* factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    RenderableTextEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        TextEntityItem(entityItemID, properties)
        { }

    virtual void render(RenderArgs* args);

private:
    void enableClipPlane(GLenum plane, float x, float y, float z, float w);

};


#endif // hifi_RenderableTextEntityItem_h
