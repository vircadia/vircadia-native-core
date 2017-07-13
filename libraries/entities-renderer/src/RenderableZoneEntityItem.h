//
//  RenderableZoneEntityItem.h
//
//
//  Created by Clement on 4/22/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableZoneEntityItem_h
#define hifi_RenderableZoneEntityItem_h

#include <Model.h>
#include <ZoneEntityItem.h>
#include "RenderableEntityItem.h"

class NetworkGeometry;
class KeyLightPayload;

class RenderableZoneEntityItemMeta;

class RenderableZoneEntityItem : public ZoneEntityItem, public RenderableEntityInterface  {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
    
    RenderableZoneEntityItem(const EntityItemID& entityItemID) :
        ZoneEntityItem(entityItemID),
        _model(nullptr),
        _needsInitialSimulation(true)
    { }
    
    RenderableEntityInterface* getRenderableInterface() override { return this; }

    virtual bool setProperties(const EntityItemProperties& properties) override;
    virtual void somethingChangedNotification() override;

    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                 ReadBitstreamToTreeParams& args,
                                                 EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                 bool& somethingChanged) override;

    virtual void render(RenderArgs* args) override;
    virtual bool contains(const glm::vec3& point) const override;
    
    virtual bool addToScene(const EntityItemPointer& self, const render::ScenePointer& scene, render::Transaction& transaction) override;
    virtual void removeFromScene(const EntityItemPointer& self, const render::ScenePointer& scene, render::Transaction& transaction) override;
    
    render::ItemID getRenderItemID() const { return _myMetaItem; }
    
private:
    virtual void locationChanged(bool tellPhysics = true) override { EntityItem::locationChanged(tellPhysics); notifyBoundChanged(); }
    virtual void dimensionsChanged() override { EntityItem::dimensionsChanged(); notifyBoundChanged(); }
    void notifyBoundChanged();

    void updateGeometry();
    
    template<typename Lambda>
    void changeProperties(Lambda functor);

    void notifyChangedRenderItem();
    void sceneUpdateRenderItemFromEntity(render::Transaction& transaction);
    void updateKeyZoneItemFromEntity(RenderableZoneEntityItemMeta& keyZonePayload);

    void updateKeySunFromEntity(RenderableZoneEntityItemMeta& keyZonePayload);
    void updateKeyAmbientFromEntity(RenderableZoneEntityItemMeta& keyZonePayload);
    void updateKeyBackgroundFromEntity(RenderableZoneEntityItemMeta& keyZonePayload);

    ModelPointer _model;
    bool _needsInitialSimulation;

    render::ItemID _myMetaItem{ render::Item::INVALID_ITEM_ID };
};

#endif // hifi_RenderableZoneEntityItem_h
