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

#include <gpu/GPUConfig.h>

#include <DependencyManager.h>
#include <DeferredLightingEffect.h>
#include <PerfStat.h>
#include <GeometryCache.h>
#include "EntitiesRendererLogging.h"

#include "RenderableParticleEffectEntityItem.h"

EntityItem* RenderableParticleEffectEntityItem::factory(const QUuid& entityID, const EntityItemProperties& properties) {
    return new RenderableParticleEffectEntityItem(entityID, properties);
}

RenderableParticleEffectEntityItem::RenderableParticleEffectEntityItem(const QUuid& entityItemID,
                                                                       const EntityItemProperties& properties) :
    ParticleEffectEntityItem(entityItemID, properties) {
    _cacheID = DependencyManager::get<GeometryCache>()->allocateID();
}

void RenderableParticleEffectEntityItem::render(RenderArgs* args) {

    assert(getType() == EntityTypes::ParticleEffect);
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

    if (!_texture) {
        renderUntexturedQuads(args);
    } else if (_texture && !_texture->isLoaded()) {
        renderUntexturedQuads(args);
    } else {
        renderTexturedQuads(args);
    }
};

void RenderableParticleEffectEntityItem::renderUntexturedQuads(RenderArgs* args) {

    float particleRadius = getParticleRadius();

    const float MAX_COLOR = 255.0f;
    glm::vec4 particleColor(getColor()[RED_INDEX] / MAX_COLOR,
                            getColor()[GREEN_INDEX] / MAX_COLOR,
                            getColor()[BLUE_INDEX] / MAX_COLOR,
                            getLocalRenderAlpha());

    glm::vec3 upOffset = args->_viewFrustum->getUp() * particleRadius;
    glm::vec3 rightOffset = args->_viewFrustum->getRight() * particleRadius;

    QVector<glm::vec3> vertices(getLivingParticleCount() * VERTS_PER_PARTICLE);
    quint32 count = 0;
    for (quint32 i = _particleHeadIndex; i != _particleTailIndex; i = (i + 1) % _maxParticles) {
        glm::vec3 pos = _particlePositions[i];

        // generate corners of quad, aligned to face the camera
        vertices.append(pos - rightOffset + upOffset);
        vertices.append(pos + rightOffset + upOffset);
        vertices.append(pos + rightOffset - upOffset);
        vertices.append(pos - rightOffset - upOffset);
        count++;
    }

    // just double checking, cause if this invarient is false, we might have memory corruption bugs.
    assert(count == getLivingParticleCount());

    // update geometry cache with all the verts in model coordinates.
    DependencyManager::get<GeometryCache>()->updateVertices(_cacheID, vertices, particleColor);

    glPushMatrix();

        glm::vec3 position = getPosition();
        glTranslatef(position.x, position.y, position.z);
        glm::quat rotation = getRotation();
        glm::vec3 axis = glm::axis(rotation);
        glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);

        glPushMatrix();

            glm::vec3 positionToCenter = getCenter() - position;
            glTranslatef(positionToCenter.x, positionToCenter.y, positionToCenter.z);

            DependencyManager::get<GeometryCache>()->renderVertices(gpu::QUADS, _cacheID);

        glPopMatrix();

    glPopMatrix();
}

static glm::vec3 zSortAxis;
static bool zSort(const glm::vec3& rhs, const glm::vec3& lhs) {
    return glm::dot(rhs, ::zSortAxis) > glm::dot(lhs, ::zSortAxis);
}

void RenderableParticleEffectEntityItem::renderTexturedQuads(RenderArgs* args) {

    float particleRadius = getParticleRadius();

    const float MAX_COLOR = 255.0f;
    glm::vec4 particleColor(getColor()[RED_INDEX] / MAX_COLOR,
                            getColor()[GREEN_INDEX] / MAX_COLOR,
                            getColor()[BLUE_INDEX] / MAX_COLOR,
                            getLocalRenderAlpha());

    QVector<glm::vec3> positions(getLivingParticleCount());
    quint32 count = 0;
    for (quint32 i = _particleHeadIndex; i != _particleTailIndex; i = (i + 1) % _maxParticles) {
        positions.append(_particlePositions[i]);
        count++;
    }

    // just double checking, cause if this invarient is false, we might have memory corruption bugs.
    assert(count == getLivingParticleCount());

    // sort particles back to front
    ::zSortAxis = args->_viewFrustum->getDirection();
    qSort(positions.begin(), positions.end(), zSort);

    QVector<glm::vec3> vertices(getLivingParticleCount() * VERTS_PER_PARTICLE);
    QVector<glm::vec2> textureCoords(getLivingParticleCount() * VERTS_PER_PARTICLE);

    glm::vec3 upOffset = args->_viewFrustum->getUp() * particleRadius;
    glm::vec3 rightOffset = args->_viewFrustum->getRight() * particleRadius;

    for (int i = 0; i < positions.size(); i++) {
        glm::vec3 pos = positions[i];

        // generate corners of quad aligned to face the camera.
        vertices.append(pos - rightOffset + upOffset);
        vertices.append(pos + rightOffset + upOffset);
        vertices.append(pos + rightOffset - upOffset);
        vertices.append(pos - rightOffset - upOffset);

        textureCoords.append(glm::vec2(0, 1));
        textureCoords.append(glm::vec2(1, 1));
        textureCoords.append(glm::vec2(1, 0));
        textureCoords.append(glm::vec2(0, 0));
    }

    // update geometry cache with all the verts in model coordinates.
    DependencyManager::get<GeometryCache>()->updateVertices(_cacheID, vertices, textureCoords, particleColor);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _texture->getID());

    glPushMatrix();

        glm::vec3 position = getPosition();
        glTranslatef(position.x, position.y, position.z);
        glm::quat rotation = getRotation();
        glm::vec3 axis = glm::axis(rotation);
        glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);

        glPushMatrix();

            glm::vec3 positionToCenter = getCenter() - position;
            glTranslatef(positionToCenter.x, positionToCenter.y, positionToCenter.z);

            DependencyManager::get<GeometryCache>()->renderVertices(gpu::QUADS, _cacheID);

        glPopMatrix();

    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

