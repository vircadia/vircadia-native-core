//
//  FadeEffect.cpp

//  Created by Olivier Prat on 17/07/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "FadeEffect.h"
#include "FadeEffectJobs.h"
#include "TextureCache.h"

#include "render/TransitionStage.h"

#include <PathUtils.h>

FadeEffect::FadeEffect() {
    auto texturePath = PathUtils::resourcesPath() + "images/fadeMask.png";
    _maskMap = DependencyManager::get<TextureCache>()->getImageTexture(texturePath, image::TextureUsage::STRICT_TEXTURE);
}

void FadeEffect::build(render::Task::TaskConcept& task, const task::Varying& editableItems) {
    auto editedFadeCategory = task.addJob<FadeJob>("Fade");
    auto& fadeJob = task._jobs.back();
    _configurations = fadeJob.get<FadeJob>().getConfigurationBuffer();

    const auto fadeEditInput = FadeEditJob::Input(editableItems, editedFadeCategory).asVarying();
    task.addJob<FadeEditJob>("FadeEdit", fadeEditInput);
}

render::ShapePipeline::BatchSetter FadeEffect::getBatchSetter() const {
    return [this](const render::ShapePipeline& shapePipeline, gpu::Batch& batch, render::Args*) {
        auto program = shapePipeline.pipeline->getProgram();
        auto maskMapLocation = program->getTextures().findLocation("fadeMaskMap");
        auto bufferLocation = program->getUniformBuffers().findLocation("fadeParametersBuffer");
        batch.setResourceTexture(maskMapLocation, _maskMap);
        batch.setUniformBuffer(bufferLocation, _configurations);
    };
}

render::ShapePipeline::ItemSetter FadeEffect::getItemUniformSetter() const {
    return [](const render::ShapePipeline& shapePipeline, render::Args* args, const render::Item& item) {
        if (!render::TransitionStage::isIndexInvalid(item.getTransitionId())) {
            auto scene = args->_scene;
            auto batch = args->_batch;
            auto transitionStage = scene->getStage<render::TransitionStage>(render::TransitionStage::getName());
            auto& transitionState = transitionStage->getTransition(item.getTransitionId());
            auto program = shapePipeline.pipeline->getProgram();
            auto& uniforms = program->getUniforms();
            auto fadeNoiseOffsetLocation = uniforms.findLocation("fadeNoiseOffset");
            auto fadeBaseOffsetLocation = uniforms.findLocation("fadeBaseOffset");
            auto fadeBaseInvSizeLocation = uniforms.findLocation("fadeBaseInvSize");
            auto fadeThresholdLocation = uniforms.findLocation("fadeThreshold");
            auto fadeCategoryLocation = uniforms.findLocation("fadeCategory");

            if (fadeNoiseOffsetLocation >= 0 || fadeBaseInvSizeLocation >= 0 || fadeBaseOffsetLocation >= 0 || fadeThresholdLocation >= 0 || fadeCategoryLocation >= 0) {
                const auto fadeCategory = FadeJob::transitionToCategory[transitionState.eventType];

                batch->_glUniform1i(fadeCategoryLocation, fadeCategory);
                batch->_glUniform1f(fadeThresholdLocation, transitionState.threshold);
                batch->_glUniform3f(fadeNoiseOffsetLocation, transitionState.noiseOffset.x, transitionState.noiseOffset.y, transitionState.noiseOffset.z);
                batch->_glUniform3f(fadeBaseOffsetLocation, transitionState.baseOffset.x, transitionState.baseOffset.y, transitionState.baseOffset.z);
                batch->_glUniform3f(fadeBaseInvSizeLocation, transitionState.baseInvSize.x, transitionState.baseInvSize.y, transitionState.baseInvSize.z);
            }
        }
    };
}

render::ShapePipeline::ItemSetter FadeEffect::getItemStoredSetter() {
    return [this](const render::ShapePipeline& shapePipeline, render::Args* args, const render::Item& item) {
        if (!render::TransitionStage::isIndexInvalid(item.getTransitionId())) {
            auto scene = args->_scene;
            auto transitionStage = scene->getStage<render::TransitionStage>(render::TransitionStage::getName());
            auto& transitionState = transitionStage->getTransition(item.getTransitionId());
            const auto fadeCategory = FadeJob::transitionToCategory[transitionState.eventType];

            _lastCategory = fadeCategory;
            _lastThreshold = transitionState.threshold;
            _lastNoiseOffset = transitionState.noiseOffset;
            _lastBaseOffset = transitionState.baseOffset;
            _lastBaseInvSize = transitionState.baseInvSize;
        }
    };
}

void FadeEffect::packToAttributes(const int category, const float threshold, const glm::vec3& noiseOffset,
    const glm::vec3& baseOffset, const glm::vec3& baseInvSize,
    glm::vec4& packedData1, glm::vec4& packedData2, glm::vec4& packedData3) {
    packedData1.x = noiseOffset.x;
    packedData1.y = noiseOffset.y;
    packedData1.z = noiseOffset.z;
    packedData1.w = (float)(category+0.1f); // GLSL hack so that casting back from float to int in fragment shader returns the correct value.

    packedData2.x = baseOffset.x;
    packedData2.y = baseOffset.y;
    packedData2.z = baseOffset.z;
    packedData2.w = threshold;

    packedData3.x = baseInvSize.x;
    packedData3.y = baseInvSize.y;
    packedData3.z = baseInvSize.z;
    packedData3.w = 0.f;
}
