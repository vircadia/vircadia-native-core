//
//  GameWorkload.h
//
//  Created by Sam Gateau on 2/16/2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_GameWorkload_h
#define hifi_GameWorkload_h

#include "workload/Space.h"
#include "workload/Engine.h"

#include "render/Scene.h"

class GameSpaceToRender {
public:
    using Outputs = render::Transaction;
    using JobModel = workload::Job::ModelO<GameSpaceToRender, Outputs>;

    GameSpaceToRender() {}
    void run(const workload::WorkloadContextPointer& renderContext, Outputs& outputs);

protected:
    render::ItemID _spaceRenderItemID{ render::Item::INVALID_ITEM_ID };
};

class GameWorkloadContext : public workload::WorkloadContext {
public:
    GameWorkloadContext(const render::ScenePointer& scene);
    virtual ~GameWorkloadContext();

    render::ScenePointer _scene;
};

class GameWorkload {
public:
    GameWorkload();
    ~GameWorkload();

    void startup(const render::ScenePointer& scene);
    void shutdown();

    workload::EnginePointer _engine;
};

#endif