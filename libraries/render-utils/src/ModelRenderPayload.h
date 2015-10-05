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

#include <QUrl>

#include <gpu/Batch.h>

#include <render/Scene.h>

#include <model/Geometry.h>

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
    
    // MeshPartPayload functions to perform render
    void bindMesh(gpu::Batch& batch) const;
    void drawCall(gpu::Batch& batch) const;
    
    model::MeshPointer _drawMesh;
    model::Mesh::Part _drawPart;
    model::MaterialPointer _drawMaterial;
    
    mutable render::Item::Bound _bound;
    mutable bool _isBoundInvalid = true;
};

#endif // hifi_ModelRenderPayload_h