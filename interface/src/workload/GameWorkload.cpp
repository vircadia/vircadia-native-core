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
#include <workload/RegionTracker.h>

#include "PhysicsBoundary.h"

GameWorkloadContext::GameWorkloadContext(const workload::SpacePointer& space,
        const render::ScenePointer& scene,
        const PhysicalEntitySimulationPointer& simulation)
:   WorkloadContext(space), _scene(scene), _simulation(simulation)
{
}

GameWorkloadContext::~GameWorkloadContext() {
}


GameWorkload::GameWorkload() {
}

GameWorkload::~GameWorkload() {
    shutdown();
}

void GameWorkload::startup(const workload::SpacePointer& space,
        const render::ScenePointer& scene,
        const PhysicalEntitySimulationPointer& simulation) {
    _engine->reset(std::make_shared<GameWorkloadContext>(space, scene, simulation));

    auto output = _engine->getOutput();
    _engine->addJob<GameSpaceToRender>("SpaceToRender");

    const auto regionChanges = _engine->getOutput();
    _engine->addJob<PhysicsBoundary>("PhysicsBoundary", regionChanges);
}

void GameWorkload::shutdown() {
    _engine.reset();
}

void GameWorkload::updateViews(const ViewFrustum& frustum) {
    workload::Views views;
    views.emplace_back(workload::View::evalFromFrustum(frustum));
    _engine->feedInput(views);
}

