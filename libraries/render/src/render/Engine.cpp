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


Engine::Engine() :
    _sceneContext(new SceneContext())
{
}

void Engine::registerScene(const ScenePointer& scene) {
    _sceneContext->_scene = scene;
}

void Engine::addTask(const TaskPointer& task) {
    if (task) {
        _tasks.push_back(task);
    }
}

void Engine::run() {
    for (auto task : _tasks) {
        task->run(_sceneContext);
    }
}

void Engine::buildStandardTaskPipeline() {
    if (!_tasks.empty()) {
        _tasks.clear();
    }

    addTask(TaskPointer(new DrawSceneTask()));
}

