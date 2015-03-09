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

#include "RenderableParticleEffectEntityItem.h"

EntityItem* RenderableParticleEffectEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return new RenderableParticleEffectEntityItem(entityID, properties);
}

RenderableParticleEffectEntityItem::RenderableParticleEffectEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
    ParticleEffectEntityItem(entityItemID, properties) {
    _cacheID = DependencyManager::get<GeometryCache>()->allocateID();
}

void RenderableParticleEffectEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableParticleEffectEntityItem::render");
    assert(getType() == EntityTypes::ParticleEffect);
    float pa_rad = getParticleRadius();

    const float MAX_COLOR = 255.0f;
    glm::vec4 paColor(getColor()[RED_INDEX] / MAX_COLOR, getColor()[GREEN_INDEX] / MAX_COLOR,
                    getColor()[BLUE_INDEX] / MAX_COLOR, getLocalRenderAlpha());

    // Right now we're just iterating over particles and rendering as a cross of four quads.
    // This is pretty dumb, it was quick enough to code up.  Really, there should be many
    // rendering modes, including the all-important textured billboards.

    QVector<glm::vec3>* pointVec = new QVector<glm::vec3>(_paCount * VERTS_PER_PARTICLE);
    quint32 paIter = _paHead;
    while (_paLife[paIter] > 0.0f) {
        int j = paIter * XYZ_STRIDE;

        pointVec->append(glm::vec3(_paPosition[j] - pa_rad, _paPosition[j + 1] + pa_rad, _paPosition[j + 2]));
        pointVec->append(glm::vec3(_paPosition[j] + pa_rad, _paPosition[j + 1] + pa_rad, _paPosition[j + 2]));
        pointVec->append(glm::vec3(_paPosition[j] + pa_rad, _paPosition[j + 1] - pa_rad, _paPosition[j + 2]));
        pointVec->append(glm::vec3(_paPosition[j] - pa_rad, _paPosition[j + 1] - pa_rad, _paPosition[j + 2]));

        pointVec->append(glm::vec3(_paPosition[j] + pa_rad, _paPosition[j + 1] + pa_rad, _paPosition[j + 2]));
        pointVec->append(glm::vec3(_paPosition[j] - pa_rad, _paPosition[j + 1] + pa_rad, _paPosition[j + 2]));
        pointVec->append(glm::vec3(_paPosition[j] - pa_rad, _paPosition[j + 1] - pa_rad, _paPosition[j + 2]));
        pointVec->append(glm::vec3(_paPosition[j] + pa_rad, _paPosition[j + 1] - pa_rad, _paPosition[j + 2]));

        pointVec->append(glm::vec3(_paPosition[j], _paPosition[j + 1] + pa_rad, _paPosition[j + 2] - pa_rad));
        pointVec->append(glm::vec3(_paPosition[j], _paPosition[j + 1] + pa_rad, _paPosition[j + 2] + pa_rad));
        pointVec->append(glm::vec3(_paPosition[j], _paPosition[j + 1] - pa_rad, _paPosition[j + 2] + pa_rad));
        pointVec->append(glm::vec3(_paPosition[j], _paPosition[j + 1] - pa_rad, _paPosition[j + 2] - pa_rad));

        pointVec->append(glm::vec3(_paPosition[j], _paPosition[j + 1] + pa_rad, _paPosition[j + 2] + pa_rad));
        pointVec->append(glm::vec3(_paPosition[j], _paPosition[j + 1] + pa_rad, _paPosition[j + 2] - pa_rad));
        pointVec->append(glm::vec3(_paPosition[j], _paPosition[j + 1] - pa_rad, _paPosition[j + 2] - pa_rad));
        pointVec->append(glm::vec3(_paPosition[j], _paPosition[j + 1] - pa_rad, _paPosition[j + 2] + pa_rad));

        paIter = (paIter + 1) % _maxParticles;
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
};
