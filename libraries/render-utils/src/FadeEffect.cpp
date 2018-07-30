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

#include "FadeObjectParams.shared.slh"

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
        batch.setResourceTexture(render::ShapePipeline::Slot::FADE_MASK, _maskMap);
        batch.setUniformBuffer(render::ShapePipeline::Slot::FADE_PARAMETERS, _configurations);
    };
}

render::ShapePipeline::ItemSetter FadeEffect::getItemUniformSetter() const {
    return [](const render::ShapePipeline& shapePipeline, render::Args* args, const render::Item& item) {
        if (!render::TransitionStage::isIndexInvalid(item.getTransitionId())) {
            auto scene = args->_scene;
            auto batch = args->_batch;
            auto transitionStage = scene->getStage<render::TransitionStage>(render::TransitionStage::getName());
            auto& transitionState = transitionStage->getTransition(item.getTransitionId());

            if (transitionState.paramsBuffer._size != sizeof(gpu::StructBuffer<FadeObjectParams>)) {
                static_assert(sizeof(transitionState.paramsBuffer) == sizeof(gpu::StructBuffer<FadeObjectParams>), "Assuming gpu::StructBuffer is a helper class for gpu::BufferView");
                transitionState.paramsBuffer = gpu::StructBuffer<FadeObjectParams>();
            }

            const auto fadeCategory = FadeJob::transitionToCategory[transitionState.eventType];
            auto& paramsConst = static_cast<gpu::StructBuffer<FadeObjectParams>&>(transitionState.paramsBuffer).get();

            if (paramsConst.category != fadeCategory
                || paramsConst.threshold != transitionState.threshold
                || glm::vec3(paramsConst.baseOffset) != transitionState.baseOffset
                || glm::vec3(paramsConst.noiseOffset) != transitionState.noiseOffset
                || glm::vec3(paramsConst.baseInvSize) != transitionState.baseInvSize) {
                auto& params = static_cast<gpu::StructBuffer<FadeObjectParams>&>(transitionState.paramsBuffer).edit();

                params.category = fadeCategory;
                params.threshold = transitionState.threshold;
                params.baseInvSize = glm::vec4(transitionState.baseInvSize, 0.0f);
                params.noiseOffset = glm::vec4(transitionState.noiseOffset, 0.0f);
                params.baseOffset = glm::vec4(transitionState.baseOffset, 0.0f);
            }
            batch->setUniformBuffer(render::ShapePipeline::Slot::FADE_OBJECT_PARAMETERS, transitionState.paramsBuffer);
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
