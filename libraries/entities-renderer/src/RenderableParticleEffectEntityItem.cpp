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
#include <PerfStat.h>
#include <GeometryCache.h>
#include <AbstractViewStateInterface.h>
#include "EntitiesRendererLogging.h"

#include "RenderableParticleEffectEntityItem.h"

#include "untextured_particle_vert.h"
#include "untextured_particle_frag.h"
#include "textured_particle_vert.h"
#include "textured_particle_frag.h"


class ParticlePayloadData {
public:
    static const size_t VERTEX_PER_PARTICLE = 4;

    template<typename T>
    struct InterpolationData {
        T start;
        T middle;
        T finish;
        T spread;
    };
    struct ParticleUniforms {
        InterpolationData<float> radius;
        InterpolationData<glm::vec4> color; // rgba
        float lifespan;
        glm::vec3 spare;
    };
    
    struct ParticlePrimitive {
        ParticlePrimitive(glm::vec3 xyzIn, glm::vec2 uvIn) : xyz(xyzIn), uv(uvIn) {}
        glm::vec3 xyz; // Position
        glm::vec2 uv; // Lifetime + seed
    };
    
    using Payload = render::Payload<ParticlePayloadData>;
    using Pointer = Payload::DataPointer;
    using PipelinePointer = gpu::PipelinePointer;
    using FormatPointer = gpu::Stream::FormatPointer;
    using BufferPointer = gpu::BufferPointer;
    using TexturePointer = gpu::TexturePointer;
    using Format = gpu::Stream::Format;
    using Buffer = gpu::Buffer;
    using BufferView = gpu::BufferView;
    using ParticlePrimitives = std::vector<ParticlePrimitive>;
    
    ParticlePayloadData() {
        ParticleUniforms uniforms;
        _uniformBuffer = std::make_shared<Buffer>(sizeof(ParticleUniforms), (const gpu::Byte*) &uniforms);
        
        _vertexFormat->setAttribute(gpu::Stream::POSITION, 0, gpu::Element::VEC3F_XYZ,
                                    offsetof(ParticlePrimitive, xyz), gpu::Stream::PER_INSTANCE);
        _vertexFormat->setAttribute(gpu::Stream::COLOR, 0, gpu::Element::VEC2F_UV,
                                    offsetof(ParticlePrimitive, uv), gpu::Stream::PER_INSTANCE);
    }

    void setPipeline(PipelinePointer pipeline) { _pipeline = pipeline; }
    const PipelinePointer& getPipeline() const { return _pipeline; }

    const Transform& getModelTransform() const { return _modelTransform; }
    void setModelTransform(const Transform& modelTransform) { _modelTransform = modelTransform; }

    const AABox& getBound() const { return _bound; }
    void setBound(const AABox& bound) { _bound = bound; }

    BufferPointer getParticleBuffer() { return _particleBuffer; }
    const BufferPointer& getParticleBuffer() const { return _particleBuffer; }
    
    const ParticleUniforms& getParticleUniforms() const { return _uniformBuffer.get<ParticleUniforms>(); }
    ParticleUniforms& editParticleUniforms() { return _uniformBuffer.edit<ParticleUniforms>(); }

    void setTexture(TexturePointer texture) { _texture = texture; }
    const TexturePointer& getTexture() const { return _texture; }

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
        batch.setUniformBuffer(0, _uniformBuffer);
        batch.setInputFormat(_vertexFormat);
        batch.setInputBuffer(0, _particleBuffer, 0, sizeof(ParticlePrimitive));

        auto numParticles = _particleBuffer->getSize() / sizeof(ParticlePrimitive);
        batch.drawInstanced((gpu::uint32)numParticles, gpu::TRIANGLE_STRIP, (gpu::uint32)VERTEX_PER_PARTICLE);
    }

protected:
    Transform _modelTransform;
    AABox _bound;
    PipelinePointer _pipeline;
    FormatPointer _vertexFormat { std::make_shared<Format>() };
    BufferPointer _particleBuffer { std::make_shared<Buffer>() };
    BufferView _uniformBuffer;
    TexturePointer _texture;
    bool _visibleFlag = true;
};

namespace render {
    template <>
    const ItemKey payloadGetKey(const ParticlePayloadData::Pointer& payload) {
        if (payload->getVisibleFlag()) {
            return ItemKey::Builder::transparentShape();
        } else {
            return ItemKey::Builder().withInvisible().build();
        }
    }

    template <>
    const Item::Bound payloadGetBound(const ParticlePayloadData::Pointer& payload) {
        return payload->getBound();
    }

    template <>
    void payloadRender(const ParticlePayloadData::Pointer& payload, RenderArgs* args) {
        if (payload->getVisibleFlag()) {
            payload->render(args);
        }
    }
}



EntityItemPointer RenderableParticleEffectEntityItem::factory(const EntityItemID& entityID,
                                                              const EntityItemProperties& properties) {
    auto entity = std::make_shared<RenderableParticleEffectEntityItem>(entityID);
    entity->setProperties(properties);
    return entity;
}

RenderableParticleEffectEntityItem::RenderableParticleEffectEntityItem(const EntityItemID& entityItemID) :
    ParticleEffectEntityItem(entityItemID) {
    // lazy creation of particle system pipeline
    if (!_untexturedPipeline || !_texturedPipeline) {
        createPipelines();
    }
}

