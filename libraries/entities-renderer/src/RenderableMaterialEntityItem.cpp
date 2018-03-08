//
//  Created by Sam Gondelman on 1/18/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderableMaterialEntityItem.h"

#include "RenderPipelines.h"

using namespace render;
using namespace render::entities;

bool MaterialEntityRenderer::needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const {
    if (entity->getMaterial() != _drawMaterial) {
        return true;
    }
    if (entity->getParentID() != _parentID || entity->getClientOnly() != _clientOnly || entity->getOwningAvatarID() != _owningAvatarID) {
        return true;
    }
    if (entity->getMaterialMappingPos() != _materialMappingPos || entity->getMaterialMappingScale() != _materialMappingScale || entity->getMaterialMappingRot() != _materialMappingRot) {
        return true;
    }
    return false;
}

void MaterialEntityRenderer::doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) {
    withWriteLock([&] {
        _drawMaterial = entity->getMaterial();
        _parentID = entity->getParentID();
        _clientOnly = entity->getClientOnly();
        _owningAvatarID = entity->getOwningAvatarID();
        _materialMappingPos = entity->getMaterialMappingPos();
        _materialMappingScale = entity->getMaterialMappingScale();
        _materialMappingRot = entity->getMaterialMappingRot();
        _renderTransform = getModelTransform();
        const float MATERIAL_ENTITY_SCALE = 0.5f;
        _renderTransform.postScale(MATERIAL_ENTITY_SCALE);
        _renderTransform.postScale(ENTITY_ITEM_DEFAULT_DIMENSIONS);
    });
}

ItemKey MaterialEntityRenderer::getKey() {
    ItemKey::Builder builder;
    builder.withTypeShape().withTagBits(render::ItemKey::TAG_BITS_0 | render::ItemKey::TAG_BITS_1);

    if (!_visible) {
        builder.withInvisible();
    }

    if (_drawMaterial) {
        auto matKey = _drawMaterial->getKey();
        if (matKey.isTranslucent()) {
            builder.withTransparent();
        }
    }

    return builder.build();
}

ShapeKey MaterialEntityRenderer::getShapeKey() {
    graphics::MaterialKey drawMaterialKey;
    if (_drawMaterial) {
        drawMaterialKey = _drawMaterial->getKey();
    }

    bool isTranslucent = drawMaterialKey.isTranslucent();
    bool hasTangents = drawMaterialKey.isNormalMap();
    bool hasLightmap = drawMaterialKey.isLightmapMap();
    bool isUnlit = drawMaterialKey.isUnlit();
    
    ShapeKey::Builder builder;
    builder.withMaterial();

    if (isTranslucent) {
        builder.withTranslucent();
    }
    if (hasTangents) {
        builder.withTangents();
    }
    if (hasLightmap) {
        builder.withLightmap();
    }
    if (isUnlit) {
        builder.withUnlit();
    }

    return builder.build();
}

glm::vec3 MaterialEntityRenderer::getVertexPos(float phi, float theta) {
    return glm::vec3(glm::sin(theta) * glm::cos(phi), glm::cos(theta), glm::sin(theta) * glm::sin(phi));
}

glm::vec3 MaterialEntityRenderer::getTangent(float phi, float theta) {
    return glm::vec3(-glm::cos(theta) * glm::cos(phi), glm::sin(theta), -glm::cos(theta) * glm::sin(phi));
}

void MaterialEntityRenderer::addVertex(std::vector<float>& buffer, const glm::vec3& pos, const glm::vec3& tan, const glm::vec2 uv) {
    buffer.push_back(pos.x); buffer.push_back(pos.y); buffer.push_back(pos.z);
    buffer.push_back(tan.x); buffer.push_back(tan.y); buffer.push_back(tan.z);
    buffer.push_back(uv.x); buffer.push_back(uv.y);
}

void MaterialEntityRenderer::addTriangleFan(std::vector<float>& buffer, int stack, int step) {
    float v1 = ((float)stack) / STACKS;
    float theta1 = v1 * (float)M_PI;
    glm::vec3 tip = getVertexPos(0, theta1);
    float v2 = ((float)(stack + step)) / STACKS;
    float theta2 = v2 * (float)M_PI;
    for (int i = 0; i < SLICES; i++) {
        float u1 = ((float)i) / SLICES;
        float u2 = ((float)(i + step)) / SLICES;
        float phi1 = u1 * M_PI_TIMES_2;
        float phi2 = u2 * M_PI_TIMES_2;
        /* (flipped for negative step)
             p1
            /  \
           /    \
          /      \
        p3 ------ p2
        */

        glm::vec3 pos2 = getVertexPos(phi2, theta2);
        glm::vec3 pos3 = getVertexPos(phi1, theta2);

        glm::vec3 tan1 = getTangent(0, theta1);
        glm::vec3 tan2 = getTangent(phi2, theta2);
        glm::vec3 tan3 = getTangent(phi1, theta2);

        glm::vec2 uv1 = glm::vec2((u1 + u2) / 2.0f, v1);
        glm::vec2 uv2 = glm::vec2(u2, v2);
        glm::vec2 uv3 = glm::vec2(u1, v2);

        addVertex(buffer, tip, tan1, uv1);
        addVertex(buffer, pos2, tan2, uv2);
        addVertex(buffer, pos3, tan3, uv3);

        _numVertices += 3;
    }
}

