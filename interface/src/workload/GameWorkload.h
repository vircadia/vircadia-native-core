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

#include <workload/Space.h>
#include <workload/Engine.h>

#include <render/Scene.h>
#include "PhysicalEntitySimulation.h"

class GameWorkloadContext : public workload::WorkloadContext {
public:
    GameWorkloadContext(const workload::SpacePointer& space,
            const render::ScenePointer& scene,
            const PhysicalEntitySimulationPointer& simulation);
    virtual ~GameWorkloadContext();

    render::ScenePointer _scene;
    PhysicalEntitySimulationPointer _simulation;
};

class GameWorkload {
public:
    GameWorkload();
    ~GameWorkload();

    void startup(const workload::SpacePointer& space,
            const render::ScenePointer& scene,
            const PhysicalEntitySimulationPointer& simulation);
    void shutdown();

    void updateViews(const ViewFrustum& frustum, const glm::vec3& headPosition);
    void updateSimulationTimings(const workload::Timings& timings);

    workload::EnginePointer _engine;
};
#endif // hifi_GameWorkload_h
