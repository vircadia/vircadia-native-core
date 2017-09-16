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

protected:
    virtual bool needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const override;
    virtual void doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) override;
    virtual void doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) override;

    virtual ItemKey getKey() override;
    virtual ShapeKey getShapeKey() override;
    virtual void doRender(RenderArgs* args) override;

    virtual bool isTransparent() const override { return true; }

    struct Vertex {
        Vertex() {}
        Vertex(const vec3& position, const vec3& normal, const vec2& uv) : position(position), normal(normal), uv(uv) {}
        vec3 position;
        vec3 normal;
        vec2 uv;
    };

    void updateGeometry(const std::vector<Vertex>& vertices);
    static std::vector<Vertex> updateVertices(const QVector<glm::vec3>& points, const QVector<glm::vec3>& normals, const QVector<float>& strokeWidths);

    Transform _polylineTransform;
    QVector<glm::vec3> _lastPoints;
    QVector<glm::vec3> _lastNormals;
    QVector<float> _lastStrokeWidths;
    gpu::BufferPointer _verticesBuffer;
    gpu::BufferView _uniformBuffer;
    uint32_t _numVertices { 0 };
    bool _empty{ true };
    NetworkTexturePointer _texture;
};

} } // namespace 

#endif // hifi_RenderablePolyLineEntityItem_h
