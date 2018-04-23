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
    regionBackFronts[0] = glm::vec2(1.0f, 3.0f);
    regionBackFronts[1] = glm::vec2(2.0f, 10.0f);
    regionBackFronts[2] = glm::vec2(4.0f, 20.0f);
    regionRegulators[0] = Regulator(std::chrono::milliseconds(2), regionBackFronts[0], 5.0f * regionBackFronts[0], glm::vec2(0.3f, 0.2f),  0.5f * glm::vec2(0.3f, 0.2f));
    regionRegulators[1] = Regulator(std::chrono::milliseconds(2), regionBackFronts[1], 5.0f * regionBackFronts[1], glm::vec2(0.3f, 0.2f), 0.5f * glm::vec2(0.3f, 0.2f));
    regionRegulators[2] = Regulator(std::chrono::milliseconds(2), regionBackFronts[2], 5.0f * regionBackFronts[2], glm::vec2(0.3f, 0.2f), 0.5f * glm::vec2(0.3f, 0.2f));
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

    if (_data.regulateViewRanges && inTimings.size()) {
        regulateViews(outViews, inTimings);
    }
}

glm::vec2 Regulator::run(const Timing_ns& regulationDuration, const Timing_ns& measured, const glm::vec2& current) {
    glm::vec2 next = current;

    // Regulate next value based on current moving toward the goal budget
    float error_ms = std::chrono::duration<float, std::milli>(_budget - measured).count();
    float coef = error_ms / std::chrono::duration<float, std::milli>(regulationDuration).count();
    next += coef * (error_ms < 0.0 ? _speedDown : _speedUp);

    // Clamp to min max
    next = glm::clamp(next, _minRange, _maxRange);
        
    return next;
}

void ControlViews::regulateViews(workload::Views& outViews, const workload::Timings& timings) {

    for (auto& outView : outViews) {
        for (int r = 0; r < workload::Region::NUM_VIEW_REGIONS; r++) {
            outView.regionBackFronts[r] = regionBackFronts[r];
        }
    }

    auto loopDuration = std::chrono::nanoseconds{ std::chrono::milliseconds(16) };
    regionBackFronts[workload::Region::R1] = regionRegulators[workload::Region::R1].run(loopDuration, timings[0], regionBackFronts[workload::Region::R1]);
    regionBackFronts[workload::Region::R2] = regionRegulators[workload::Region::R2].run(loopDuration, timings[0], regionBackFronts[workload::Region::R2]);
    regionBackFronts[workload::Region::R3] = regionRegulators[workload::Region::R3].run(loopDuration, timings[1], regionBackFronts[workload::Region::R3]);

    int i = 0;
    for (auto& outView : outViews) {
        outView.regionBackFronts[workload::Region::R1] = regionBackFronts[workload::Region::R1];
        outView.regionBackFronts[workload::Region::R2] = regionBackFronts[workload::Region::R2];
        outView.regionBackFronts[workload::Region::R3] = regionBackFronts[workload::Region::R3];

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
