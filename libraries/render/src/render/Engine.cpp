//
//  Engine.cpp
//  render/src/render
//
//  Created by Sam Gateau on 3/3/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Engine.h"

#include <QtCore/QFile>

#include <PathUtils.h>

#include <gpu/Context.h>


using namespace render;

Engine::Engine() :
    _sceneContext(std::make_shared<SceneContext>()),
    _renderContext(std::make_shared<RenderContext>()) {
    addJob<EngineStats>("Stats");
}

void Engine::load() {
    auto config = getConfiguration();
    const QString configFile= "config/render.json";

    QUrl path(PathUtils::resourcesPath() + configFile);
    QFile file(path.toString());
    if (!file.exists()) {
        qWarning() << "Engine configuration file" << path << "does not exist";
    } else if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Engine configuration file" << path << "cannot be opened";
    } else {
        QString data = file.readAll();
        file.close();
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8(), &error);
        if (error.error == error.NoError) {
            config->setPresetList(doc.object());
            qDebug() << "Engine configuration file" << path << "loaded";
        } else {
            qWarning() << "Engine configuration file" << path << "failed to load:" <<
                error.errorString() << "at offset" << error.offset;
        }
    }
}

void Engine::run() {
    // Sync GPU state before beginning to render
    _renderContext->args->_context->syncCache();

    for (auto job : _jobs) {
        job.run(_sceneContext, _renderContext);
    }

}

#include <gpu/Texture.h>
void EngineStats::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    const size_t KILO_BYTES = 1024;
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

    config->numFrameTextures = _gpuStats._RSNumTextureBounded - gpuStats._RSNumTextureBounded;

    config->numFrameTriangles = _gpuStats._DSNumTriangles - gpuStats._DSNumTriangles;

    config->emitDirty();
}
