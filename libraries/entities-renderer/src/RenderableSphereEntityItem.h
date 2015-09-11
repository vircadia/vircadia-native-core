//
//  RenderableSphereEntityItem.h
//  interface/src/entities
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableSphereEntityItem_h
#define hifi_RenderableSphereEntityItem_h

#include <SphereEntityItem.h>
#include <procedural/Procedural.h>

#include "RenderableEntityItem.h"

class RenderableSphereEntityItem : public SphereEntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    RenderableSphereEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        SphereEntityItem(entityItemID, properties)
        { }

    virtual void render(RenderArgs* args);
    virtual void setUserData(const QString& value);

    SIMPLE_RENDERABLE();

private:
    QSharedPointer<Procedural> _procedural;
};


#endif // hifi_RenderableSphereEntityItem_h
