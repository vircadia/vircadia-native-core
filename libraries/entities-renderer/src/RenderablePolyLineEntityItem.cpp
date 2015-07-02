//
//  RenderablePolyLineEntityItem.cpp
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

#include "RenderablePolyLineEntityItem.h"


EntityItemPointer RenderablePolyLineEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return EntityItemPointer(new RenderablePolyLineEntityItem(entityID, properties));
}

RenderablePolyLineEntityItem::RenderablePolyLineEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
PolyLineEntityItem(entityItemID, properties) {
    _numVertices = 0;

}

gpu::PipelinePointer RenderablePolyLineEntityItem::_pipeline;
gpu::Stream::FormatPointer RenderablePolyLineEntityItem::_format;

void RenderablePolyLineEntityItem::createPipeline() {
    static const int NORMAL_OFFSET = 12;
    static const int COLOR_OFFSET = 24;
    _format.reset(new gpu::Stream::Format());
    _format->setAttribute(gpu::Stream::POSITION, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), 0);
    _format->setAttribute(gpu::Stream::NORMAL, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), NORMAL_OFFSET);
    _format->setAttribute(gpu::Stream::COLOR, 0, gpu::Element(gpu::VEC4, gpu::UINT8, gpu::RGBA), COLOR_OFFSET);
    
    auto VS = DependencyManager::get<DeferredLightingEffect>()->getSimpleVertexShader();
    auto PS = DependencyManager::get<DeferredLightingEffect>()->getSimplePixelShader();
    gpu::ShaderPointer program = gpu::ShaderPointer(gpu::Shader::createProgram(VS, PS));
    
    gpu::Shader::BindingSet slotBindings;
    gpu::Shader::makeProgram(*program, slotBindings);
    
    gpu::StatePointer state = gpu::StatePointer(new gpu::State());
    //state->setCullMode(gpu::State::CULL_BACK);
    state->setDepthTest(true, true, gpu::LESS_EQUAL);
    state->setBlendFunction(false,
                            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
                            gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
   _pipeline = gpu::PipelinePointer(gpu::Pipeline::create(program, state));
}
int generateColor() {
    float c1 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    float c2 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    float c3 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    return ((int(0.7 * 255.0f) & 0xFF)) |
    ((int(0.3 * 255.0f) & 0xFF) << 8) |
    ((int(0.6 * 255.0f) & 0xFF) << 16) |
    ((int(255.0f) & 0xFF) << 24);
}

void RenderablePolyLineEntityItem::updateGeometry() {
    int compactColor = generateColor();
    _numVertices = 0;
    _verticesBuffer.reset(new gpu::Buffer());
    int vertexIndex = 0;
    for (int i = 0; i < _normals.size(); i++) {
        compactColor = generateColor();
        _verticesBuffer->append(sizeof(glm::vec3), (const gpu::Byte*)&_vertices.at(vertexIndex));
        vertexIndex++;
        _verticesBuffer->append(sizeof(glm::vec3), (const gpu::Byte*)&_normals.at(i));
        _verticesBuffer->append(sizeof(int), (gpu::Byte*)&_color);
        
        _verticesBuffer->append(sizeof(glm::vec3), (const gpu::Byte*)&_vertices.at(vertexIndex));
        vertexIndex++;
        _verticesBuffer->append(sizeof(glm::vec3), (const gpu::Byte*)&_normals.at(i));
        _verticesBuffer->append(sizeof(int), (gpu::Byte*)_color);
        
        _numVertices +=2;
    }
    _pointsChanged = false;
    
    _pointsChanged = false;
    
}

void RenderablePolyLineEntityItem::render(RenderArgs* args) {
    QWriteLocker lock(&_quadReadWriteLock);
    if (_points.size() < 2  || _vertices.size() != _normals.size() * 2) {
        return;
    }
    
    if (!_pipeline) {
        createPipeline();
    }
 
    PerformanceTimer perfTimer("RenderablePolyLineEntityItem::render");
    Q_ASSERT(getType() == EntityTypes::PolyLine);
    
    Q_ASSERT(args->_batch);
    if (_pointsChanged) {
        updateGeometry();
    }

    
    gpu::Batch& batch = *args->_batch;
    Transform transform = Transform();
    transform.setTranslation(getPosition());
    transform.setRotation(getRotation());
    batch.setModelTransform(transform);


    batch.setPipeline(_pipeline);
    
    batch.setInputFormat(_format);
    batch.setInputBuffer(0, _verticesBuffer, 0, _format->getChannels().at(0)._stride);
     
    batch.draw(gpu::TRIANGLE_STRIP, _numVertices, 0);
    
    RenderableDebugableEntityItem::render(this, args);
};
