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
#include <procedural/Procedural.h>

#include "RenderableEntityItem.h"

class RenderableBoxEntityItem : public BoxEntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
    RenderableBoxEntityItem(const EntityItemID& entityItemID) : BoxEntityItem(entityItemID) { }

    virtual void render(RenderArgs* args) override;
    virtual void setUserData(const QString& value) override;

    SIMPLE_RENDERABLE()
private:
    QSharedPointer<Procedural> _procedural;
};


#endif // hifi_RenderableBoxEntityItem_h
