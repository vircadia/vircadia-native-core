//
//  ModelMeshPartPayload.h
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
#include <render/ShapePipeline.h>

#include <model/Geometry.h>

class Model;

class MeshPartPayload {
public:
    MeshPartPayload() {}
    MeshPartPayload(const std::shared_ptr<const model::Mesh>& mesh, int partIndex, model::MaterialPointer material, const Transform& transform, const Transform& offsetTransform);

    typedef render::Payload<MeshPartPayload> Payload;
    typedef Payload::DataPointer Pointer;

    virtual void updateMeshPart(const std::shared_ptr<const model::Mesh>& drawMesh, int partIndex);

    virtual void notifyLocationChanged() {}
    virtual void updateTransform(const Transform& transform, const Transform& offsetTransform);

    virtual void updateMaterial(model::MaterialPointer drawMaterial);

    // Render Item interface
    virtual render::ItemKey getKey() const;
    virtual render::Item::Bound getBound() const;
    virtual render::ShapeKey getShapeKey() const; // shape interface
    virtual void render(RenderArgs* args) const;

    // ModelMeshPartPayload functions to perform render
    void drawCall(gpu::Batch& batch) const;
    virtual void bindMesh(gpu::Batch& batch) const;
    virtual void bindMaterial(gpu::Batch& batch, const render::ShapePipeline::LocationsPointer locations) const;
    virtual void bindTransform(gpu::Batch& batch, const render::ShapePipeline::LocationsPointer locations, bool canCauterize = true) const;

    // Payload resource cached values
    std::shared_ptr<const model::Mesh> _drawMesh;
    int _partIndex = 0;
    model::Mesh::Part _drawPart;

    std::shared_ptr<const model::Material> _drawMaterial;
    
    model::Box _localBound;
    Transform _drawTransform;
    Transform _transform;
    Transform _offsetTransform;
    mutable model::Box _worldBound;
    
    bool _hasColorAttrib = false;
};

namespace render {
    template <> const ItemKey payloadGetKey(const MeshPartPayload::Pointer& payload);
    template <> const Item::Bound payloadGetBound(const MeshPartPayload::Pointer& payload);
    template <> const ShapeKey shapeGetShapeKey(const MeshPartPayload::Pointer& payload);
    template <> void payloadRender(const MeshPartPayload::Pointer& payload, RenderArgs* args);
}

class ModelMeshPartPayload : public MeshPartPayload {
public:
    ModelMeshPartPayload(Model* model, int meshIndex, int partIndex, int shapeIndex, const Transform& transform, const Transform& offsetTransform);

    typedef render::Payload<ModelMeshPartPayload> Payload;
    typedef Payload::DataPointer Pointer;

    void notifyLocationChanged() override;
    void updateTransformForSkinnedMesh(const Transform& transform, const Transform& offsetTransform, const QVector<glm::mat4>& clusterMatrices);

    // Entity fade in
    float calcFadeRatio() const;

    // Render Item interface
    render::ItemKey getKey() const override;
    render::ShapeKey getShapeKey() const override; // shape interface
    void render(RenderArgs* args) const override;

    // ModelMeshPartPayload functions to perform render
    void bindMesh(gpu::Batch& batch) const override;
    void bindTransform(gpu::Batch& batch, const render::ShapePipeline::LocationsPointer locations, bool canCauterize) const override;

    void initCache();

    Model* _model;

    int _meshIndex;
    int _shapeID;

    bool _isSkinned{ false };
    bool _isBlendShaped{ false };

private:
    quint64 _fadeStartTime { usecTimestampNow() };
    bool _hasFadeStarted { false };
};

namespace render {
    template <> const ItemKey payloadGetKey(const ModelMeshPartPayload::Pointer& payload);
    template <> const Item::Bound payloadGetBound(const ModelMeshPartPayload::Pointer& payload);
    template <> const ShapeKey shapeGetShapeKey(const ModelMeshPartPayload::Pointer& payload);
    template <> void payloadRender(const ModelMeshPartPayload::Pointer& payload, RenderArgs* args);
}

#endif // hifi_MeshPartPayload_h
