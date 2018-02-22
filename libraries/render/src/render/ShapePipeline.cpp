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
#include "Logging.h"
#include "ShapePipeline.h"

#include <PerfStat.h>

using namespace render;

ShapePipeline::CustomFactoryMap ShapePipeline::_globalCustomFactoryMap;

ShapePipeline::CustomKey ShapePipeline::registerCustomShapePipelineFactory(CustomFactory factory) {  
    ShapePipeline::CustomKey custom = (ShapePipeline::CustomKey) _globalCustomFactoryMap.size() + 1;
    _globalCustomFactoryMap[custom] = factory;
    return custom;  
}


void ShapePipeline::prepare(gpu::Batch& batch, RenderArgs* args) {
    if (_batchSetter) {
        _batchSetter(*this, batch, args);
    }
}

void ShapePipeline::prepareShapeItem(RenderArgs* args, const ShapeKey& key, const Item& shape) {
    if (_itemSetter) {
        _itemSetter(*this, args, shape);
    }
}


ShapeKey::Filter::Builder::Builder() {
    _mask.set(OWN_PIPELINE);
    _mask.set(INVALID);
}

void ShapePlumber::addPipelineHelper(const Filter& filter, ShapeKey key, int bit, const PipelinePointer& pipeline) const {
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
        auto precedent = _pipelineMap.find(key);
        if (precedent != _pipelineMap.end()) {
            qCDebug(renderlogging) << "Key already assigned: " << key;
        }
        _pipelineMap.insert(PipelineMap::value_type(key, pipeline));
    }
}

void ShapePlumber::addPipeline(const Key& key, const gpu::ShaderPointer& program, const gpu::StatePointer& state,
        BatchSetter batchSetter, ItemSetter itemSetter) {
    addPipeline(Filter{key}, program, state, batchSetter, itemSetter);
}

