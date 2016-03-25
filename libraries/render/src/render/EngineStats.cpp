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

    config->numBuffers = gpu::Buffer::getCurrentNumBuffers();
    config->numGPUBuffers = gpu::Buffer::getCurrentNumGPUBuffers();
    config->bufferSysmemUsage = gpu::Buffer::getCurrentSystemMemoryUsage();
    config->bufferVidmemUsage = gpu::Buffer::getCurrentVideoMemoryUsage();

    config->numTextures = gpu::Texture::getCurrentNumTextures();
    config->numGPUTextures = gpu::Texture::getCurrentNumGPUTextures();
    config->textureSysmemUsage = gpu::Texture::getCurrentSystemMemoryUsage();
    config->textureVidmemUsage = gpu::Texture::getCurrentVideoMemoryUsage();

    gpu::ContextStats gpuStats(_gpuStats);
    renderContext->args->_context->getStats(_gpuStats);

    config->frameDrawcallCount = _gpuStats._DSNumDrawcalls - gpuStats._DSNumDrawcalls;
    config->frameDrawcallRate = config->frameDrawcallCount * frequency;

    config->frameTriangleCount = _gpuStats._DSNumTriangles - gpuStats._DSNumTriangles;
    config->frameTriangleRate = config->frameTriangleCount * frequency;

    config->frameTextureCount = _gpuStats._RSNumTextureBounded - gpuStats._RSNumTextureBounded;
    config->frameTextureRate = config->frameTextureCount * frequency;

    config->emitDirty();
}
