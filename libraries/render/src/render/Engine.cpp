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

#include <gpu/Context.h>

#include "Engine.h"

using namespace render;

Engine::Engine() :
    _sceneContext(std::make_shared<SceneContext>()),
    _renderContext(std::make_shared<RenderContext>()) {
}

void Engine::run() {
    // Sync GPU state before beginning to render
    _renderContext->args->_context->syncCache();

    for (auto job : _jobs) {
        job.run(_sceneContext, _renderContext);
    }
}
