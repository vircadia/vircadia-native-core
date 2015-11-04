//
//  CameraEntityItem.cpp
//  libraries/entities/src
//
//  Created by Thijs Wenker on 11/4/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntitiesLogging.h"
#include "EntityItemID.h"
#include "EntityItemProperties.h"
#include "CameraEntityItem.h"


EntityItemPointer CameraEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer result { new CameraEntityItem(entityID, properties) };
    return result;
}

// our non-pure virtual subclass for now...
CameraEntityItem::CameraEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID) 
{
    _type = EntityTypes::Camera;

    setProperties(properties);
}
