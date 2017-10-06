//
//  RenderableParticleEffectEntityItem.cpp
//  interface/src
//
//  Created by Jason Rickwald on 3/2/15.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderableParticleEffectEntityItem.h"

#include <StencilMaskPass.h>

#include <GeometryCache.h>

#include "textured_particle_vert.h"
#include "textured_particle_frag.h"

using namespace render;
using namespace render::entities;

static uint8_t CUSTOM_PIPELINE_NUMBER = 0;
static gpu::Stream::FormatPointer _vertexFormat;
static std::weak_ptr<gpu::Pipeline> _texturedPipeline;

static ShapePipelinePointer shapePipelineFactory(const ShapePlumber& plumber, const ShapeKey& key) {
    auto texturedPipeline = _texturedPipeline.lock();
    if (!texturedPipeline) {
        auto state = std::make_shared<gpu::State>();
        state->setCullMode(gpu::State::CULL_BACK);
        state->setDepthTest(true, false, gpu::LESS_EQUAL);
        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE,
            gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
        PrepareStencil::testMask(*state);

        auto vertShader = gpu::Shader::createVertex(std::string(textured_particle_vert));
        auto fragShader = gpu::Shader::createPixel(std::string(textured_particle_frag));

        auto program = gpu::Shader::createProgram(vertShader, fragShader);
        _texturedPipeline = texturedPipeline = gpu::Pipeline::create(program, state);
    }

    return std::make_shared<render::ShapePipeline>(texturedPipeline, nullptr, nullptr, nullptr);
}

struct GpuParticle {
    GpuParticle(const glm::vec3& xyzIn, const glm::vec2& uvIn) : xyz(xyzIn), uv(uvIn) {}
    glm::vec3 xyz; // Position
    glm::vec2 uv; // Lifetime + seed
};

using GpuParticles = std::vector<GpuParticle>;

ParticleEffectEntityRenderer::ParticleEffectEntityRenderer(const EntityItemPointer& entity) : Parent(entity) {
    ParticleUniforms uniforms;
    _uniformBuffer = std::make_shared<Buffer>(sizeof(ParticleUniforms), (const gpu::Byte*) &uniforms);

    static std::once_flag once;
    std::call_once(once, [] {
        // As we create the first ParticuleSystem entity, let s register its special shapePIpeline factory:
        CUSTOM_PIPELINE_NUMBER = render::ShapePipeline::registerCustomShapePipelineFactory(shapePipelineFactory);
        _vertexFormat = std::make_shared<Format>();
        _vertexFormat->setAttribute(gpu::Stream::POSITION, 0, gpu::Element::VEC3F_XYZ,
            offsetof(GpuParticle, xyz), gpu::Stream::PER_INSTANCE);
        _vertexFormat->setAttribute(gpu::Stream::COLOR, 0, gpu::Element::VEC2F_UV,
            offsetof(GpuParticle, uv), gpu::Stream::PER_INSTANCE);
    });
}

bool ParticleEffectEntityRenderer::needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const {
    entity->checkAndMaybeUpdateQueryAACube();
        
    if (_emitting != entity->getIsEmitting()) {
        return true;
    }

    auto particleProperties = entity->getParticleProperties();
    if (particleProperties != _particleProperties) {
        return true;
    }

    return false;
}

void ParticleEffectEntityRenderer::doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) {
    auto newParticleProperties = entity->getParticleProperties();
    if (!newParticleProperties.valid()) {
        qCWarning(entitiesrenderer) << "Bad particle properties";
        if (!entity->getParticleProperties().valid()) {
            qCWarning(entitiesrenderer) << "Bad particle properties";
        }
    }
    
    if (resultWithReadLock<bool>([&]{ return _particleProperties != newParticleProperties; })) {
        _timeUntilNextEmit = 0;
        withWriteLock([&]{
            _particleProperties = newParticleProperties;
        });
    }
    _emitting = entity->getIsEmitting();

    bool hasTexture = resultWithReadLock<bool>([&]{ return _particleProperties.textures.isEmpty(); });
    if (hasTexture) {
        if (_networkTexture) {
            withWriteLock([&] { 
                _networkTexture.reset();
            });
        }
    } else {
        bool textureNeedsUpdate = resultWithReadLock<bool>([&]{
            return !_networkTexture || _networkTexture->getURL() != QUrl(_particleProperties.textures);
        });
        if (textureNeedsUpdate) {
            withWriteLock([&] { 
                _networkTexture = DependencyManager::get<TextureCache>()->getTexture(_particleProperties.textures);
            });
        }
    }
}

