//
//  RenderablePolyVoxEntityItem.h
//  libraries/entities-renderer/src/
//
//  Created by Seth Alves on 5/19/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderablePolyVoxEntityItem_h
#define hifi_RenderablePolyVoxEntityItem_h

#include <PolyVoxCore/SimpleVolume.h>

#include "PolyVoxEntityItem.h"
#include "RenderableDebugableEntityItem.h"

class RenderablePolyVoxEntityItem : public PolyVoxEntityItem {
public:
    static EntityItem* factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    RenderablePolyVoxEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        PolyVoxEntityItem(entityItemID, properties) { }

    void render(RenderArgs* args);
    virtual bool hasModel() const { return true; }
    void getModel();

private:
    // Model* _model = nullptr;
    // bool _needsInitialSimulation = true;
    // bool _needsModelReload = true;
    // EntityTreeRenderer* _myRenderer = nullptr;

    model::Geometry _modelGeometry;
    bool _needsModelReload = true;
};


#endif // hifi_RenderablePolyVoxEntityItem_h
