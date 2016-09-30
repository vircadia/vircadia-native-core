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

void ShapePipeline::prepare(gpu::Batch& batch) {
    if (batchSetter) {
        batchSetter(*this, batch);
    }
}

ShapeKey::Filter::Builder::Builder() {
    _mask.set(OWN_PIPELINE);
    _mask.set(INVALID);
}

void ShapePlumber::addPipelineHelper(const Filter& filter, ShapeKey key, int bit, const PipelinePointer& pipeline) {
    // Iterate over all keys
    if (bit < (int)ShapeKey::FlagBit::NUM_FLAGS) {
        addPipelineHelper(filter, key, bit + 1, pipeline);
        if (!filter._mask[bit]) {
            // Toggle bits set as insignificant in filter._mask 
            key._flags.flip(bit);
            addPipelineHelper(filter, key, bit + 1, pipeline);
        }
    } else {
        // Add the brand new pipeline and cache its location in the lib
        _pipelineMap.insert(PipelineMap::value_type(key, pipeline));
    }
}

void ShapePlumber::addPipeline(const Key& key, const gpu::ShaderPointer& program, const gpu::StatePointer& state,
        BatchSetter batchSetter) {
    addPipeline(Filter{key}, program, state, batchSetter);
}

void ShapePlumber::addPipeline(const Filter& filter, const gpu::ShaderPointer& program, const gpu::StatePointer& state,
        BatchSetter batchSetter) {
    gpu::Shader::BindingSet slotBindings;
    slotBindings.insert(gpu::Shader::Binding(std::string("lightingModelBuffer"), Slot::BUFFER::LIGHTING_MODEL));
    slotBindings.insert(gpu::Shader::Binding(std::string("skinClusterBuffer"), Slot::BUFFER::SKINNING));
    slotBindings.insert(gpu::Shader::Binding(std::string("materialBuffer"), Slot::BUFFER::MATERIAL));
    slotBindings.insert(gpu::Shader::Binding(std::string("texMapArrayBuffer"), Slot::BUFFER::TEXMAPARRAY));
    slotBindings.insert(gpu::Shader::Binding(std::string("albedoMap"), Slot::MAP::ALBEDO));
    slotBindings.insert(gpu::Shader::Binding(std::string("roughnessMap"), Slot::MAP::ROUGHNESS));
    slotBindings.insert(gpu::Shader::Binding(std::string("normalMap"), Slot::MAP::NORMAL));
    slotBindings.insert(gpu::Shader::Binding(std::string("metallicMap"), Slot::MAP::METALLIC));
    slotBindings.insert(gpu::Shader::Binding(std::string("emissiveMap"), Slot::MAP::EMISSIVE_LIGHTMAP));
    slotBindings.insert(gpu::Shader::Binding(std::string("occlusionMap"), Slot::MAP::OCCLUSION));
    slotBindings.insert(gpu::Shader::Binding(std::string("scatteringMap"), Slot::MAP::SCATTERING));
    slotBindings.insert(gpu::Shader::Binding(std::string("lightBuffer"), Slot::BUFFER::LIGHT));
    slotBindings.insert(gpu::Shader::Binding(std::string("lightAmbientBuffer"), Slot::BUFFER::LIGHT_AMBIENT_BUFFER));
    slotBindings.insert(gpu::Shader::Binding(std::string("skyboxMap"), Slot::MAP::LIGHT_AMBIENT));
    slotBindings.insert(gpu::Shader::Binding(std::string("normalFittingMap"), Slot::NORMAL_FITTING));

    gpu::Shader::makeProgram(*program, slotBindings);

    auto locations = std::make_shared<Locations>();
    locations->normalFittingMapUnit = program->getTextures().findLocation("normalFittingMap");
    if (program->getTextures().findLocation("normalFittingMap") > -1) {
        locations->normalFittingMapUnit = program->getTextures().findLocation("normalFittingMap");

    }
    locations->albedoTextureUnit = program->getTextures().findLocation("albedoMap");
    locations->roughnessTextureUnit = program->getTextures().findLocation("roughnessMap");
    locations->normalTextureUnit = program->getTextures().findLocation("normalMap");
    locations->metallicTextureUnit = program->getTextures().findLocation("metallicMap");
    locations->emissiveTextureUnit = program->getTextures().findLocation("emissiveMap");
    locations->occlusionTextureUnit = program->getTextures().findLocation("occlusionMap");
    locations->lightingModelBufferUnit = program->getBuffers().findLocation("lightingModelBuffer");
    locations->skinClusterBufferUnit = program->getBuffers().findLocation("skinClusterBuffer");
    locations->materialBufferUnit = program->getBuffers().findLocation("materialBuffer");
    locations->texMapArrayBufferUnit = program->getBuffers().findLocation("texMapArrayBuffer");
    locations->lightBufferUnit = program->getBuffers().findLocation("lightBuffer");
    locations->lightAmbientBufferUnit = program->getBuffers().findLocation("lightAmbientBuffer");
    locations->lightAmbientMapUnit = program->getTextures().findLocation("skyboxMap");
    
    ShapeKey key{filter._flags};
    auto gpuPipeline = gpu::Pipeline::create(program, state);
    auto shapePipeline = std::make_shared<Pipeline>(gpuPipeline, locations, batchSetter);
    addPipelineHelper(filter, key, 0, shapePipeline);
}

const ShapePipelinePointer ShapePlumber::pickPipeline(RenderArgs* args, const Key& key) const {
    assert(!_pipelineMap.empty());
    assert(args);
    assert(args->_batch);

    PerformanceTimer perfTimer("ShapePlumber::pickPipeline");

    const auto& pipelineIterator = _pipelineMap.find(key);
    if (pipelineIterator == _pipelineMap.end()) {
        // The first time we can't find a pipeline, we should log it
        if (_missingKeys.find(key) == _missingKeys.end()) {
            _missingKeys.insert(key);
            qDebug() << "Couldn't find a pipeline for" << key;
        }
        return PipelinePointer(nullptr);
    }

    PipelinePointer shapePipeline(pipelineIterator->second);
    auto& batch = args->_batch;

    // Setup the one pipeline (to rule them all)
    batch->setPipeline(shapePipeline->pipeline);

    // Run the pipeline's BatchSetter on the passed in batch
    if (shapePipeline->batchSetter) {
        shapePipeline->batchSetter(*shapePipeline, *batch);
    }

    return shapePipeline;
}
