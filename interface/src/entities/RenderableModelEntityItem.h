//
//  RenderableModelEntityItem.h
//  interface/src/entities
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableModelEntityItem_h
#define hifi_RenderableModelEntityItem_h

#include <glm/glm.hpp>
#include <stdint.h>

#include <EntityTree.h>
#include <Octree.h>
#include <OctreePacketData.h>
#include <OctreeRenderer.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <ViewFrustum.h>

#include "renderer/Model.h"

#include <ModelEntityItem.h>
#include <BoxEntityItem.h>

class RenderableModelEntityItem : public ModelEntityItem  {
public:
    static EntityItem* factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    RenderableModelEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        ModelEntityItem(entityItemID, properties),
        _model(NULL),
        _needsSimulation(true),
        _needsModelReload(true) { 
        
qDebug() << "*********** RenderableModelEntityItem -- ENTITY ITEM BEING CREATED ************* this=" << this;
        
        };

    virtual ~RenderableModelEntityItem() { 
qDebug() << "*********** RenderableModelEntityItem -- ENTITY ITEM BEING DELETED ************* this=" << this;
        };

    virtual bool setProperties(const EntityItemProperties& properties, bool forceCopy);
    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData);
                                                
    virtual void somethingChangedNotification() { _needsSimulation = true; }

    virtual void render(RenderArgs* args);
    Model* getModel();
private:
    Model* _model;
    bool _needsSimulation;
    bool _needsModelReload;
};

#endif // hifi_RenderableModelEntityItem_h
