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
#include "EntitiesRendererLogging.h"

#include "RenderableParticleEffectEntityItem.h"

EntityItemPointer RenderableParticleEffectEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return std::make_shared<RenderableParticleEffectEntityItem>(entityID, properties);
}

RenderableParticleEffectEntityItem::RenderableParticleEffectEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
    ParticleEffectEntityItem(entityItemID, properties) {
    _cacheID = DependencyManager::get<GeometryCache>()->allocateID();
}

void RenderableParticleEffectEntityItem::render(RenderArgs* args) {
    Q_ASSERT(getType() == EntityTypes::ParticleEffect);
    PerformanceTimer perfTimer("RenderableParticleEffectEntityItem::render");

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

    bool textured = _texture && _texture->isLoaded();
    updateQuads(args, textured);
    
    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;
    if (textured) {
        batch.setResourceTexture(0, _texture->getGPUTexture());
    }
    batch.setModelTransform(getTransformToCenter());
    DependencyManager::get<DeferredLightingEffect>()->bindSimpleProgram(batch, textured);
    DependencyManager::get<GeometryCache>()->renderVertices(batch, gpu::QUADS, _cacheID);
};

static glm::vec3 zSortAxis;
static bool zSort(const glm::vec3& rhs, const glm::vec3& lhs) {
    return glm::dot(rhs, ::zSortAxis) > glm::dot(lhs, ::zSortAxis);
}

void RenderableParticleEffectEntityItem::updateQuads(RenderArgs* args, bool textured) {
    float particleRadius = getParticleRadius();
    glm::vec4 particleColor(toGlm(getXColor()), getLocalRenderAlpha());
    
    glm::vec3 upOffset = args->_viewFrustum->getUp() * particleRadius;
    glm::vec3 rightOffset = args->_viewFrustum->getRight() * particleRadius;
    
    QVector<glm::vec3> vertices;
    QVector<glm::vec3> positions;
    QVector<glm::vec2> textureCoords;
    vertices.reserve(getLivingParticleCount() * VERTS_PER_PARTICLE);
    
    if (textured) {
        textureCoords.reserve(getLivingParticleCount() * VERTS_PER_PARTICLE);
    }
    positions.reserve(getLivingParticleCount());
   
    
    for (quint32 i = _particleHeadIndex; i != _particleTailIndex; i = (i + 1) % _maxParticles) {
        positions.append(_particlePositions[i]);
        if (textured) {        
            textureCoords.append(glm::vec2(0, 1));
            textureCoords.append(glm::vec2(1, 1));
            textureCoords.append(glm::vec2(1, 0));
            textureCoords.append(glm::vec2(0, 0));
        }
    }
        
    // sort particles back to front
    ::zSortAxis = args->_viewFrustum->getDirection();
    qSort(positions.begin(), positions.end(), zSort);
    
    for (int i = 0; i < positions.size(); i++) {
        glm::vec3 pos = (textured) ? positions[i] : _particlePositions[i];

        // generate corners of quad aligned to face the camera.
        vertices.append(pos + rightOffset + upOffset);
        vertices.append(pos - rightOffset + upOffset);
        vertices.append(pos - rightOffset - upOffset);
        vertices.append(pos + rightOffset - upOffset);
   
    }
    
    if (textured) {
        DependencyManager::get<GeometryCache>()->updateVertices(_cacheID, vertices, textureCoords, particleColor);
    } else {
        DependencyManager::get<GeometryCache>()->updateVertices(_cacheID, vertices, particleColor);
    }
}

