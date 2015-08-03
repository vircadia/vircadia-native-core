//
//  RenderableParticleEffectEntityItem.cpp
//  interface/src
//
//  Created by Jason Rickwald on 3/2/15.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/quaternion.hpp>

#include <DependencyManager.h>
#include <DeferredLightingEffect.h>
#include <PerfStat.h>
#include <GeometryCache.h>
#include <AbstractViewStateInterface.h>
#include "EntitiesRendererLogging.h"

#include "RenderableParticleEffectEntityItem.h"

#include "untextured_particle_vert.h"
#include "untextured_particle_frag.h"
#include "textured_particle_vert.h"
#include "textured_particle_frag.h"

class ParticlePayload {
public:
    typedef render::Payload<ParticlePayload> Payload;
    typedef Payload::DataPointer Pointer;
    typedef RenderableParticleEffectEntityItem::Vertex Vertex;

    ParticlePayload() : _vertexFormat(std::make_shared<gpu::Stream::Format>()),
                        _vertexBuffer(std::make_shared<gpu::Buffer>()),
                        _indexBuffer(std::make_shared<gpu::Buffer>()) {
        _vertexFormat->setAttribute(gpu::Stream::POSITION, 0, gpu::Element::VEC3F_XYZ, 0);
        _vertexFormat->setAttribute(gpu::Stream::TEXCOORD, 0, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV), offsetof(Vertex, uv));
        _vertexFormat->setAttribute(gpu::Stream::COLOR, 0, gpu::Element::COLOR_RGBA_32, offsetof(Vertex, rgba));
    }

    void setPipeline(gpu::PipelinePointer pipeline) { _pipeline = pipeline; }
    const gpu::PipelinePointer& getPipeline() const { return _pipeline; }

    const Transform& getModelTransform() const { return _modelTransform; }
    void setModelTransform(const Transform& modelTransform) { _modelTransform = modelTransform; }

    const AABox& getBound() const { return _bound; }
    void setBound(AABox& bound) { _bound = bound; }

    gpu::BufferPointer getVertexBuffer() { return _vertexBuffer; }
    const gpu::BufferPointer& getVertexBuffer() const { return _vertexBuffer; }

    gpu::BufferPointer getIndexBuffer() { return _indexBuffer; }
    const gpu::BufferPointer& getIndexBuffer() const { return _indexBuffer; }

    void setTexture(gpu::TexturePointer texture) { _texture = texture; }
    const gpu::TexturePointer& getTexture() const { return _texture; }

    bool getVisibleFlag() const { return _visibleFlag; }
    void setVisibleFlag(bool visibleFlag) { _visibleFlag = visibleFlag; }

    void render(RenderArgs* args) const {
        assert(_pipeline);

        gpu::Batch& batch = *args->_batch;
        batch.setPipeline(_pipeline);

        if (_texture) {
            batch.setResourceTexture(0, _texture);
        }

        batch.setModelTransform(_modelTransform);
        batch.setInputFormat(_vertexFormat);
        batch.setInputBuffer(0, _vertexBuffer, 0, sizeof(Vertex));
        batch.setIndexBuffer(gpu::UINT16, _indexBuffer, 0);

        auto numIndices = _indexBuffer->getSize() / sizeof(uint16_t);
        batch.drawIndexed(gpu::TRIANGLES, numIndices);
    }

protected:
    Transform _modelTransform;
    AABox _bound;
    gpu::PipelinePointer _pipeline;
    gpu::Stream::FormatPointer _vertexFormat;
    gpu::BufferPointer _vertexBuffer;
    gpu::BufferPointer _indexBuffer;
    gpu::TexturePointer _texture;
    bool _visibleFlag = true;
};

namespace render {
    template <>
    const ItemKey payloadGetKey(const ParticlePayload::Pointer& payload) {
        if (payload->getVisibleFlag()) {
            return ItemKey::Builder::transparentShape();
        } else {
            return ItemKey::Builder().withInvisible().build();
        }
    }

    template <>
    const Item::Bound payloadGetBound(const ParticlePayload::Pointer& payload) {
        return payload->getBound();
    }

    template <>
    void payloadRender(const ParticlePayload::Pointer& payload, RenderArgs* args) {
        payload->render(args);
    }
}

gpu::PipelinePointer RenderableParticleEffectEntityItem::_texturedPipeline;
gpu::PipelinePointer RenderableParticleEffectEntityItem::_untexturedPipeline;

EntityItemPointer RenderableParticleEffectEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return std::make_shared<RenderableParticleEffectEntityItem>(entityID, properties);
}

RenderableParticleEffectEntityItem::RenderableParticleEffectEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
    ParticleEffectEntityItem(entityItemID, properties) {

    // lazy creation of particle system pipeline
    if (!_untexturedPipeline && !_texturedPipeline) {
        createPipelines();
    }
}

