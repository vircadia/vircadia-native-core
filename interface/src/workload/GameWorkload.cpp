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
#include <workload/SpaceClassifier.h>

#include "PhysicsBoundary.h"
#pragma optimize( "[optimization-list]", off )

ControlViews::ControlViews() {
    regionBackFronts[0] = glm::vec2(2.0f, 10.0f);
    regionBackFronts[1] = glm::vec2(5.0f, 30.0f);
    regionBackFronts[2] = glm::vec2(10.0f, 100.0f);

}

void ControlViews::configure(const Config& config) {
    _data = config.data;
}

void ControlViews::run(const workload::WorkloadContextPointer& runContext, const Input& inputs, Output& outputs) {
    const auto& inViews = inputs.get0();
    const auto& inTimings = inputs.get1();
    auto& outViews = outputs;
    outViews.clear();
    outViews = inViews;

    if (_data.regulateViewRanges) {
        regulateViews(outViews, inTimings);
    }
}

float wtf_adjust(float current, float timing) {
    float error = -((timing * 0.001f) - 2.0f);
    if (error < 0.0f) {
        current += 0.2f * (error) / 16.0f;
    } else {
        current += 0.1f * (error) / 16.0f;
    }

    if (current > 100.0f) {
        current = 100.0f;
    } else if (current < 5.0f) {
        current = 5.0f;
    }
    return current;
}

void ControlViews::regulateViews(workload::Views& outViews, const workload::Timings& timings) {

    for (auto& outView : outViews) {
        for (int r = 0; r < workload::Region::NUM_VIEW_REGIONS; r++) {
            outView.regionBackFronts[r] = regionBackFronts[r];
        }
    }

    int i = 0;
    for (auto& outView : outViews) {
        auto current = regionBackFronts[workload::Region::R2].y;
        auto newCurrent = wtf_adjust(current, timings[0]);
        regionBackFronts[workload::Region::R2].y = newCurrent;
        outView.regionBackFronts[workload::Region::R2].y = newCurrent;
        workload::View::updateRegionsFromBackFronts(outView);
    }
}

#pragma optimize( "[optimization-list]", on )

class WorkloadEngineBuilder {
public:
public:
    using Inputs = workload::VaryingSet2<workload::Views, workload::Timings>;
    using Outputs = workload::RegionTracker::Outputs;
    using JobModel = workload::Task::ModelIO<WorkloadEngineBuilder, Inputs, Outputs>;
    void build(JobModel& model, const workload::Varying& in, workload::Varying& out) {

        const auto& inViews = in.getN<Inputs>(0);
        const auto& inTimings = in.getN<Inputs>(1);

        const auto usedViews = model.addJob<workload::SetupViews>("setupViews", inViews);

        const auto controlViewsIn = ControlViews::Input(usedViews, inTimings).asVarying();
        const auto fixedViews = model.addJob<ControlViews>("controlViews", controlViewsIn);

        const auto regionTrackerOut = model.addJob<workload::SpaceClassifierTask>("spaceClassifier", fixedViews);

        model.addJob<PhysicsBoundary>("PhysicsBoundary", regionTrackerOut);

        model.addJob<GameSpaceToRender>("SpaceToRender");

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
    _engine(std::make_shared<workload::Engine>(WorkloadEngineBuilder::JobModel::create("Engine")))
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