int MaterialEntityRenderer::_numVertices = 0;
std::shared_ptr<gpu::Stream::Format> MaterialEntityRenderer::_streamFormat = nullptr;
std::shared_ptr<gpu::BufferStream> MaterialEntityRenderer::_stream = nullptr;
std::shared_ptr<gpu::Buffer> MaterialEntityRenderer::_verticesBuffer = nullptr;

void MaterialEntityRenderer::generateMesh() {
    _streamFormat = std::make_shared<gpu::Stream::Format>();
    _stream = std::make_shared<gpu::BufferStream>();
    _verticesBuffer = std::make_shared<gpu::Buffer>();

    const int NUM_POS_COORDS = 3;
    const int NUM_TANGENT_COORDS = 3;
    const int VERTEX_TANGENT_OFFSET = NUM_POS_COORDS * sizeof(float);
    const int VERTEX_TEXCOORD_OFFSET = VERTEX_TANGENT_OFFSET + NUM_TANGENT_COORDS * sizeof(float);

    _streamFormat->setAttribute(gpu::Stream::POSITION, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), 0);
    _streamFormat->setAttribute(gpu::Stream::NORMAL, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), 0);
    _streamFormat->setAttribute(gpu::Stream::TANGENT, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), VERTEX_TANGENT_OFFSET);
    _streamFormat->setAttribute(gpu::Stream::TEXCOORD, 0, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV), VERTEX_TEXCOORD_OFFSET);

    _stream->addBuffer(_verticesBuffer, 0, _streamFormat->getChannels().at(0)._stride);

    std::vector<float> vertexBuffer;

    // Top
    addTriangleFan(vertexBuffer, 0, 1);

    // Middle section
    for (int j = 1; j < STACKS - 1; j++) {
        float v1 = ((float)j) / STACKS;
        float v2 = ((float)(j + 1)) / STACKS;
        float theta1 = v1 * (float)M_PI;
        float theta2 = v2 * (float)M_PI;
        for (int i = 0; i < SLICES; i++) {
            float u1 = ((float)i) / SLICES;
            float u2 = ((float)(i + 1)) / SLICES;
            float phi1 = u1 * M_PI_TIMES_2;
            float phi2 = u2 * M_PI_TIMES_2;

            /*
            p2 ---- p3
             |    /  |
             |   /   |
             |  /    |
            p1 ---- p4
            */

            glm::vec3 pos1 = getVertexPos(phi1, theta2);
            glm::vec3 pos2 = getVertexPos(phi1, theta1);
            glm::vec3 pos3 = getVertexPos(phi2, theta1);
            glm::vec3 pos4 = getVertexPos(phi2, theta2);

            glm::vec3 tan1 = getTangent(phi1, theta2);
            glm::vec3 tan2 = getTangent(phi1, theta1);
            glm::vec3 tan3 = getTangent(phi2, theta1);
            glm::vec3 tan4 = getTangent(phi2, theta2);

            glm::vec2 uv1 = glm::vec2(u1, v2);
            glm::vec2 uv2 = glm::vec2(u1, v1);
            glm::vec2 uv3 = glm::vec2(u2, v1);
            glm::vec2 uv4 = glm::vec2(u2, v2);

            addVertex(vertexBuffer, pos1, tan1, uv1);
            addVertex(vertexBuffer, pos2, tan2, uv2);
            addVertex(vertexBuffer, pos3, tan3, uv3);

            addVertex(vertexBuffer, pos3, tan3, uv3);
            addVertex(vertexBuffer, pos4, tan4, uv4);
            addVertex(vertexBuffer, pos1, tan1, uv1);

            _numVertices += 6;
        }
    }

    // Bottom
    addTriangleFan(vertexBuffer, STACKS, -1);

    _verticesBuffer->append(vertexBuffer.size() * sizeof(float), (gpu::Byte*) vertexBuffer.data());
}

void MaterialEntityRenderer::doRender(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableMaterialEntityItem::render");
    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;

    // Don't render if our parent is set or our material is null
    QUuid parentID;
    Transform renderTransform;
    graphics::MaterialPointer drawMaterial;
    Transform textureTransform;
    withReadLock([&] {
        parentID = _clientOnly ? _owningAvatarID : _parentID;
        renderTransform = _renderTransform;
        drawMaterial = _drawMaterial;
        textureTransform.setTranslation(glm::vec3(_materialMappingPos, 0));
        textureTransform.setRotation(glm::vec3(0, 0, glm::radians(_materialMappingRot)));
        textureTransform.setScale(glm::vec3(_materialMappingScale, 1));
    });
    if (!parentID.isNull() || !drawMaterial) {
        return;
    }

    batch.setModelTransform(renderTransform);
    drawMaterial->setTextureTransforms(textureTransform);

    // bind the material
    RenderPipelines::bindMaterial(drawMaterial, batch, args->_enableTexturing);
    args->_details._materialSwitches++;

    // Draw!
    if (_numVertices == 0) {
        generateMesh();
    }

    batch.setInputFormat(_streamFormat);
    batch.setInputStream(0, *_stream);
    batch.draw(gpu::TRIANGLES, _numVertices, 0);

    const int NUM_VERTICES_PER_TRIANGLE = 3;
    args->_details._trianglesRendered += _numVertices / NUM_VERTICES_PER_TRIANGLE;
}
