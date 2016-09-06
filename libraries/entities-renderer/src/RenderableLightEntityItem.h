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
#include <LightPayload.h>
#include "RenderableEntityItem.h"


class RenderableLightEntityItem : public LightEntityItem  {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
    RenderableLightEntityItem(const EntityItemID& entityItemID);

    virtual bool supportsDetailedRayIntersection() const override { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElementPointer& element, float& distance, 
                         BoxFace& face, glm::vec3& surfaceNormal,
                         void** intersectedObject, bool precisionPicking) const override;

    void updateLightFromEntity(render::PendingChanges& pendingChanges);

    virtual bool addToScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges) override;

    virtual void somethingChangedNotification() override;
    virtual void removeFromScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges) override;

    virtual void locationChanged(bool tellPhysics = true) override;

    virtual void dimensionsChanged() override;

    void checkFading();

    void notifyChanged();

private:
    bool _prevIsTransparent { isTransparent() };
    render::ItemID _myItem { render::Item::INVALID_ITEM_ID };

    // Dirty flag turn true when either setSubClassProperties or readEntitySubclassDataFromBuffer is changing a value 

    void updateRenderItemFromEntity(LightPayload* lightPayload);

};


#endif // hifi_RenderableLightEntityItem_h
