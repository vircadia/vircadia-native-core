//
//  RenderablePolyLineEntityItem.h
//  libraries/entities-renderer/src/
//
//  Created by Eric Levin on 6/22/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderablePolyLineEntityItem_h
#define hifi_RenderablePolyLineEntityItem_h

#include "RenderableEntityItem.h"
#include <PolyLineEntityItem.h>
#include <TextureCache.h>

namespace render { namespace entities {

class PolyLineEntityRenderer : public TypedEntityRenderer<PolyLineEntityItem> {
    using Parent = TypedEntityRenderer<PolyLineEntityItem>;
    friend class EntityRenderer;

public:
    PolyLineEntityRenderer(const EntityItemPointer& entity);

    // FIXME: shouldn't always be transparent: take into account texture and glow
    virtual bool isTransparent() const override { return true; }

protected:
    virtual bool needsRenderUpdate() const override;
    virtual bool needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const override;
    virtual void doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) override;

    virtual ItemKey getKey() override;
    virtual ShapeKey getShapeKey() override;
    virtual void doRender(RenderArgs* args) override;

    void buildPipeline();
    void updateGeometry();
    void updateData();

    QVector<glm::vec3> _points;
    QVector<glm::vec3> _normals;
    QVector<glm::vec3> _colors;
    glm::vec3 _color;
    QVector<float> _widths;

    NetworkTexturePointer _texture;
    float _textureAspectRatio { 1.0f };
    bool _textureLoaded { false };

    bool _isUVModeStretch;
    bool _faceCamera;
    bool _glow;

    size_t _numVertices;
    gpu::BufferPointer _polylineDataBuffer;
    gpu::BufferPointer _polylineGeometryBuffer;
    static gpu::PipelinePointer _pipeline;
    static gpu::PipelinePointer _glowPipeline;
};

} } // namespace 

#endif // hifi_RenderablePolyLineEntityItem_h
