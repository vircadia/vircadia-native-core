//
//  DeferredPipelineLib.cpp
//  render-utils/src
//
//  Created by Zach Pomerantz on 1/4/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DeferredPipelineLib.h"

#include <model-networking/TextureCache.h>

#include <PerfStat.h>

#include "DeferredLightingEffect.h"

#include "model_vert.h"
#include "model_shadow_vert.h"
#include "model_normal_map_vert.h"
#include "model_lightmap_vert.h"
#include "model_lightmap_normal_map_vert.h"
#include "skin_model_vert.h"
#include "skin_model_shadow_vert.h"
#include "skin_model_normal_map_vert.h"

#include "model_frag.h"
#include "model_shadow_frag.h"
#include "model_normal_map_frag.h"
#include "model_normal_specular_map_frag.h"
#include "model_specular_map_frag.h"
#include "model_lightmap_frag.h"
#include "model_lightmap_normal_map_frag.h"
#include "model_lightmap_normal_specular_map_frag.h"
#include "model_lightmap_specular_map_frag.h"
#include "model_translucent_frag.h"

DeferredPipelineLib::DeferredPipelineLib() {
    if (!_isInitPipeline) {
        initPipeline();
    }
}

const DeferredPipelineLib::PipelinePointer DeferredPipelineLib::pickPipeline(RenderArgs* args, const Key& key) const {
    PerformanceTimer perfTimer("DeferredPipelineLib::pickPipeline");

    auto pipeline = _pickPipeline(args, key);
    if (!pipeline) {
        return pipeline;
    }

    if ((pipeline->locations->normalFittingMapUnit > -1)) {
        args->_batch->setResourceTexture(pipeline->locations->normalFittingMapUnit,
            DependencyManager::get<TextureCache>()->getNormalFittingTexture());
    }

    return pipeline;
}

bool DeferredPipelineLib::_isInitPipeline { false };

