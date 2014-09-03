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
        _needsInitialSimulation(true),
        _needsModelReload(true),
        _myRenderer(NULL) { }

    virtual ~RenderableModelEntityItem();

    virtual bool setProperties(const EntityItemProperties& properties, bool forceCopy);
    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData);
                                                
    virtual void somethingChangedNotification() { _needsInitialSimulation = true; }

    virtual void render(RenderArgs* args);
    Model* getModel(EntityTreeRenderer* renderer);
private:
    bool needsSimulation() const;
    
    Model* _model;
    bool _needsInitialSimulation;
    bool _needsModelReload;
    EntityTreeRenderer* _myRenderer;
};

#endif // hifi_RenderableModelEntityItem_h
