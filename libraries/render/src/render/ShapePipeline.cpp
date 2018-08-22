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

#include "ShapePipeline.h"

#include <PerfStat.h>
#include <DependencyManager.h>
#include <gpu/ShaderConstants.h>
#include <graphics/ShaderConstants.h>
#include <../render-utils/src/render-utils/ShaderConstants.h>

#include "Logging.h"


using namespace render;

namespace ru {
    using render_utils::slot::texture::Texture;
    using render_utils::slot::buffer::Buffer;
}

namespace gr {
    using graphics::slot::texture::Texture;
    using graphics::slot::buffer::Buffer;
}

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
    auto locations = std::make_shared<Locations>();
    locations->albedoTextureUnit = program->getTextures().isValid(graphics::slot::texture::MaterialAlbedo);
    locations->roughnessTextureUnit = program->getTextures().isValid(graphics::slot::texture::MaterialRoughness);
    locations->normalTextureUnit = program->getTextures().isValid(graphics::slot::texture::MaterialNormal);
    locations->metallicTextureUnit = program->getTextures().isValid(graphics::slot::texture::MaterialMetallic);
    locations->emissiveTextureUnit = program->getTextures().isValid(graphics::slot::texture::MaterialEmissiveLightmap);
    locations->occlusionTextureUnit = program->getTextures().isValid(graphics::slot::texture::MaterialOcclusion);
    locations->lightingModelBufferUnit = program->getUniformBuffers().isValid(render_utils::slot::buffer::LightModel);
    locations->skinClusterBufferUnit = program->getUniformBuffers().isValid(graphics::slot::buffer::Skinning);
    locations->materialBufferUnit = program->getUniformBuffers().isValid(graphics::slot::buffer::Material);
    locations->texMapArrayBufferUnit = program->getUniformBuffers().isValid(graphics::slot::buffer::TexMapArray);
    locations->keyLightBufferUnit = program->getUniformBuffers().isValid(graphics::slot::buffer::KeyLight);
    locations->lightBufferUnit = program->getUniformBuffers().isValid(graphics::slot::buffer::Light);
    locations->lightAmbientBufferUnit = program->getUniformBuffers().isValid(graphics::slot::buffer::AmbientLight);
    locations->lightAmbientMapUnit = program->getTextures().isValid(graphics::slot::texture::Skybox);
    locations->fadeMaskTextureUnit = program->getTextures().isValid(render_utils::slot::texture::FadeMask);
    locations->fadeParameterBufferUnit = program->getUniformBuffers().isValid(render_utils::slot::buffer::FadeParameters);
    locations->fadeObjectParameterBufferUnit = program->getUniformBuffers().isValid(render_utils::slot::buffer::FadeObjectParameters);
    locations->hazeParameterBufferUnit = program->getUniformBuffers().isValid(render_utils::slot::buffer::HazeParams);
    if (key.isTranslucent()) {
        locations->lightClusterGridBufferUnit = program->getUniformBuffers().isValid(render_utils::slot::buffer::LightClusterGrid);
        locations->lightClusterContentBufferUnit = program->getUniformBuffers().isValid(render_utils::slot::buffer::LightClusterContent);
        locations->lightClusterFrustumBufferUnit = program->getUniformBuffers().isValid(render_utils::slot::buffer::LightClusterFrustumGrid);
    }

    {
        PROFILE_RANGE(app, "Pipeline::create");
        auto gpuPipeline = gpu::Pipeline::create(program, state);
        auto shapePipeline = std::make_shared<Pipeline>(gpuPipeline, locations, batchSetter, itemSetter);
        addPipelineHelper(filter, key, 0, shapePipeline);
    }
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
