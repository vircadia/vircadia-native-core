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

EntityItem* RenderableParticleEffectEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return new RenderableParticleEffectEntityItem(entityID, properties);
}

RenderableParticleEffectEntityItem::RenderableParticleEffectEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
    ParticleEffectEntityItem(entityItemID, properties) {
    _cacheID = DependencyManager::get<GeometryCache>()->allocateID();
}

void RenderableParticleEffectEntityItem::render(RenderArgs* args) {

    assert(getType() == EntityTypes::ParticleEffect);
    PerformanceTimer perfTimer("RenderableParticleEffectEntityItem::render");

    if (_texturesChangedFlag)
    {
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

    float pa_rad = getParticleRadius();

    const float MAX_COLOR = 255.0f;
    glm::vec4 paColor(getColor()[RED_INDEX] / MAX_COLOR, getColor()[GREEN_INDEX] / MAX_COLOR,
                    getColor()[BLUE_INDEX] / MAX_COLOR, getLocalRenderAlpha());

    glm::vec3 up_offset = args->_viewFrustum->getUp() * pa_rad;
    glm::vec3 right_offset = args->_viewFrustum->getRight() * pa_rad;

    QVector<glm::vec3>* pointVec = new QVector<glm::vec3>(_paCount * VERTS_PER_PARTICLE);
    quint32 paCount = 0;
    quint32 paIter = _paHead;
    while (_paLife[paIter] > 0.0f && paCount < _maxParticles) {
        int j = paIter * XYZ_STRIDE;

        glm::vec3 pos(_paPosition[j], _paPosition[j + 1], _paPosition[j + 2]);

        pointVec->append(pos - right_offset + up_offset);
        pointVec->append(pos + right_offset + up_offset);
        pointVec->append(pos + right_offset - up_offset);
        pointVec->append(pos - right_offset - up_offset);

        paIter = (paIter + 1) % _maxParticles;
        paCount++;
    }

    DependencyManager::get<GeometryCache>()->updateVertices(_cacheID, *pointVec, paColor);

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

    delete pointVec;
}

static glm::vec3 s_dir;
static bool zSort(const glm::vec3& rhs, const glm::vec3& lhs) {
    return glm::dot(rhs, s_dir) > glm::dot(lhs, s_dir);
}

void RenderableParticleEffectEntityItem::renderTexturedQuads(RenderArgs* args) {

    float pa_rad = getParticleRadius();

    const float MAX_COLOR = 255.0f;
    glm::vec4 paColor(getColor()[RED_INDEX] / MAX_COLOR, getColor()[GREEN_INDEX] / MAX_COLOR,
                      getColor()[BLUE_INDEX] / MAX_COLOR, getLocalRenderAlpha());

    QVector<glm::vec3>* posVec = new QVector<glm::vec3>(_paCount);
    quint32 paCount = 0;
    quint32 paIter = _paHead;
    while (_paLife[paIter] > 0.0f && paCount < _maxParticles) {
        int j = paIter * XYZ_STRIDE;
        posVec->append(glm::vec3(_paPosition[j], _paPosition[j + 1], _paPosition[j + 2]));

        paIter = (paIter + 1) % _maxParticles;
        paCount++;
    }

    // sort particles back to front
    qSort(posVec->begin(), posVec->end(), zSort);

    QVector<glm::vec3>* vertVec = new QVector<glm::vec3>(_paCount * VERTS_PER_PARTICLE);
    QVector<glm::vec2>* texCoordVec = new QVector<glm::vec2>(_paCount * VERTS_PER_PARTICLE);

    s_dir = args->_viewFrustum->getDirection();
    glm::vec3 up_offset = args->_viewFrustum->getUp() * pa_rad;
    glm::vec3 right_offset = args->_viewFrustum->getRight() * pa_rad;

    for (int i = 0; i < posVec->size(); i++) {
        glm::vec3 pos = posVec->at(i);

        vertVec->append(pos - right_offset + up_offset);
        vertVec->append(pos + right_offset + up_offset);
        vertVec->append(pos + right_offset - up_offset);
        vertVec->append(pos - right_offset - up_offset);

        texCoordVec->append(glm::vec2(0, 1));
        texCoordVec->append(glm::vec2(1, 1));
        texCoordVec->append(glm::vec2(1, 0));
        texCoordVec->append(glm::vec2(0, 0));
    }

    DependencyManager::get<GeometryCache>()->updateVertices(_cacheID, *vertVec, *texCoordVec, paColor);

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

    delete posVec;
    delete vertVec;
    delete texCoordVec;
}

