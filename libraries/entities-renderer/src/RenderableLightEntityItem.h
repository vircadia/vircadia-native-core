//
//  RenderableLightEntityItem.h
//  interface/src/entities
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableLightEntityItem_h
#define hifi_RenderableLightEntityItem_h

#include <LightEntityItem.h>
#include "RenderableEntityItem.h"

class RenderableLightEntityItem : public LightEntityItem  {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    RenderableLightEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        LightEntityItem(entityItemID, properties)
        { }

    virtual void render(RenderArgs* args);
    virtual bool supportsDetailedRayIntersection() const { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElementPointer& element, float& distance, BoxFace& face, 
                         void** intersectedObject, bool precisionPicking) const;

    SIMPLE_RENDERABLE();
};


#endif // hifi_RenderableLightEntityItem_h
