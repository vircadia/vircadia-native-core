//
//  GameWorkload.cpp
//
//  Created by Sam Gateau on 2/16/2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GameWorkload.h"
#include "GameWorkloadRenderer.h"
#include <ViewFrustum.h>

GameWorkloadContext::GameWorkloadContext(const workload::SpacePointer& space, const render::ScenePointer& scene) : WorkloadContext(space), _scene(scene) {
}

GameWorkloadContext::~GameWorkloadContext() {
}


GameWorkload::GameWorkload() {
}

GameWorkload::~GameWorkload() {
    shutdown();
}

void GameWorkload::startup(const workload::SpacePointer& space, const render::ScenePointer& scene) {
    _engine.reset(new workload::Engine(std::make_shared<GameWorkloadContext>(space, scene)));

    _engine->addJob<GameSpaceToRender>("SpaceToRender");


}

void GameWorkload::shutdown() {
    _engine.reset();
}

void GameWorkload::updateViews(const ViewFrustum& frustum) {
    // TEMP HACK: one view with static radiuses
    float r0 = 10.0f;
    float r1 = 20.0f;
    float r2 = 30.0f;
    workload::View view(frustum.getPosition(), r0, r1, r2);
    workload::Views views;
    views.push_back(view);
    _engine->setInput(views);
}

