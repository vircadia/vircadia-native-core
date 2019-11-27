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
#include "SelectedWorkloadRenderer.h"
#include <ViewFrustum.h>
#include <workload/RegionTracker.h>
#include <workload/SpaceClassifier.h>

#include "PhysicsBoundary.h"

class WorkloadEngineBuilder {
public:
    using Inputs = workload::VaryingSet2<workload::Views, workload::Timings>;
    using Outputs = workload::RegionTracker::Outputs;
    using JobModel = workload::Task::ModelIO<WorkloadEngineBuilder, Inputs, Outputs>;
    void build(JobModel& model, const workload::Varying& in, workload::Varying& out) {

        const auto& inViews = in.getN<Inputs>(0);
        const auto& inTimings = in.getN<Inputs>(1);

        const auto usedViews = model.addJob<workload::SetupViews>("setupViews", inViews);

        const auto controlViewsIn = workload::ControlViews::Input(usedViews, inTimings).asVarying();
        const auto fixedViews = model.addJob<workload::ControlViews>("controlViews", controlViewsIn);

        const auto regionTrackerOut = model.addJob<workload::SpaceClassifierTask>("spaceClassifier", fixedViews);

        model.addJob<PhysicsBoundary>("PhysicsBoundary", regionTrackerOut);

        model.addJob<GameSpaceToRender>("SpaceToRender");
        model.addJob<SelectedWorkloadRenderer>("SelectedWorkloadRender");

        out = regionTrackerOut;
    }
};

GameWorkloadContext::GameWorkloadContext(const workload::SpacePointer& space,
        const render::ScenePointer& scene,
        const PhysicalEntitySimulationPointer& simulation): WorkloadContext(space),
    _scene(scene), _simulation(simulation)
{
}

GameWorkloadContext::~GameWorkloadContext() {
}


GameWorkload::GameWorkload() :
    _engine(std::make_shared<workload::Engine>(WorkloadEngineBuilder::JobModel::create("Workload"), std::shared_ptr<GameWorkloadContext>()))
{
}

GameWorkload::~GameWorkload() {
    shutdown();
}

void GameWorkload::startup(const workload::SpacePointer& space,
        const render::ScenePointer& scene,
        const PhysicalEntitySimulationPointer& simulation) {
    _engine->reset(std::make_shared<GameWorkloadContext>(space, scene, simulation));
}

void GameWorkload::shutdown() {
    _engine.reset();
}

void GameWorkload::updateViews(const ViewFrustum& frustum, const glm::vec3& headPosition) {
    workload::Views views;
    views.emplace_back(workload::View::evalFromFrustum(frustum, headPosition - frustum.getPosition()));
    views.emplace_back(workload::View::evalFromFrustum(frustum));
    _engine->feedInput<WorkloadEngineBuilder::Inputs>(0, views);
}

void GameWorkload::updateSimulationTimings(const workload::Timings& timings) {
    _engine->feedInput<WorkloadEngineBuilder::Inputs>(1, timings);
}
