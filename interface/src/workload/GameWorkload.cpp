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


GameWorkload::GameWorkload() {
}

GameWorkload::~GameWorkload() {
    shutdown();
}

void GameWorkload::startup() {
    _engine.reset(new workload::Engine());

    _engine->addJob<GameSpaceToRender>("SpaceToRender");
}

void GameWorkload::shutdown() {
    _engine.reset();
}


void GameSpaceToRender::run(const workload::WorkloadContextPointer& renderContext, Outputs& outputs) {
}