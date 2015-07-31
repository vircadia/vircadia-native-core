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


class RenderableEntityItemProxy {
public:
    RenderableEntityItemProxy(EntityItemPointer entity) : entity(entity) { }
    typedef render::Payload<RenderableEntityItemProxy> Payload;
    typedef Payload::DataPointer Pointer;
    
    EntityItemPointer entity;
};

namespace render {
   template <> const ItemKey payloadGetKey(const RenderableEntityItemProxy::Pointer& payload); 
   template <> const Item::Bound payloadGetBound(const RenderableEntityItemProxy::Pointer& payload);
   template <> void payloadRender(const RenderableEntityItemProxy::Pointer& payload, RenderArgs* args);
}

// Mixin class for implementing basic single item rendering
class SimpleRenderableEntityItem {
public:
    bool addToScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges) {
        _myItem = scene->allocateID();
        
        auto renderData = std::make_shared<RenderableEntityItemProxy>(self);
        auto renderPayload = std::make_shared<RenderableEntityItemProxy::Payload>(renderData);
        
        pendingChanges.resetItem(_myItem, renderPayload);
        
        return true;
    }

    void removeFromScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges) {
        pendingChanges.removeItem(_myItem);
    }
    
private:
    render::ItemID _myItem;
};


#define SIMPLE_RENDERABLE() \
public: \
    virtual bool addToScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges) { return _renderHelper.addToScene(self, scene, pendingChanges); } \
    virtual void removeFromScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges) { _renderHelper.removeFromScene(self, scene, pendingChanges); } \
private: \
    SimpleRenderableEntityItem _renderHelper;



#endif // hifi_RenderableEntityItem_h