bool RenderableParticleEffectEntityItem::addToScene(EntityItemPointer self,
                                                    render::ScenePointer scene,
                                                    render::PendingChanges& pendingChanges) {
    _scene = scene;
    _renderItemId = _scene->allocateID();
    auto particlePayloadData = std::make_shared<ParticlePayloadData>();
    particlePayloadData->setPipeline(_untexturedPipeline);
    auto renderPayload = std::make_shared<ParticlePayloadData::Payload>(particlePayloadData);
    render::Item::Status::Getters statusGetters;
    makeEntityItemStatusGetters(getThisPointer(), statusGetters);
    renderPayload->addStatusGetters(statusGetters);
    pendingChanges.resetItem(_renderItemId, renderPayload);
    return true;
}

void RenderableParticleEffectEntityItem::removeFromScene(EntityItemPointer self,
                                                         render::ScenePointer scene,
                                                         render::PendingChanges& pendingChanges) {
    pendingChanges.removeItem(_renderItemId);
    _scene = nullptr;
    render::Item::clearID(_renderItemId);
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

void RenderableParticleEffectEntityItem::updateRenderItem() {
    // this 2 tests are synonyms for this class, but we would like to get rid of the _scene pointer ultimately
    if (!_scene || !render::Item::isValidID(_renderItemId)) { 
        return;
    }
    if (!getVisible()) {
        render::PendingChanges pendingChanges;
        pendingChanges.updateItem<ParticlePayloadData>(_renderItemId, [](ParticlePayloadData& payload) {
            payload.setVisibleFlag(false);
        });
        
        _scene->enqueuePendingChanges(pendingChanges);
        return;
    }
    
    using ParticleUniforms = ParticlePayloadData::ParticleUniforms;
    using ParticlePrimitive = ParticlePayloadData::ParticlePrimitive;
    using ParticlePrimitives = ParticlePayloadData::ParticlePrimitives;

    // Fill in Uniforms structure
    ParticleUniforms particleUniforms;
    particleUniforms.radius.start = getRadiusStart();
    particleUniforms.radius.middle = getParticleRadius();
    particleUniforms.radius.finish = getRadiusFinish();
    particleUniforms.radius.spread = getRadiusSpread();
    particleUniforms.color.start = glm::vec4(getColorStartRGB(), getAlphaStart());
    particleUniforms.color.middle = glm::vec4(getColorRGB(), getAlpha());
    particleUniforms.color.finish = glm::vec4(getColorFinishRGB(), getAlphaFinish());
    particleUniforms.color.spread = glm::vec4(getColorSpreadRGB(), getAlphaSpread());
    particleUniforms.lifespan = getLifespan();
    
    // Build particle primitives
    auto particlePrimitives = std::make_shared<ParticlePrimitives>();
    particlePrimitives->reserve(_particles.size()); // Reserve space
    for (auto& particle : _particles) {
        particlePrimitives->emplace_back(particle.position, glm::vec2(particle.lifetime, particle.seed));
    }

    bool successb, successp, successr;
    auto bounds = getAABox(successb);
    auto position = getPosition(successp);
    auto rotation = getOrientation(successr);
    bool success = successb && successp && successr;
    if (!success) {
        return;
    }
    Transform transform;
    if (!getEmitterShouldTrail()) {
        transform.setTranslation(position);
        transform.setRotation(rotation);
    }


    render::PendingChanges pendingChanges;
    pendingChanges.updateItem<ParticlePayloadData>(_renderItemId, [=](ParticlePayloadData& payload) {
        payload.setVisibleFlag(true);
        
        // Update particle uniforms
        memcpy(&payload.editParticleUniforms(), &particleUniforms, sizeof(ParticleUniforms));
        
        // Update particle buffer
        auto particleBuffer = payload.getParticleBuffer();
        size_t numBytes = sizeof(ParticlePrimitive) * particlePrimitives->size();
        particleBuffer->resize(numBytes);
        if (numBytes == 0) {
            return;
        }
        particleBuffer->setData(numBytes, (const gpu::Byte*)particlePrimitives->data());

        // Update transform and bounds
        payload.setModelTransform(transform);
        payload.setBound(bounds);

        if (_texture && _texture->isLoaded()) {
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
        state->setDepthTest(true, false, gpu::LESS_EQUAL);
        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE,
                                gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);

        auto vertShader = gpu::Shader::createVertex(std::string(untextured_particle_vert));
        auto fragShader = gpu::Shader::createPixel(std::string(untextured_particle_frag));

        auto program = gpu::Shader::createProgram(vertShader, fragShader);
        _untexturedPipeline = gpu::Pipeline::create(program, state);
    }
    if (!_texturedPipeline) {
        auto state = std::make_shared<gpu::State>();
        state->setCullMode(gpu::State::CULL_BACK);
        state->setDepthTest(true, false, gpu::LESS_EQUAL);
        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE,
                                gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);

        auto vertShader = gpu::Shader::createVertex(std::string(textured_particle_vert));
        auto fragShader = gpu::Shader::createPixel(std::string(textured_particle_frag));

        auto program = gpu::Shader::createProgram(vertShader, fragShader);
        _texturedPipeline = gpu::Pipeline::create(program, state);
    }
}

void RenderableParticleEffectEntityItem::notifyBoundChanged() {
    if (!render::Item::isValidID(_renderItemId)) {
        return;
    }
    render::PendingChanges pendingChanges;
    pendingChanges.updateItem<ParticlePayloadData>(_renderItemId, [](ParticlePayloadData& payload) {
    });

    _scene->enqueuePendingChanges(pendingChanges);
}