void ParticleEffectEntityRenderer::doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) {
    // Fill in Uniforms structure
    ParticleUniforms particleUniforms;
    withReadLock([&]{
        particleUniforms.radius.start = _particleProperties.radius.range.start;
        particleUniforms.radius.middle = _particleProperties.radius.gradient.target;
        particleUniforms.radius.finish = _particleProperties.radius.range.finish;
        particleUniforms.radius.spread = _particleProperties.radius.gradient.spread;
        particleUniforms.color.start = _particleProperties.getColorStart();
        particleUniforms.color.middle = _particleProperties.getColorMiddle();
        particleUniforms.color.finish = _particleProperties.getColorFinish();
        particleUniforms.color.spread = _particleProperties.getColorSpread();
        particleUniforms.lifespan = _particleProperties.lifespan;
    });
    // Update particle uniforms
    memcpy(&_uniformBuffer.edit<ParticleUniforms>(), &particleUniforms, sizeof(ParticleUniforms));
}

ItemKey ParticleEffectEntityRenderer::getKey() {
    if (_visible) {
        return ItemKey::Builder::transparentShape();
    } else {
        return ItemKey::Builder().withInvisible().build();
    }
}

ShapeKey ParticleEffectEntityRenderer::getShapeKey() {
    return ShapeKey::Builder().withCustom(CUSTOM_PIPELINE_NUMBER).withTranslucent().build();
}

Item::Bound ParticleEffectEntityRenderer::getBound() {
    return _bound;
}

static const size_t VERTEX_PER_PARTICLE = 4;

ParticleEffectEntityRenderer::CpuParticle ParticleEffectEntityRenderer::createParticle(uint64_t now, const Transform& baseTransform, const particle::Properties& particleProperties) {
    CpuParticle particle;

    const auto& accelerationSpread = particleProperties.emission.acceleration.spread;
    const auto& azimuthStart = particleProperties.azimuth.start;
    const auto& azimuthFinish = particleProperties.azimuth.finish;
    const auto& emitDimensions = particleProperties.emission.dimensions;
    const auto& emitAcceleration = particleProperties.emission.acceleration.target;
    auto emitOrientation = particleProperties.emission.orientation;
    const auto& emitRadiusStart = glm::max(particleProperties.radiusStart, EPSILON); // Avoid math complications at center
    const auto& emitSpeed = particleProperties.emission.speed.target;
    const auto& speedSpread = particleProperties.emission.speed.spread;
    const auto& polarStart = particleProperties.polar.start;
    const auto& polarFinish = particleProperties.polar.finish;

    particle.seed = randFloatInRange(-1.0f, 1.0f);
    particle.expiration = now + (uint64_t)(particleProperties.lifespan * USECS_PER_SECOND);
    if (particleProperties.emission.shouldTrail) {
        particle.position = baseTransform.getTranslation();
        emitOrientation = baseTransform.getRotation() * emitOrientation;
    }

    // Position, velocity, and acceleration
    if (polarStart == 0.0f && polarFinish == 0.0f && emitDimensions.z == 0.0f) {
        // Emit along z-axis from position

        particle.velocity = (emitSpeed + 0.2f * speedSpread) * (emitOrientation * Vectors::UNIT_Z);
        particle.acceleration = emitAcceleration + randFloatInRange(-1.0f, 1.0f) * accelerationSpread;

    } else {
        // Emit around point or from ellipsoid
        // - Distribute directions evenly around point
        // - Distribute points relatively evenly over ellipsoid surface
        // - Distribute points relatively evenly within ellipsoid volume

        float elevationMinZ = sin(PI_OVER_TWO - polarFinish);
        float elevationMaxZ = sin(PI_OVER_TWO - polarStart);
        //  float elevation = asin(elevationMinZ + (elevationMaxZ - elevationMinZ) * randFloat());
        float elevation = asin(elevationMinZ + (elevationMaxZ - elevationMinZ) *randFloat());

        float azimuth;
        if (azimuthFinish >= azimuthStart) {
            azimuth = azimuthStart + (azimuthFinish - azimuthStart) *  randFloat();
        } else {
            azimuth = azimuthStart + (TWO_PI + azimuthFinish - azimuthStart) * randFloat();
        }

        glm::vec3 emitDirection;
        if (emitDimensions == Vectors::ZERO) {
            // Point
            emitDirection = glm::quat(glm::vec3(PI_OVER_TWO - elevation, 0.0f, azimuth)) * Vectors::UNIT_Z;
        } else {
            // Ellipsoid
            float radiusScale = 1.0f;
            if (emitRadiusStart < 1.0f) {
                float randRadius =
                    emitRadiusStart + randFloatInRange(0.0f, particle::MAXIMUM_EMIT_RADIUS_START - emitRadiusStart);
                radiusScale = 1.0f - std::pow(1.0f - randRadius, 3.0f);
            }

            glm::vec3 radii = radiusScale * 0.5f * emitDimensions;
            float x = radii.x * glm::cos(elevation) * glm::cos(azimuth);
            float y = radii.y * glm::cos(elevation) * glm::sin(azimuth);
            float z = radii.z * glm::sin(elevation);
            glm::vec3 emitPosition = glm::vec3(x, y, z);
            emitDirection = glm::normalize(glm::vec3(
                radii.x > 0.0f ? x / (radii.x * radii.x) : 0.0f,
                radii.y > 0.0f ? y / (radii.y * radii.y) : 0.0f,
                radii.z > 0.0f ? z / (radii.z * radii.z) : 0.0f
            ));
            particle.position += emitOrientation * emitPosition;
        }

        particle.velocity = (emitSpeed + randFloatInRange(-1.0f, 1.0f) * speedSpread) * (emitOrientation * emitDirection);
        particle.acceleration = emitAcceleration + randFloatInRange(-1.0f, 1.0f) * accelerationSpread;
    }

    return particle;
}

