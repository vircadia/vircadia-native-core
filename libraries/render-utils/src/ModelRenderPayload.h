//
//  ModelRenderPayload.h
//  interface/src/renderer
//
//  Created by Sam Gateau on 10/3/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelRenderPayload_h
#define hifi_ModelRenderPayload_h

#include <render/Scene.h>
#include <gpu/Batch.h>

class Model;

class MeshPartPayload {
public:
    MeshPartPayload(Model* model, int meshIndex, int partIndex, int shapeIndex);
    
    typedef render::Payload<MeshPartPayload> Payload;
    typedef Payload::DataPointer Pointer;
    
    Model* model;
    QUrl url;
    int meshIndex;
    int partIndex;
    int _shapeID;
    
    // Render Item interface
    render::ItemKey getKey() const;
    render::Item::Bound getBound() const;
    void render(RenderArgs* args) const;
    
    
    mutable render::Item::Bound _bound;
    mutable bool _isBoundInvalid = true;
};

namespace render {
    template <> const ItemKey payloadGetKey(const MeshPartPayload::Pointer& payload) {
        if (payload) {
            return payload->getKey();
        }
        // Return opaque for lack of a better idea
        return ItemKey::Builder::opaqueShape();
    }
    
    template <> const Item::Bound payloadGetBound(const MeshPartPayload::Pointer& payload) {
        if (payload) {
            return payload->getBound();
        }
        return render::Item::Bound();
    }
    template <> void payloadRender(const MeshPartPayload::Pointer& payload, RenderArgs* args) {
        return payload->render(args);
    }
    
    /* template <> const model::MaterialKey& shapeGetMaterialKey(const MeshPartPayload::Pointer& payload) {
     return payload->model->getPartMaterial(payload->meshIndex, payload->partIndex);
     }*/
}


#endif // hifi_ModelRenderPayload_h