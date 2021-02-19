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

    void updateModelTransformAndBound(const EntityItemPointer& entity) override;

    virtual bool isTransparent() const override;

protected:
    virtual bool needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const override;
    virtual void doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) override;
    virtual void doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) override;

    virtual ItemKey getKey() override;
    virtual ShapeKey getShapeKey() override;
    virtual void doRender(RenderArgs* args) override;

    static void buildPipelines();
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

    bool _isUVModeStretch { false };
    bool _faceCamera { false };
    bool _glow { false };

    size_t _numVertices { 0 };
    gpu::BufferPointer _polylineDataBuffer;
    gpu::BufferPointer _polylineGeometryBuffer;
    static std::map<std::pair<render::Args::RenderMethod, bool>, gpu::PipelinePointer> _pipelines;
};

} } // namespace 

#endif // hifi_RenderablePolyLineEntityItem_h
