//
//  RenderableEntityItem.h
//  interface/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableEntityItem_h
#define hifi_RenderableEntityItem_h

#include <render/Scene.h>
#include <EntityItem.h>
#include <Sound.h>
#include "AbstractViewStateInterface.h"
#include "EntitiesRendererLogging.h"


// These or the icon "name" used by the render item status value, they correspond to the atlas texture used by the DrawItemStatus
// job in the current rendering pipeline defined as of now  (11/2015) in render-utils/RenderDeferredTask.cpp.
enum class RenderItemStatusIcon {
    ACTIVE_IN_BULLET = 0,
    PACKET_SENT = 1,
    PACKET_RECEIVED = 2,
    SIMULATION_OWNER = 3,
    HAS_ACTIONS = 4,
    OTHER_SIMULATION_OWNER = 5,
    CLIENT_ONLY = 6,
    NONE = 255
};

void makeEntityItemStatusGetters(EntityItemPointer entity, render::Item::Status::Getters& statusGetters);


// Renderable entity item interface
class RenderableEntityInterface {
public:
    virtual void render(RenderArgs* args) {};
    virtual bool addToScene(const EntityItemPointer& self, const render::ScenePointer& scene, render::Transaction& transaction) = 0;
    virtual void removeFromScene(const EntityItemPointer& self, const render::ScenePointer& scene, render::Transaction& transaction) = 0;
    const SharedSoundPointer& getCollisionSound() { return _collisionSound; }
    void setCollisionSound(const SharedSoundPointer& sound) { _collisionSound = sound; }
    virtual RenderableEntityInterface* getRenderableInterface() { return nullptr; }
private:
    SharedSoundPointer _collisionSound;
};

class RenderableEntityItemProxy {
public:
    RenderableEntityItemProxy(const EntityItemPointer& entity, render::ItemID metaID)
        : _entity(entity), _metaID(metaID) {}
    typedef render::Payload<RenderableEntityItemProxy> Payload;
    typedef Payload::DataPointer Pointer;

    const EntityItemPointer _entity;
    render::ItemID _metaID;
};

namespace render {
   template <> const ItemKey payloadGetKey(const RenderableEntityItemProxy::Pointer& payload);
   template <> const Item::Bound payloadGetBound(const RenderableEntityItemProxy::Pointer& payload);
   template <> void payloadRender(const RenderableEntityItemProxy::Pointer& payload, RenderArgs* args);
   template <> uint32_t metaFetchMetaSubItems(const RenderableEntityItemProxy::Pointer& payload, ItemIDs& subItems);
}


// Mixin class for implementing basic single item rendering
class SimplerRenderableEntitySupport : public RenderableEntityInterface {
public:

    bool addToScene(const EntityItemPointer& self, const render::ScenePointer& scene, render::Transaction& transaction) override;
    void removeFromScene(const EntityItemPointer& self, const render::ScenePointer& scene, render::Transaction& transaction) override;
    void notifyChanged();

protected:

    render::ItemID _myItem { render::Item::INVALID_ITEM_ID };
};


#define SIMPLE_RENDERABLE() \
public: \
    virtual void locationChanged(bool tellPhysics = true) override { EntityItem::locationChanged(tellPhysics); notifyChanged(); } \
    virtual void dimensionsChanged() override { EntityItem::dimensionsChanged(); notifyChanged(); } \
    virtual RenderableEntityInterface* getRenderableInterface() override { return this; } \
    void checkFading() { \
        bool transparent = isTransparent(); \
        if (transparent != _prevIsTransparent) { \
            notifyChanged(); \
            _isFading = false; \
            _prevIsTransparent = transparent; \
        } \
    } \
private: \
    bool _prevIsTransparent { isTransparent() };


#endif // hifi_RenderableEntityItem_h
