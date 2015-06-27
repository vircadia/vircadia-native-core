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
    _numVertices = 0;

}

gpu::PipelinePointer RenderableQuadEntityItem::_pipeline;
gpu::Stream::FormatPointer RenderableQuadEntityItem::_format;

void RenderableQuadEntityItem::createPipeline() {
    static const int COLOR_OFFSET = 12;
    _format.reset(new gpu::Stream::Format());
    _format->setAttribute(gpu::Stream::POSITION, 0, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), 0);
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
    return ((int(c1 * 255.0f) & 0xFF)) |
    ((int(c2 * 255.0f) & 0xFF) << 8) |
    ((int(c3 * 255.0f) & 0xFF) << 16) |
    ((int(255.0f) & 0xFF) << 24);
}

void RenderableQuadEntityItem::updateGeometry() {
    if(_points.size() < 1) {
        return;
    }
    int compactColor = generateColor();
    if (_pointsChanged) {
        _numVertices = 0;
        _verticesBuffer.reset(new gpu::Buffer());
        glm::vec3 point, v1, v2;
        for (int i = 0; i < _points.size(); i++) {
       
            
            if(i % 2 == 0) {
                compactColor = generateColor();
            }
            point = _points.at(i);
            v1 = {point.x - _lineWidth, point.y, point.z};
            v2 = {point.x + _lineWidth, point.y, point.z};
            
            _verticesBuffer->append(sizeof(glm::vec3), (const gpu::Byte*)&v1);
            _verticesBuffer->append(sizeof(int), (gpu::Byte*)&compactColor);
            _verticesBuffer->append(sizeof(glm::vec3), (const gpu::Byte*)&v2);
            _verticesBuffer->append(sizeof(int), (gpu::Byte*)&compactColor);
            _numVertices +=2;
        }
        _pointsChanged = false;
    }
}



void RenderableQuadEntityItem::render(RenderArgs* args) {
    if (_points.size() < 2 ) {
        return;
    }
    if (!_pipeline) {
        createPipeline();
    }
    
 
    PerformanceTimer perfTimer("RenderableQuadEntityItem::render");
    Q_ASSERT(getType() == EntityTypes::Quad);
    
    Q_ASSERT(args->_batch);
    updateGeometry();
    
    gpu::Batch& batch = *args->_batch;
    Transform transform = Transform();
    transform.setTranslation(getPosition());
    batch.setModelTransform(transform);

    batch.setPipeline(_pipeline);
    
    batch.setInputFormat(_format);
    batch.setInputBuffer(0, _verticesBuffer, 0, _format->getChannels().at(0)._stride);
    
    batch.draw(gpu::TRIANGLE_STRIP, _numVertices, 0);
    
    RenderableDebugableEntityItem::render(this, args);
};
