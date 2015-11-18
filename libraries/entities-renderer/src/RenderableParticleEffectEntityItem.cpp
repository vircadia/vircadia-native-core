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

    ParticlePayload(EntityItemPointer entity) :
        _entity(entity),
        _vertexFormat(std::make_shared<gpu::Stream::Format>()),
        _particleBuffer(std::make_shared<gpu::Buffer>()) {

        _vertexFormat->setAttribute(gpu::Stream::POSITION, 0, gpu::Element::VEC4F_XYZW,
                                    offsetof(ParticlePrimitive, xyzw), gpu::Stream::PER_INSTANCE);
        _vertexFormat->setAttribute(gpu::Stream::COLOR, 0, gpu::Element::COLOR_RGBA_32,
                                    offsetof(ParticlePrimitive, rgba), gpu::Stream::PER_INSTANCE);
    }

    void setPipeline(gpu::PipelinePointer pipeline) { _pipeline = pipeline; }
    const gpu::PipelinePointer& getPipeline() const { return _pipeline; }

    const Transform& getModelTransform() const { return _modelTransform; }
    void setModelTransform(const Transform& modelTransform) { _modelTransform = modelTransform; }

    const AABox& getBound() const { return _bound; }
    void setBound(AABox& bound) { _bound = bound; }

    gpu::BufferPointer getParticleBuffer() { return _particleBuffer; }
    const gpu::BufferPointer& getParticleBuffer() const { return _particleBuffer; }

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
        batch.setInputBuffer(0, _particleBuffer, 0, sizeof(ParticlePrimitive));

        auto numParticles = _particleBuffer->getSize() / sizeof(ParticlePrimitive);
        batch.drawInstanced(numParticles, gpu::TRIANGLE_STRIP, VERTEX_PER_PARTICLE);
    }

protected:
    EntityItemPointer _entity;
    Transform _modelTransform;
    AABox _bound;
    gpu::PipelinePointer _pipeline;
    gpu::Stream::FormatPointer _vertexFormat;
    gpu::BufferPointer _particleBuffer;
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

uint32_t toRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return ((uint32_t)r | (uint32_t)g << 8 | (uint32_t)b << 16 | (uint32_t)a << 24);
}

struct ParticleDetails {
    ParticleDetails(glm::vec3 position, float radius, uint32_t rgba) : position(position), radius(radius), rgba(rgba) { }

    glm::vec3 position;
    float radius;
    uint32_t rgba;
};

void RenderableParticleEffectEntityItem::updateRenderItem() {
    if (!_scene) {
        return;
    }

    // make a copy of each particle's details
    std::vector<ParticleDetails> particleDetails;
    particleDetails.reserve(getLivingParticleCount());
    for (auto& particle : _particles) {
        auto xcolor = particle.color;
        auto alpha = (uint8_t)(glm::clamp(particle.alpha * getLocalRenderAlpha(), 0.0f, 1.0f) * 255.0f);
        auto rgba = toRGBA(xcolor.red, xcolor.green, xcolor.blue, alpha);
        particleDetails.emplace_back(particle.position, particle.radius, rgba);
    }

    // No need to sort if we're doing additive blending
    if (!_additiveBlending) {
        // sort particles back to front
        // NOTE: this is view frustum might be one frame out of date.
        auto direction = AbstractViewStateInterface::instance()->getCurrentViewFrustum()->getDirection();
        // Get direction in the entity space
        direction = glm::inverse(getRotation()) * direction;
        
        std::sort(particleDetails.begin(), particleDetails.end(),
                  [&](const ParticleDetails& lhs, const ParticleDetails& rhs) {
            return glm::dot(lhs.position, direction) > glm::dot(rhs.position, direction);
        });
    }

    // build primitives from particle positions and radiuses
    _particlePrimitives.clear(); // clear primitives
    _particlePrimitives.reserve(particleDetails.size()); // Reserve space
    for (const auto& particle : particleDetails) {
        _particlePrimitives.emplace_back(glm::vec4(particle.position, particle.radius), particle.rgba);
    }

    render::PendingChanges pendingChanges;
    pendingChanges.updateItem<ParticlePayload>(_renderItemId, [this](ParticlePayload& payload) {
        // update particle buffer
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
    }
    else {
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