void DeferredPipelineLib::initPipeline() {
    assert(_pipelineLib.empty());

    // Vertex shaders
    auto modelVertex = gpu::Shader::createVertex(std::string(model_vert));
    auto modelNormalMapVertex = gpu::Shader::createVertex(std::string(model_normal_map_vert));
    auto modelLightmapVertex = gpu::Shader::createVertex(std::string(model_lightmap_vert));
    auto modelLightmapNormalMapVertex = gpu::Shader::createVertex(std::string(model_lightmap_normal_map_vert));
    auto modelShadowVertex = gpu::Shader::createVertex(std::string(model_shadow_vert));
    auto skinModelVertex = gpu::Shader::createVertex(std::string(skin_model_vert));
    auto skinModelNormalMapVertex = gpu::Shader::createVertex(std::string(skin_model_normal_map_vert));
    auto skinModelShadowVertex = gpu::Shader::createVertex(std::string(skin_model_shadow_vert));

    // Pixel shaders
    auto modelPixel = gpu::Shader::createPixel(std::string(model_frag));
    auto modelNormalMapPixel = gpu::Shader::createPixel(std::string(model_normal_map_frag));
    auto modelSpecularMapPixel = gpu::Shader::createPixel(std::string(model_specular_map_frag));
    auto modelNormalSpecularMapPixel = gpu::Shader::createPixel(std::string(model_normal_specular_map_frag));
    auto modelTranslucentPixel = gpu::Shader::createPixel(std::string(model_translucent_frag));
    auto modelShadowPixel = gpu::Shader::createPixel(std::string(model_shadow_frag));
    auto modelLightmapPixel = gpu::Shader::createPixel(std::string(model_lightmap_frag));
    auto modelLightmapNormalMapPixel = gpu::Shader::createPixel(std::string(model_lightmap_normal_map_frag));
    auto modelLightmapSpecularMapPixel = gpu::Shader::createPixel(std::string(model_lightmap_specular_map_frag));
    auto modelLightmapNormalSpecularMapPixel = gpu::Shader::createPixel(std::string(model_lightmap_normal_specular_map_frag));

    // Fill the pipelineLib

    _pipelineLib.addPipeline(
        Key::Builder(),
        modelVertex, modelPixel);

    _pipelineLib.addPipeline(
        Key::Builder().withTangents(),
        modelNormalMapVertex, modelNormalMapPixel);

    _pipelineLib.addPipeline(
        Key::Builder().withSpecular(),
        modelVertex, modelSpecularMapPixel);

    _pipelineLib.addPipeline(
        Key::Builder().withTangents().withSpecular(),
        modelNormalMapVertex, modelNormalSpecularMapPixel);


    _pipelineLib.addPipeline(
        Key::Builder().withTranslucent(),
        modelVertex, modelTranslucentPixel);
    // FIXME Ignore lightmap for translucents meshpart
    _pipelineLib.addPipeline(
        Key::Builder().withTranslucent().withLightmap(),
        modelVertex, modelTranslucentPixel);

    _pipelineLib.addPipeline(
        Key::Builder().withTangents().withTranslucent(),
        modelNormalMapVertex, modelTranslucentPixel);

    _pipelineLib.addPipeline(
        Key::Builder().withSpecular().withTranslucent(),
        modelVertex, modelTranslucentPixel);

    _pipelineLib.addPipeline(
        Key::Builder().withTangents().withSpecular().withTranslucent(),
        modelNormalMapVertex, modelTranslucentPixel);


    _pipelineLib.addPipeline(
        Key::Builder().withLightmap(),
        modelLightmapVertex, modelLightmapPixel);

    _pipelineLib.addPipeline(
        Key::Builder().withLightmap().withTangents(),
        modelLightmapNormalMapVertex, modelLightmapNormalMapPixel);

    _pipelineLib.addPipeline(
        Key::Builder().withLightmap().withSpecular(),
        modelLightmapVertex, modelLightmapSpecularMapPixel);

    _pipelineLib.addPipeline(
        Key::Builder().withLightmap().withTangents().withSpecular(),
        modelLightmapNormalMapVertex, modelLightmapNormalSpecularMapPixel);


    _pipelineLib.addPipeline(
        Key::Builder().withSkinned(),
        skinModelVertex, modelPixel);

    _pipelineLib.addPipeline(
        Key::Builder().withSkinned().withTangents(),
        skinModelNormalMapVertex, modelNormalMapPixel);

    _pipelineLib.addPipeline(
        Key::Builder().withSkinned().withSpecular(),
        skinModelVertex, modelSpecularMapPixel);

    _pipelineLib.addPipeline(
        Key::Builder().withSkinned().withTangents().withSpecular(),
        skinModelNormalMapVertex, modelNormalSpecularMapPixel);


    _pipelineLib.addPipeline(
        Key::Builder().withSkinned().withTranslucent(),
        skinModelVertex, modelTranslucentPixel);

    _pipelineLib.addPipeline(
        Key::Builder().withSkinned().withTangents().withTranslucent(),
        skinModelNormalMapVertex, modelTranslucentPixel);

    _pipelineLib.addPipeline(
        Key::Builder().withSkinned().withSpecular().withTranslucent(),
        skinModelVertex, modelTranslucentPixel);

    _pipelineLib.addPipeline(
        Key::Builder().withSkinned().withTangents().withSpecular().withTranslucent(),
        skinModelNormalMapVertex, modelTranslucentPixel);


    _pipelineLib.addPipeline(
        Key::Builder().withDepthOnly().withShadow(),
        modelShadowVertex, modelShadowPixel);


    _pipelineLib.addPipeline(
        Key::Builder().withSkinned().withDepthOnly().withShadow(),
        skinModelShadowVertex, modelShadowPixel);

    _isInitPipeline = true;
}

model::MaterialPointer DeferredPipelineLib::_collisionHullMaterial;

model::MaterialPointer DeferredPipelineLib::getCollisionHullMaterial() {
    if (!_collisionHullMaterial) {
        _collisionHullMaterial = std::make_shared<model::Material>();
        _collisionHullMaterial->setDiffuse(glm::vec3(1.0f, 0.5f, 0.0f));
        _collisionHullMaterial->setMetallic(0.02f);
        _collisionHullMaterial->setGloss(1.0f);
    }

    return _collisionHullMaterial;
}