void ParticleEffectEntityRenderer::stepSimulation() {
    if (_lastSimulated == 0) {
        _lastSimulated = usecTimestampNow();
        return;
    }

    const auto now = usecTimestampNow();
    const auto interval = std::min<uint64_t>(USECS_PER_SECOND / 60, now - _lastSimulated);
    _lastSimulated = now;
    
    particle::Properties particleProperties;
    withReadLock([&]{
        particleProperties = _particleProperties;
    });

    if (_emitting && particleProperties.emitting()) {
        uint64_t emitInterval = particleProperties.emitIntervalUsecs();
        if (emitInterval > 0 && interval >= _timeUntilNextEmit) {
            auto timeRemaining = interval;
            while (timeRemaining > _timeUntilNextEmit) {
                // emit particle
                _cpuParticles.push_back(createParticle(now, _modelTransform, particleProperties));
                _timeUntilNextEmit = emitInterval;
                if (emitInterval < timeRemaining) {
                    timeRemaining -= emitInterval;
                }
            }
        } else {
            _timeUntilNextEmit -= interval;
        }
    }

    // Kill any particles that have expired or are over the max size
    while (_cpuParticles.size() > particleProperties.maxParticles || (!_cpuParticles.empty() && _cpuParticles.front().expiration <= now)) {
        _cpuParticles.pop_front();
    }

    const float deltaTime = (float)interval / (float)USECS_PER_SECOND;
    // update the particles 
    for (auto& particle : _cpuParticles) {
        particle.integrate(deltaTime);
    }

    // Build particle primitives
    static GpuParticles gpuParticles;
    gpuParticles.clear();
    gpuParticles.reserve(_cpuParticles.size()); // Reserve space
    std::transform(_cpuParticles.begin(), _cpuParticles.end(), std::back_inserter(gpuParticles), [](const CpuParticle& particle) {
        return GpuParticle(particle.position, glm::vec2(particle.lifetime, particle.seed));
    });

    // Update particle buffer
    auto& particleBuffer = _particleBuffer;
    size_t numBytes = sizeof(GpuParticle) * gpuParticles.size();
    particleBuffer->resize(numBytes);
    if (numBytes != 0) {
        particleBuffer->setData(numBytes, (const gpu::Byte*)gpuParticles.data());
    }
}

void ParticleEffectEntityRenderer::doRender(RenderArgs* args) {
    if (!_visible) {
        return;
    }


    // FIXME migrate simulation to a compute stage
    stepSimulation();

    gpu::Batch& batch = *args->_batch;
    if (_networkTexture && _networkTexture->isLoaded()) {
        batch.setResourceTexture(0, _networkTexture->getGPUTexture());
    } else {
        batch.setResourceTexture(0, DependencyManager::get<TextureCache>()->getWhiteTexture());
    }

    Transform transform; 
    // In trail mode, the particles are created in world space.
    // so we only set a transform if they're not in trail mode
    if (!_particleProperties.emission.shouldTrail) {
        transform = _modelTransform;
        transform.setScale(vec3(1));
    }
    batch.setModelTransform(transform);
    batch.setUniformBuffer(0, _uniformBuffer);
    batch.setInputFormat(_vertexFormat);
    batch.setInputBuffer(0, _particleBuffer, 0, sizeof(GpuParticle));

    auto numParticles = _particleBuffer->getSize() / sizeof(GpuParticle);
    batch.drawInstanced((gpu::uint32)numParticles, gpu::TRIANGLE_STRIP, (gpu::uint32)VERTEX_PER_PARTICLE);
}