void ShapePlumber::addPipeline(const Filter& filter, const gpu::ShaderPointer& program, const gpu::StatePointer& state,
        BatchSetter batchSetter, ItemSetter itemSetter) {

    ShapeKey key{ filter._flags };


    // don't call makeProgram on shaders that are already made.
    if (program->getNumCompilationAttempts() < 1) {
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
        slotBindings.insert(gpu::Shader::Binding(std::string("keyLightBuffer"), Slot::BUFFER::KEY_LIGHT));
        slotBindings.insert(gpu::Shader::Binding(std::string("lightBuffer"), Slot::BUFFER::LIGHT));
        slotBindings.insert(gpu::Shader::Binding(std::string("lightAmbientBuffer"), Slot::BUFFER::LIGHT_AMBIENT_BUFFER));
        slotBindings.insert(gpu::Shader::Binding(std::string("skyboxMap"), Slot::MAP::LIGHT_AMBIENT));
        slotBindings.insert(gpu::Shader::Binding(std::string("fadeMaskMap"), Slot::MAP::FADE_MASK));
        slotBindings.insert(gpu::Shader::Binding(std::string("fadeParametersBuffer"), Slot::BUFFER::FADE_PARAMETERS));
        slotBindings.insert(gpu::Shader::Binding(std::string("hazeBuffer"), Slot::BUFFER::HAZE_MODEL));

        if (key.isTranslucent()) {
            slotBindings.insert(gpu::Shader::Binding(std::string("clusterGridBuffer"), Slot::BUFFER::LIGHT_CLUSTER_GRID_CLUSTER_GRID_SLOT));
            slotBindings.insert(gpu::Shader::Binding(std::string("clusterContentBuffer"), Slot::BUFFER::LIGHT_CLUSTER_GRID_CLUSTER_CONTENT_SLOT));
            slotBindings.insert(gpu::Shader::Binding(std::string("frustumGridBuffer"), Slot::BUFFER::LIGHT_CLUSTER_GRID_FRUSTUM_GRID_SLOT));
        }

        gpu::Shader::makeProgram(*program, slotBindings);
    }

    auto locations = std::make_shared<Locations>();

    locations->albedoTextureUnit = program->getTextures().findLocation("albedoMap");
    locations->roughnessTextureUnit = program->getTextures().findLocation("roughnessMap");
    locations->normalTextureUnit = program->getTextures().findLocation("normalMap");
    locations->metallicTextureUnit = program->getTextures().findLocation("metallicMap");
    locations->emissiveTextureUnit = program->getTextures().findLocation("emissiveMap");
    locations->occlusionTextureUnit = program->getTextures().findLocation("occlusionMap");
    locations->lightingModelBufferUnit = program->getUniformBuffers().findLocation("lightingModelBuffer");
    locations->skinClusterBufferUnit = program->getUniformBuffers().findLocation("skinClusterBuffer");
    locations->materialBufferUnit = program->getUniformBuffers().findLocation("materialBuffer");
    locations->texMapArrayBufferUnit = program->getUniformBuffers().findLocation("texMapArrayBuffer");
    locations->keyLightBufferUnit = program->getUniformBuffers().findLocation("keyLightBuffer");
    locations->lightBufferUnit = program->getUniformBuffers().findLocation("lightBuffer");
    locations->lightAmbientBufferUnit = program->getUniformBuffers().findLocation("lightAmbientBuffer");
    locations->lightAmbientMapUnit = program->getTextures().findLocation("skyboxMap");
    locations->fadeMaskTextureUnit = program->getTextures().findLocation("fadeMaskMap");
    locations->fadeParameterBufferUnit = program->getUniformBuffers().findLocation("fadeParametersBuffer");
    locations->hazeParameterBufferUnit = program->getUniformBuffers().findLocation("hazeParametersBuffer");
    if (key.isTranslucent()) {
        locations->lightClusterGridBufferUnit = program->getUniformBuffers().findLocation("clusterGridBuffer");
        locations->lightClusterContentBufferUnit = program->getUniformBuffers().findLocation("clusterContentBuffer");
        locations->lightClusterFrustumBufferUnit = program->getUniformBuffers().findLocation("frustumGridBuffer");
    } else {
        locations->lightClusterGridBufferUnit = -1;
        locations->lightClusterContentBufferUnit = -1;
        locations->lightClusterFrustumBufferUnit = -1;
    }

    auto gpuPipeline = gpu::Pipeline::create(program, state);
    auto shapePipeline = std::make_shared<Pipeline>(gpuPipeline, locations, batchSetter, itemSetter);
    addPipelineHelper(filter, key, 0, shapePipeline);
}

const ShapePipelinePointer ShapePlumber::pickPipeline(RenderArgs* args, const Key& key) const {
    assert(!_pipelineMap.empty());
    assert(args);
    assert(args->_batch);

    PerformanceTimer perfTimer("ShapePlumber::pickPipeline");

    auto pipelineIterator = _pipelineMap.find(key);
    if (pipelineIterator == _pipelineMap.end()) {
        // The first time we can't find a pipeline, we should try things to solve that
        if (_missingKeys.find(key) == _missingKeys.end()) {
            if (key.isCustom()) {
                auto factoryIt = ShapePipeline::_globalCustomFactoryMap.find(key.getCustom());
                if ((factoryIt != ShapePipeline::_globalCustomFactoryMap.end()) && (factoryIt)->second) {
                    // found a factory for the custom key, can now generate a shape pipeline for this case:
                    addPipelineHelper(Filter(key), key, 0, (factoryIt)->second(*this, key, *(args->_batch)));

                    return pickPipeline(args, key);
                } else {
                    qCDebug(renderlogging) << "ShapePlumber::Couldn't find a custom pipeline factory for " << key.getCustom() << " key is: " << key;
                }
            }

           _missingKeys.insert(key);
            qCDebug(renderlogging) << "ShapePlumber::Couldn't find a pipeline for" << key;
        }
        return PipelinePointer(nullptr);
    }

    PipelinePointer shapePipeline(pipelineIterator->second);

    // Setup the one pipeline (to rule them all)
    args->_batch->setPipeline(shapePipeline->pipeline);

    // Run the pipeline's BatchSetter on the passed in batch
    if (shapePipeline->_batchSetter) {
        shapePipeline->_batchSetter(*shapePipeline, *(args->_batch), args);
    }

    return shapePipeline;
}
