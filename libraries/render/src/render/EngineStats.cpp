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

void EngineStats::run(const RenderContextPointer& renderContext) {
    // Tick time

    quint64 msecsElapsed = _frameTimer.restart();
    double frequency = 1000.0 / msecsElapsed;

    // Update the stats
    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);

    config->bufferCPUCount = gpu::Buffer::getBufferCPUCount();
    config->bufferGPUCount = gpu::Context::getBufferGPUCount();
    config->bufferCPUMemSize = gpu::Buffer::getBufferCPUMemSize();
    config->bufferGPUMemSize = gpu::Context::getBufferGPUMemSize();

    config->textureCPUCount = gpu::Texture::getTextureCPUCount();
    config->textureGPUCount = gpu::Context::getTextureGPUCount();
    config->textureCPUMemSize = gpu::Texture::getTextureCPUMemSize();
    config->textureGPUMemSize = gpu::Context::getTextureGPUMemSize();

    config->textureResidentGPUCount = gpu::Context::getTextureResidentGPUCount();
    config->textureFramebufferGPUCount = gpu::Context::getTextureFramebufferGPUCount();
    config->textureResourceGPUCount = gpu::Context::getTextureResourceGPUCount();
    config->textureExternalGPUCount = gpu::Context::getTextureExternalGPUCount();

    config->textureResidentGPUMemSize = gpu::Context::getTextureResidentGPUMemSize();
    config->textureFramebufferGPUMemSize = gpu::Context::getTextureFramebufferGPUMemSize();
    config->textureResourceGPUMemSize = gpu::Context::getTextureResourceGPUMemSize();
    config->textureExternalGPUMemSize = gpu::Context::getTextureExternalGPUMemSize();

    config->texturePendingGPUTransferCount = gpu::Context::getTexturePendingGPUTransferCount();
    config->texturePendingGPUTransferSize = gpu::Context::getTexturePendingGPUTransferMemSize();

    config->textureResourcePopulatedGPUMemSize = gpu::Context::getTextureResourcePopulatedGPUMemSize();

    renderContext->args->_context->getFrameStats(_gpuStats);

    config->frameAPIDrawcallCount = _gpuStats._DSNumAPIDrawcalls;
    config->frameDrawcallCount = _gpuStats._DSNumDrawcalls;
    config->frameDrawcallRate = config->frameDrawcallCount * frequency;

    config->frameTriangleCount = _gpuStats._DSNumTriangles;
    config->frameTriangleRate = config->frameTriangleCount * frequency;

    config->frameTextureCount = _gpuStats._RSNumTextureBounded;
    config->frameTextureRate = config->frameTextureCount * frequency;
    config->frameTextureMemoryUsage = _gpuStats._RSAmountTextureMemoryBounded;

    config->frameSetPipelineCount = _gpuStats._PSNumSetPipelines;
    config->frameSetInputFormatCount = _gpuStats._ISNumFormatChanges;

    // These new stat values are notified with the "newStats" signal triggered by the timer
}
