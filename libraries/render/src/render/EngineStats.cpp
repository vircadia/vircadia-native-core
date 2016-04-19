//
//  EngineStats.cpp
//  render/src/render
//
//  Created by Sam Gateau on 3/27/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "EngineStats.h"

#include <gpu/Texture.h>

using namespace render;

void EngineStats::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    // Tick time

    quint64 msecsElapsed = _frameTimer.restart();
    double frequency = 1000.0 / msecsElapsed;

    // Update the stats
    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);

    config->bufferCPUCount = gpu::Buffer::getBufferCPUCount();
    config->bufferGPUCount = gpu::Buffer::getBufferGPUCount();
    config->bufferCPUMemoryUsage = gpu::Buffer::getBufferCPUMemoryUsage();
    config->bufferGPUMemoryUsage = gpu::Buffer::getBufferGPUMemoryUsage();

    config->textureCPUCount = gpu::Texture::getTextureCPUCount();
    config->textureGPUCount = gpu::Texture::getTextureGPUCount();
    config->textureCPUMemoryUsage = gpu::Texture::getTextureCPUMemoryUsage();
    config->textureGPUMemoryUsage = gpu::Texture::getTextureGPUMemoryUsage();
    config->textureGPUVirtualMemoryUsage = gpu::Texture::getTextureGPUVirtualMemoryUsage();
    config->textureGPUTransferCount = gpu::Texture::getTextureGPUTransferCount();

    gpu::ContextStats gpuStats(_gpuStats);
    renderContext->args->_context->getStats(_gpuStats);

    config->frameAPIDrawcallCount = _gpuStats._DSNumAPIDrawcalls - gpuStats._DSNumAPIDrawcalls;
    config->frameDrawcallCount = _gpuStats._DSNumDrawcalls - gpuStats._DSNumDrawcalls;
    config->frameDrawcallRate = config->frameDrawcallCount * frequency;

    config->frameTriangleCount = _gpuStats._DSNumTriangles - gpuStats._DSNumTriangles;
    config->frameTriangleRate = config->frameTriangleCount * frequency;

    config->frameTextureCount = _gpuStats._RSNumTextureBounded - gpuStats._RSNumTextureBounded;
    config->frameTextureRate = config->frameTextureCount * frequency;
    config->frameTextureMemoryUsage = _gpuStats._RSAmountTextureMemoryBounded - gpuStats._RSAmountTextureMemoryBounded;

    config->frameSetPipelineCount = _gpuStats._PSNumSetPipelines - gpuStats._PSNumSetPipelines;

    config->emitDirty();
}
