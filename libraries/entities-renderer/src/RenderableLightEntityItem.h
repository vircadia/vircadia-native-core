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
#include <model/Light.h>
#include "RenderableEntityItem.h"

class RenderableLightEntityItem;

class LightRenderItem {
public:
    using Payload = render::Payload<LightRenderItem>;
    using Pointer = Payload::DataPointer;

    model::LightPointer _light;

    render::Item::Bound _bound;

    LightRenderItem();
    void render(RenderArgs* args);

    void updateLightFromEntity(RenderableLightEntityItem* entity);

};

namespace render {
    template <> const ItemKey payloadGetKey(const LightRenderItem::Pointer& payload);
    template <> const Item::Bound payloadGetBound(const LightRenderItem::Pointer& payload);
    template <> void payloadRender(const LightRenderItem::Pointer& payload, RenderArgs* args);
}

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

    virtual bool addToScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges) override {
        _myItem = scene->allocateID();

        auto renderItem = std::make_shared<LightRenderItem>();
        renderItem->updateLightFromEntity(this);

        auto renderPayload = std::make_shared<LightRenderItem::Payload>(renderItem);

        render::Item::Status::Getters statusGetters;
        makeEntityItemStatusGetters(self, statusGetters);
        renderPayload->addStatusGetters(statusGetters);

        pendingChanges.resetItem(_myItem, renderPayload);

        return true;
    }

    virtual void removeFromScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges) override {
        pendingChanges.removeItem(_myItem);
        render::Item::clearID(_myItem);
    }

    virtual void locationChanged(bool tellPhysics = true) override {
        EntityItem::locationChanged(tellPhysics);
        notifyChanged();
    }

    virtual void dimensionsChanged() override {
        EntityItem::dimensionsChanged();
        notifyChanged();
    }

    void checkFading() {
        bool transparent = isTransparent();
        if (transparent != _prevIsTransparent) {
            notifyChanged();
            _isFading = false;
            _prevIsTransparent = transparent;
        }
    }

    void notifyChanged() {

        if (!render::Item::isValidID(_myItem)) {
            return;
        }

        render::PendingChanges pendingChanges;
        render::ScenePointer scene = AbstractViewStateInterface::instance()->getMain3DScene();

        updateLightFromEntity(pendingChanges);

        scene->enqueuePendingChanges(pendingChanges);
    }

private:
    bool _prevIsTransparent { isTransparent() };
    render::ItemID _myItem { render::Item::INVALID_ITEM_ID };



};


#endif // hifi_RenderableLightEntityItem_h