bool RenderableParticleEffectEntityItem::addToScene(EntityItemPointer self,
                                                    render::ScenePointer scene,
                                                    render::PendingChanges& pendingChanges) {

    auto particlePayload = std::shared_ptr<ParticlePayload>(new ParticlePayload());
    particlePayload->setPipeline(_untexturedPipeline);
    _renderItemId = scene->allocateID();
    auto renderData = ParticlePayload::Pointer(particlePayload);
    auto renderPayload = render::PayloadPointer(new ParticlePayload::Payload(renderData));
    pendingChanges.resetItem(_renderItemId, renderPayload);
    _scene = scene;
    return true;
}

void RenderableParticleEffectEntityItem::removeFromScene(EntityItemPointer self,
                                                         render::ScenePointer scene,
                                                         render::PendingChanges& pendingChanges) {
    pendingChanges.removeItem(_renderItemId);
    _scene = nullptr;
};

void RenderableParticleEffectEntityItem::update(const quint64& now) {
    ParticleEffectEntityItem::update(now);

    if (_texturesChangedFlag) {
        if (_textures.isEmpty()) {
            _texture.clear();
        } else {
            // for now use the textures string directly.
            // Eventually we'll want multiple textures in a map or array.
            _texture = DependencyManager::get<TextureCache>()->getTexture(_textures);
        }
        _texturesChangedFlag = false;
    }

    updateRenderItem();
}

static glm::vec3 zSortAxis;
static bool zSort(const glm::vec3& rhs, const glm::vec3& lhs) {
    return glm::dot(rhs, ::zSortAxis) > glm::dot(lhs, ::zSortAxis);
}

uint32_t toRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return ((uint32_t)r | (uint32_t)g << 8 | (uint32_t)b << 16 | (uint32_t)a << 24);
}

