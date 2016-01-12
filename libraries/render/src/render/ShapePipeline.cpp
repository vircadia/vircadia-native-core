//
//  ShapePipeline.cpp
//  render/src/render
//
//  Created by Zach Pomerantz on 12/31/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DependencyManager.h"

#include "ShapePipeline.h"

#include <PerfStat.h>

using namespace render;

ShapeKey::Filter::Builder::Builder() {
    _mask.set(OWN_PIPELINE);
    _mask.set(INVALID);
}

void defaultStateSetter(ShapeKey key, gpu::State& state) {
    // Cull backface
    state.setCullMode(gpu::State::CULL_BACK);

    // Z test depends on transparency
    state.setDepthTest(true, !key.isTranslucent(), gpu::LESS_EQUAL);

    // Blend if transparent
    state.setBlendFunction(key.isTranslucent(),
        // For transparency, keep the highlight intensity
        gpu::State::ONE, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
        gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);

    // Add a wireframe version
    if (!key.isWireFrame()) {
        state.setFillMode(gpu::State::FILL_LINE);
    }
}

void defaultBatchSetter(gpu::Batch& batch, ShapePipelinePointer pipeline) {
}

ShapePlumber::ShapePlumber() : _stateSetter{ defaultStateSetter }, _batchSetter{ defaultBatchSetter } {}
ShapePlumber::ShapePlumber(BatchSetter batchSetter) : _stateSetter{ defaultStateSetter }, _batchSetter{ batchSetter } {}

void ShapePlumber::addPipelineHelper(const Filter& filter, ShapeKey key, int bit, gpu::ShaderPointer program, LocationsPointer locations) {
    // Iterate over all keys, toggling only bits set as significant in filter._mask
    if (bit < (int)ShapeKey::FlagBit::NUM_FLAGS) {
        addPipelineHelper(filter, key, ++bit, program, locations);
        if (filter._mask[bit]) {
            key._flags.flip(bit);
            addPipelineHelper(filter, key, ++bit, program, locations);
        }
    } else {
        auto state = std::make_shared<gpu::State>();
        _stateSetter(key, *state);
    
        // Add the brand new pipeline and cache its location in the lib
        auto pipeline = gpu::Pipeline::create(program, state);
        _pipelineMap.insert(PipelineMap::value_type(key, std::make_shared<Pipeline>(pipeline, locations)));
    }
}

void ShapePlumber::addPipeline(const Key& key, gpu::ShaderPointer& vertexShader, gpu::ShaderPointer& pixelShader) {
    addPipeline(Filter{key}, vertexShader, pixelShader);
}

void ShapePlumber::addPipeline(const Filter& filter, gpu::ShaderPointer& vertexShader, gpu::ShaderPointer& pixelShader) {
    gpu::Shader::BindingSet slotBindings;
    slotBindings.insert(gpu::Shader::Binding(std::string("skinClusterBuffer"), Slot::SKINNING_GPU));
    slotBindings.insert(gpu::Shader::Binding(std::string("materialBuffer"), Slot::MATERIAL_GPU));
    slotBindings.insert(gpu::Shader::Binding(std::string("diffuseMap"), Slot::DIFFUSE_MAP));
    slotBindings.insert(gpu::Shader::Binding(std::string("normalMap"), Slot::NORMAL_MAP));
    slotBindings.insert(gpu::Shader::Binding(std::string("specularMap"), Slot::SPECULAR_MAP));
    slotBindings.insert(gpu::Shader::Binding(std::string("emissiveMap"), Slot::LIGHTMAP_MAP));
    slotBindings.insert(gpu::Shader::Binding(std::string("lightBuffer"), Slot::LIGHT_BUFFER));
    slotBindings.insert(gpu::Shader::Binding(std::string("normalFittingMap"), Slot::NORMAL_FITTING_MAP));

    gpu::ShaderPointer program = gpu::Shader::createProgram(vertexShader, pixelShader);
    gpu::Shader::makeProgram(*program, slotBindings);

    auto locations = std::make_shared<Locations>();
    locations->texcoordMatrices = program->getUniforms().findLocation("texcoordMatrices");
    locations->emissiveParams = program->getUniforms().findLocation("emissiveParams");
    locations->normalFittingMapUnit = program->getTextures().findLocation("normalFittingMap");
    locations->diffuseTextureUnit = program->getTextures().findLocation("diffuseMap");
    locations->normalTextureUnit = program->getTextures().findLocation("normalMap");
    locations->specularTextureUnit = program->getTextures().findLocation("specularMap");
    locations->emissiveTextureUnit = program->getTextures().findLocation("emissiveMap");
    locations->skinClusterBufferUnit = program->getBuffers().findLocation("skinClusterBuffer");
    locations->materialBufferUnit = program->getBuffers().findLocation("materialBuffer");
    locations->lightBufferUnit = program->getBuffers().findLocation("lightBuffer");

    ShapeKey key{filter._flags};
    addPipelineHelper(filter, key, 0, program, locations);
}

const ShapePipelinePointer ShapePlumber::pickPipeline(RenderArgs* args, const Key& key) const {
    assert(!_pipelineMap.empty());
    assert(args);
    assert(args->_batch);

    PerformanceTimer perfTimer("ShapePlumber::pickPipeline");

    const auto& pipelineIterator = _pipelineMap.find(key);
    if (pipelineIterator == _pipelineMap.end()) {
        qDebug() << "Couldn't find a pipeline from ShapeKey ?" << key;
        return PipelinePointer(nullptr);
    }

    PipelinePointer shapePipeline(pipelineIterator->second);
    auto& batch = args->_batch;
    _batchSetter(*batch, shapePipeline);

    // Setup the one pipeline (to rule them all)
    batch->setPipeline(shapePipeline->pipeline);

    return shapePipeline;
}
