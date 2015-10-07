//
//  MeshPartPayload.h
//  interface/src/renderer
//
//  Created by Sam Gateau on 10/3/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MeshPartPayload_h
#define hifi_MeshPartPayload_h

#include <gpu/Batch.h>

#include <render/Scene.h>

#include <model/Geometry.h>

#include "ModelRender.h"

class Model;

class MeshPartPayload {
public:
    MeshPartPayload(Model* model, int meshIndex, int partIndex, int shapeIndex);
    
    typedef render::Payload<MeshPartPayload> Payload;
    typedef Payload::DataPointer Pointer;

    Model* model;
    int meshIndex;
    int partIndex;
    int _shapeID;
    
    // Render Item interface
    render::ItemKey getKey() const;
    render::Item::Bound getBound() const;
    void render(RenderArgs* args) const;
    
    // MeshPartPayload functions to perform render
    void drawCall(gpu::Batch& batch) const;
    void bindMesh(gpu::Batch& batch) const;
    void bindMaterial(gpu::Batch& batch, const ModelRender::Locations* locations) const;
    void bindTransform(gpu::Batch& batch, const ModelRender::Locations* locations) const;


    void initCache();

    // Payload resource cached values
    model::MeshPointer _drawMesh;
    model::Mesh::Part _drawPart;
    model::MaterialPointer _drawMaterial;
    bool _hasColorAttrib = false;
    bool _isSkinned = false;
    bool _isBlendShaped = false;

    mutable render::Item::Bound _bound;
    mutable bool _isBoundInvalid = true;
};

namespace render {
    template <> const ItemKey payloadGetKey(const MeshPartPayload::Pointer& payload);
    template <> const Item::Bound payloadGetBound(const MeshPartPayload::Pointer& payload);
    template <> void payloadRender(const MeshPartPayload::Pointer& payload, RenderArgs* args);
}

#endif // hifi_MeshPartPayload_h