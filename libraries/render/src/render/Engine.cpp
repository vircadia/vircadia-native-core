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

#include "DrawTask.h"
using namespace render;

RenderContext::RenderContext(ItemsConfig items, Tone tone, int drawStatus, bool drawHitEffect, glm::vec4 deferredDebugSize, int deferredDebugMode)
    : _deferredDebugMode{ deferredDebugMode }, _deferredDebugSize{ deferredDebugSize },
    _args{ nullptr },
    _drawStatus{ drawStatus }, _drawHitEffect{ drawHitEffect },
    _items{ items }, _tone{ tone } {}

void RenderContext::setOptions(bool occlusion, bool fxaa, bool showOwned) {
    _occlusionStatus = occlusion;
    _fxaaStatus = fxaa;

    if (showOwned) {
        _drawStatus |= render::showNetworkStatusFlag;
    }
};

Engine::Engine() :
    _sceneContext(std::make_shared<SceneContext>()),
    _renderContext(std::make_shared<RenderContext>())
{
}

void Engine::registerScene(const ScenePointer& scene) {
    _sceneContext->_scene = scene;
}

void Engine::setRenderContext(const RenderContext& renderContext) {
    (*_renderContext) = renderContext;
}

void Engine::addTask(const TaskPointer& task) {
    if (task) {
        _tasks.push_back(task);
    }
}

void Engine::run() {
    for (auto task : _tasks) {
        task->run(_sceneContext, _renderContext);
    }
}

void Engine::buildStandardTaskPipeline() {
    if (!_tasks.empty()) {
        _tasks.clear();
    }

    addTask(std::make_shared<DrawSceneTask>());
}

