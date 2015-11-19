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
#include "textured_particle_alpha_discard_frag.h"

static const size_t VERTEX_PER_PARTICLE = 4;

class ParticlePayload {
public:
    using Payload = render::Payload<ParticlePayload>;
    using Pointer = Payload::DataPointer;
    using ParticlePrimitive = RenderableParticleEffectEntityItem::ParticlePrimitive;
    using ParticleUniforms = RenderableParticleEffectEntityItem::ParticleUniforms;
    using PipelinePointer = gpu::PipelinePointer;
    using FormatPointer = gpu::Stream::FormatPointer;
    using BufferPointer = gpu::BufferPointer;
    using TexturePointer = gpu::TexturePointer;
    using Format = gpu::Stream::Format;
    using Buffer = gpu::Buffer;
    using BufferView = gpu::BufferView;

    ParticlePayload(EntityItemPointer entity) : _entity(entity) {
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
    void setBound(AABox& bound) { _bound = bound; }

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
        batch.drawInstanced(numParticles, gpu::TRIANGLE_STRIP, VERTEX_PER_PARTICLE);
    }

protected:
    EntityItemPointer _entity;
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

    _scene = scene;
    _renderItemId = _scene->allocateID();
    auto particlePayload = std::make_shared<ParticlePayload>(shared_from_this());
    particlePayload->setPipeline(_untexturedPipeline);
    auto renderPayload = std::make_shared<ParticlePayload::Payload>(particlePayload);
    render::Item::Status::Getters statusGetters;
    makeEntityItemStatusGetters(shared_from_this(), statusGetters);
    renderPayload->addStatusGetters(statusGetters);
    pendingChanges.resetItem(_renderItemId, renderPayload);
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

static glm::vec4 toGlm(const xColor& color, float alpha) {
    return glm::vec4((float)color.red / 255.0f, (float)color.green / 255.0f, (float)color.blue / 255.0f, alpha);
}

void RenderableParticleEffectEntityItem::updateRenderItem() {
    if (!_scene) {
        return;
    }
    
    // Fill in Uniforms structure
    _particleUniforms.radius.start = getRadiusStart();
    _particleUniforms.radius.middle = getParticleRadius();
    _particleUniforms.radius.finish = getRadiusFinish();
    _particleUniforms.radius.spread = getRadiusSpread();
    
    _particleUniforms.color.start = toGlm(getColorStart(), getAlphaStart());
    _particleUniforms.color.middle = toGlm(getXColor(), getAlpha());
    _particleUniforms.color.finish = toGlm(getColorFinish(), getAlphaFinish());
    _particleUniforms.color.spread = toGlm(getColorSpread(), getAlphaSpread());
    
    _particleUniforms.lifespan = getLifespan();
    
    // Build particle primitives
    _particlePrimitives.clear(); // clear primitives
    _particlePrimitives.reserve(_particles.size()); // Reserve space
    for (auto& particle : _particles) {
        _particlePrimitives.emplace_back(particle.position, glm::vec2(particle.lifetime, particle.seed));
    }

    // No need to sort if we're doing additive blending
    if (!_additiveBlending) {
        // sort particles back to front
        // NOTE: this is view frustum might be one frame out of date.
        auto direction = AbstractViewStateInterface::instance()->getCurrentViewFrustum()->getDirection();
        // Get direction in the entity space
        direction = glm::inverse(getRotation()) * direction;
        
        std::sort(_particlePrimitives.begin(), _particlePrimitives.end(),
                  [&](const ParticlePrimitive& lhs, const ParticlePrimitive& rhs) {
            return glm::dot(lhs.xyz, direction) > glm::dot(rhs.xyz, direction);
        });
    }

    render::PendingChanges pendingChanges;
    pendingChanges.updateItem<ParticlePayload>(_renderItemId, [this](ParticlePayload& payload) {
        // Update particle uniforms
        memcpy(&payload.editParticleUniforms(), &_particleUniforms, sizeof(ParticleUniforms));
        
        // Update particle buffer
        auto particleBuffer = payload.getParticleBuffer();
        size_t numBytes = sizeof(ParticlePrimitive) * _particlePrimitives.size();
        particleBuffer->resize(numBytes);
        if (numBytes == 0) {
            return;
        }
        memcpy(particleBuffer->editData(), _particlePrimitives.data(), numBytes);

        // update transform
        glm::vec3 position = getPosition();
        glm::quat rotation = getRotation();
        Transform transform;
        transform.setTranslation(position);
        transform.setRotation(rotation);
        payload.setModelTransform(transform);

        AABox bounds(_particlesBounds);
        bounds.rotate(rotation);
        bounds.shiftBy(position);
        payload.setBound(bounds);

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
    bool writeToDepthBuffer = false;
    gpu::State::BlendArg destinationColorBlendArg;
    if (_additiveBlending) {
        destinationColorBlendArg = gpu::State::ONE;
    } else {
        destinationColorBlendArg = gpu::State::INV_SRC_ALPHA;
        writeToDepthBuffer = true;
    }
    if (!_untexturedPipeline) {
        auto state = std::make_shared<gpu::State>();
        state->setCullMode(gpu::State::CULL_BACK);
        state->setDepthTest(true, writeToDepthBuffer, gpu::LESS_EQUAL);
        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD,
                                destinationColorBlendArg, gpu::State::FACTOR_ALPHA,
                                gpu::State::BLEND_OP_ADD, gpu::State::ONE);
        auto vertShader = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(untextured_particle_vert)));
        auto fragShader = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(untextured_particle_frag)));
        auto program = gpu::ShaderPointer(gpu::Shader::createProgram(vertShader, fragShader));
        _untexturedPipeline = gpu::PipelinePointer(gpu::Pipeline::create(program, state));
    }
    if (!_texturedPipeline) {
        auto state = std::make_shared<gpu::State>();
        state->setCullMode(gpu::State::CULL_BACK);
       

        bool writeToDepthBuffer = !_additiveBlending;
        state->setDepthTest(true, writeToDepthBuffer, gpu::LESS_EQUAL);
        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD,
                                destinationColorBlendArg, gpu::State::FACTOR_ALPHA,
                                gpu::State::BLEND_OP_ADD, gpu::State::ONE);
        auto vertShader = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(textured_particle_vert)));
        gpu::ShaderPointer fragShader;
        if (_additiveBlending) {
           fragShader = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(textured_particle_frag)));
        } else {
            //If we are sorting and have no additive blending, we want to discard pixels with low alpha to avoid inter-particle entity artifacts
            fragShader = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(textured_particle_alpha_discard_frag)));
        }
        auto program = gpu::ShaderPointer(gpu::Shader::createProgram(vertShader, fragShader));
        _texturedPipeline = gpu::PipelinePointer(gpu::Pipeline::create(program, state));
   
    }
}
