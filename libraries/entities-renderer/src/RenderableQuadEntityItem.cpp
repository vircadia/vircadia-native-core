//
//  RenderableQuadEntityItem.cpp
//  libraries/entities-renderer/src/
//
//  Created by Eric Levin on 6/22/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/quaternion.hpp>

#include <gpu/GPUConfig.h>
#include <GeometryCache.h>

#include <DeferredLightingEffect.h>
#include <PerfStat.h>

#include "RenderableQuadEntityItem.h"


EntityItemPointer RenderableQuadEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return EntityItemPointer(new RenderableQuadEntityItem(entityID, properties));
}

RenderableQuadEntityItem::RenderableQuadEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
QuadEntityItem(entityItemID, properties) {
    _format.reset(new gpu::Stream::Format());
    _format->setAttribute(gpu::Stream::POSITION, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), 0);
    _numVertices = 0;
}


void RenderableQuadEntityItem::updateGeometry() {
    if(_quadVertices.size() < 1) {
        return;
    }
//    qDebug() << "num points: " << _points.size();
//    qDebug() << "num quad vertices" << _quadVertices.size();
    if (_pointsChanged) {
        _numVertices = 0;
        _verticesBuffer.reset(new gpu::Buffer());
        _verticesBuffer->append(sizeof(glm::vec3), (const gpu::Byte*)&_quadVertices.at(0));
        _verticesBuffer->append(sizeof(glm::vec3), (const gpu::Byte*)&_quadVertices.at(1));
        _verticesBuffer->append(sizeof(glm::vec3), (const gpu::Byte*)&_quadVertices.at(2));
        _verticesBuffer->append(sizeof(glm::vec3), (const gpu::Byte*)&_quadVertices.at(3));
        _numVertices = 4;
        glm::vec3 point, v1;
        for (int i = 1; i < _points.size(); i++) {
            point = _points.at(i);
            if(i % 2 == 0) {
                v1 = {point.x - _lineWidth, point.y, point.z};
            } else {
                v1 = {point.x + _lineWidth, point.y, point.z};
            }

            _verticesBuffer->append(sizeof(glm::vec3), (const gpu::Byte*)&v1);
            _numVertices ++;
        }
        _pointsChanged = false;
    }
}

void RenderableQuadEntityItem::render(RenderArgs* args) {
    if (_quadVertices.size() < 4 ) {
        return;
    }
    PerformanceTimer perfTimer("RenderableQuadEntityItem::render");
    Q_ASSERT(getType() == EntityTypes::Quad);
    
    Q_ASSERT(args->_batch);
    updateGeometry();
    
    gpu::Batch& batch = *args->_batch;
    Transform transform = Transform();
    transform.setTranslation(getPosition());
    batch.setModelTransform(transform);
    

    DependencyManager::get<DeferredLightingEffect>()->bindSimpleProgram(batch);
    batch.setInputFormat(_format);
    batch.setInputBuffer(0, _verticesBuffer, 0, _format->getChannels().at(0)._stride);
    batch.draw(gpu::TRIANGLE_STRIP, _numVertices, 0);
    

    
    RenderableDebugableEntityItem::render(this, args);
};
