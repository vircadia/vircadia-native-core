//
//  Created by Sam Gondelman on 1/18/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableMaterialEntityItem_h
#define hifi_RenderableMaterialEntityItem_h

#include "RenderableEntityItem.h"

#include <MaterialEntityItem.h>

class NetworkMaterial;

namespace render { namespace entities { 

class MaterialEntityRenderer : public TypedEntityRenderer<MaterialEntityItem> {
    using Parent = TypedEntityRenderer<MaterialEntityItem>;
    using Pointer = std::shared_ptr<MaterialEntityRenderer>;
public:
    MaterialEntityRenderer(const EntityItemPointer& entity) : Parent(entity) {}

private:
    virtual bool needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const override;
    virtual void doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) override;
    virtual void doRender(RenderArgs* args) override;

    ItemKey getKey() override;
    ShapeKey getShapeKey() override;

    QUuid _parentID;
    bool _clientOnly;
    QUuid _owningAvatarID;
    glm::vec2 _materialMappingPos;
    glm::vec2 _materialMappingScale;
    float _materialMappingRot;
    Transform _renderTransform;

    std::shared_ptr<NetworkMaterial> _drawMaterial;

    static int _numVertices;
    static std::shared_ptr<gpu::Stream::Format> _streamFormat;
    static std::shared_ptr<gpu::BufferStream> _stream;
    static std::shared_ptr<gpu::Buffer> _verticesBuffer;

    void generateMesh();
    void addTriangleFan(std::vector<float>& buffer, int stack, int step);
    static glm::vec3 getVertexPos(float phi, float theta);
    static glm::vec3 getTangent(float phi, float theta);
    static void addVertex(std::vector<float>& buffer, const glm::vec3& pos, const glm::vec3& tan, const glm::vec2 uv);
    const int SLICES = 15;
    const int STACKS = 9;
    const float M_PI_TIMES_2 = 2.0f * (float)M_PI;
};

} } 
#endif // hifi_RenderableMaterialEntityItem_h
