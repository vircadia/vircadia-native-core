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

/*

#include <render/Scene.h>
#include <EntityItem.h>

class RenderableEntityItemProxy {
public:
    RenderableEntityItemProxy(EntityItem* entity) {
        _entity = entity;
    }
    
    const render::ItemKey& getKey() const {
        _myKey = render::ItemKey::Builder().withTypeShape().build();
        return _myKey;
    }
    
    const render::Item::Bound getBounds() const {
        if (_entity) {
            return _entity->getAABox();
        }
        return render::Item::Bound();
    }
    
    void render(RenderArgs* args) const {
        if (_entity) {
            _entity->render(args);
        }
    }
    
    void clearEntity() {
        _entity = nullptr;
    }
    
private:
    mutable render::ItemKey _myKey;
    EntityItem* _entity = nullptr;
};

typedef render::Payload<RenderableEntityItemProxy> RenderableEntityItemProxyPayload;

class RenderableEntityItem {
public:
    ~RenderableEntityItem() {
        if (_proxy) {
            _proxy->clearEntity();
        }
    }
    
    RenderableEntityItemProxy* getRenderProxy() {
        if (!_proxy) {
            //_proxy = new RenderableEntityItemProxy(this);
        }
        return _proxy;
    }
private:
    RenderableEntityItemProxy* _proxy = nullptr;
};

*/


#endif // hifi_RenderableEntityItem_h