void RenderableParticleEffectEntityItem::updateRenderItem() {

    if (!_scene) {
        return;
    }

    float particleRadius = getParticleRadius();
    auto xcolor = getXColor();
    auto alpha = (uint8_t)(glm::clamp(getLocalRenderAlpha(), 0.0f, 1.0f) * 255.0f);
    auto rgba = toRGBA(xcolor.red, xcolor.green, xcolor.blue, alpha);

    // make a copy of each particle position
    std::vector<glm::vec3> positions;
    positions.reserve(getLivingParticleCount());
    for (quint32 i = _particleHeadIndex; i != _particleTailIndex; i = (i + 1) % _maxParticles) {
        positions.push_back(_particlePositions[i]);
    }

    // sort particles back to front
    // NOTE: this is view frustum might be one frame out of date.
    auto frustum = AbstractViewStateInterface::instance()->getCurrentViewFrustum();
    ::zSortAxis = frustum->getDirection();
    qSort(positions.begin(), positions.end(), zSort);

    // allocate vertices
    _vertices.clear();

    // build vertices from particle positions
    const glm::vec3 upOffset = frustum->getUp() * particleRadius;
    const glm::vec3 rightOffset = frustum->getRight() * particleRadius;
    for (auto&& pos : positions) {
        // generate corners of quad aligned to face the camera.
        _vertices.emplace_back(pos + rightOffset + upOffset, glm::vec2(1.0f, 1.0f), rgba);
        _vertices.emplace_back(pos - rightOffset + upOffset, glm::vec2(0.0f, 1.0f), rgba);
        _vertices.emplace_back(pos - rightOffset - upOffset, glm::vec2(0.0f, 0.0f), rgba);
        _vertices.emplace_back(pos + rightOffset - upOffset, glm::vec2(1.0f, 0.0f), rgba);
    }

    render::PendingChanges pendingChanges;
    pendingChanges.updateItem<ParticlePayload>(_renderItemId, [&](ParticlePayload& payload) {

        // update vertex buffer
        auto vertexBuffer = payload.getVertexBuffer();
        size_t numBytes = sizeof(Vertex) * _vertices.size();
        vertexBuffer->resize(numBytes);
        gpu::Byte* data = vertexBuffer->editData();
        memcpy(data, &(_vertices[0]), numBytes);

        // FIXME, don't update index buffer if num particles has not changed.
        // update index buffer
        auto indexBuffer = payload.getIndexBuffer();
        const size_t NUM_VERTS_PER_PARTICLE = 4;
        const size_t NUM_INDICES_PER_PARTICLE = 6;
        auto numQuads = (_vertices.size() / NUM_VERTS_PER_PARTICLE);
        numBytes = sizeof(uint16_t) * numQuads * NUM_INDICES_PER_PARTICLE;
        indexBuffer->resize(numBytes);
        data = indexBuffer->editData();
        auto indexPtr = reinterpret_cast<uint16_t*>(data);
        for (size_t i = 0; i < numQuads; ++i) {
            indexPtr[i * NUM_INDICES_PER_PARTICLE + 0] = i * NUM_VERTS_PER_PARTICLE + 0;
            indexPtr[i * NUM_INDICES_PER_PARTICLE + 1] = i * NUM_VERTS_PER_PARTICLE + 1;
            indexPtr[i * NUM_INDICES_PER_PARTICLE + 2] = i * NUM_VERTS_PER_PARTICLE + 3;
            indexPtr[i * NUM_INDICES_PER_PARTICLE + 3] = i * NUM_VERTS_PER_PARTICLE + 1;
            indexPtr[i * NUM_INDICES_PER_PARTICLE + 4] = i * NUM_VERTS_PER_PARTICLE + 2;
            indexPtr[i * NUM_INDICES_PER_PARTICLE + 5] = i * NUM_VERTS_PER_PARTICLE + 3;
        }

        // update transform
        glm::quat rot = _transform.getRotation();
        glm::vec3 pos = _transform.getTranslation();
        Transform t;
        t.setRotation(rot);
        t.setTranslation(pos);
        payload.setModelTransform(t);

        // transform _particleMinBound and _particleMaxBound corners into world coords
        glm::vec3 d = _particleMaxBound - _particleMinBound;
        const size_t NUM_BOX_CORNERS = 8;
        glm::vec3 corners[NUM_BOX_CORNERS] = {
            pos + rot * (_particleMinBound + glm::vec3(0.0f, 0.0f, 0.0f)),
            pos + rot * (_particleMinBound + glm::vec3(d.x, 0.0f, 0.0f)),
            pos + rot * (_particleMinBound + glm::vec3(0.0f, d.y, 0.0f)),
            pos + rot * (_particleMinBound + glm::vec3(d.x, d.y, 0.0f)),
            pos + rot * (_particleMinBound + glm::vec3(0.0f, 0.0f, d.z)),
            pos + rot * (_particleMinBound + glm::vec3(d.x, 0.0f, d.z)),
            pos + rot * (_particleMinBound + glm::vec3(0.0f, d.y, d.z)),
            pos + rot * (_particleMinBound + glm::vec3(d.x, d.y, d.z))
        };
        glm::vec3 min(FLT_MAX, FLT_MAX, FLT_MAX);
        glm::vec3 max = -min;
        for (size_t i = 0; i < NUM_BOX_CORNERS; i++) {
            min.x = std::min(min.x, corners[i].x);
            min.y = std::min(min.y, corners[i].y);
            min.z = std::min(min.z, corners[i].z);
            max.x = std::max(max.x, corners[i].x);
            max.y = std::max(max.y, corners[i].y);
            max.z = std::max(max.z, corners[i].z);
        }
        AABox bound(min, max - min);
        payload.setBound(bound);

        bool textured = _texture && _texture->isLoaded();
        if (textured) {
            payload.setTexture(_texture->getGPUTexture());
            payload.setPipeline(_texturedPipeline);
        } else {
            payload.setTexture(nullptr);
            payload.setPipeline(_untexturedPipeline);
        }
    });

    _scene->enqueuePendingChanges(pendingChanges);
}

void RenderableParticleEffectEntityItem::createPipelines() {
    if (!_untexturedPipeline) {
        auto state = std::make_shared<gpu::State>();
        state->setCullMode(gpu::State::CULL_BACK);
        state->setDepthTest(true, true, gpu::LESS_EQUAL);
        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD,
                                gpu::State::INV_SRC_ALPHA, gpu::State::FACTOR_ALPHA,
                                gpu::State::BLEND_OP_ADD, gpu::State::ONE);
        auto vertShader = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(untextured_particle_vert)));
        auto fragShader = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(untextured_particle_frag)));
        auto program = gpu::ShaderPointer(gpu::Shader::createProgram(vertShader, fragShader));
        _untexturedPipeline = gpu::PipelinePointer(gpu::Pipeline::create(program, state));
    }
    if (!_texturedPipeline) {
        auto state = std::make_shared<gpu::State>();
        state->setCullMode(gpu::State::CULL_BACK);
        state->setDepthTest(true, true, gpu::LESS_EQUAL);
        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD,
                                gpu::State::INV_SRC_ALPHA, gpu::State::FACTOR_ALPHA,
                                gpu::State::BLEND_OP_ADD, gpu::State::ONE);
        auto vertShader = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(textured_particle_vert)));
        auto fragShader = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(textured_particle_frag)));
        auto program = gpu::ShaderPointer(gpu::Shader::createProgram(vertShader, fragShader));
        _texturedPipeline = gpu::PipelinePointer(gpu::Pipeline::create(program, state));
    }
}
