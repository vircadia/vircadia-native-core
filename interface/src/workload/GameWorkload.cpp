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

const glm::vec2 DEFAULT_R1_BACK_FRONT = { 50.0f, 100.0f };
const glm::vec2 DEFAULT_R2_BACK_FRONT = { 75.0f, 150.0f };
const glm::vec2 DEFAULT_R3_BACK_FRONT = { 100.0f, 250.0f };

ControlViews::ControlViews() {
    regionBackFronts[0] = DEFAULT_R1_BACK_FRONT;
    regionBackFronts[1] = DEFAULT_R2_BACK_FRONT;
    regionBackFronts[2] = DEFAULT_R3_BACK_FRONT;

    const int32_t TIME_BUDGET_MSEC = 2;
    const float MIN_BACK_FRONT_SCALE_FACTOR = 0.05f;
    const float MAX_BACK_FRONT_SCALE_FACTOR = 2.0f;
    const float MIN_NUM_STEPS_DOWN = 250.0f;
    const float MIN_NUM_STEPS_UP = 375.0f;

    for (int32_t i = 0; i < workload::Region::NUM_VIEW_REGIONS; ++i) {
        glm::vec2 minBackFront = MIN_BACK_FRONT_SCALE_FACTOR * regionBackFronts[i];
        glm::vec2 maxBackFront = MAX_BACK_FRONT_SCALE_FACTOR * regionBackFronts[i];
        glm::vec2 stepDown = (maxBackFront - minBackFront) / MIN_NUM_STEPS_DOWN;
        glm::vec2 stepUp = (maxBackFront - minBackFront) / MIN_NUM_STEPS_UP;
        regionRegulators[i] = Regulator(std::chrono::milliseconds(TIME_BUDGET_MSEC), minBackFront, maxBackFront, stepDown, stepUp);
    }
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

        auto config = std::static_pointer_cast<Config>(runContext->jobConfig);
        config->dataExport = _dataExport;
        config->emitDirty();
    } else {
        for (auto& outView : outViews) {
            outView.regionBackFronts[workload::Region::R1] = regionBackFronts[workload::Region::R1];
            outView.regionBackFronts[workload::Region::R2] = regionBackFronts[workload::Region::R2];
            outView.regionBackFronts[workload::Region::R3] = regionBackFronts[workload::Region::R3];
            workload::View::updateRegionsFromBackFronts(outView);
        }
    }
}

glm::vec2 Regulator::run(const Timing_ns& regulationDuration, const Timing_ns& measured, const glm::vec2& current) {
    // Regulate next value based on current moving toward the goal budget
    float error_ms = std::chrono::duration<float, std::milli>(_budget - measured).count();
    float coef = error_ms / std::chrono::duration<float, std::milli>(regulationDuration).count();
    return current + coef * (error_ms < 0.0f ? _stepDown : _stepUp);
}

glm::vec2 Regulator::clamp(const glm::vec2& backFront) const {
    return glm::clamp(backFront, _minRange, _maxRange);
}

void ControlViews::regulateViews(workload::Views& outViews, const workload::Timings& timings) {

    for (auto& outView : outViews) {
        for (int32_t r = 0; r < workload::Region::NUM_VIEW_REGIONS; r++) {
            outView.regionBackFronts[r] = regionBackFronts[r];
        }
    }

    auto loopDuration = std::chrono::nanoseconds{ std::chrono::milliseconds(16) };
    regionBackFronts[workload::Region::R1] = regionRegulators[workload::Region::R1].run(loopDuration, timings[0], regionBackFronts[workload::Region::R1]);
    regionBackFronts[workload::Region::R2] = regionRegulators[workload::Region::R2].run(loopDuration, timings[0], regionBackFronts[workload::Region::R2]);
    regionBackFronts[workload::Region::R3] = regionRegulators[workload::Region::R3].run(loopDuration, timings[1], regionBackFronts[workload::Region::R3]);

    enforceRegionContainment();

    _dataExport.ranges[workload::Region::R1] = regionBackFronts[workload::Region::R1];
    _dataExport.ranges[workload::Region::R2] = regionBackFronts[workload::Region::R2];
    _dataExport.ranges[workload::Region::R3] = regionBackFronts[workload::Region::R3];

    _dataExport.timings[workload::Region::R1] = std::chrono::duration<float, std::milli>(timings[0]).count();
    _dataExport.timings[workload::Region::R2] = _dataExport.timings[workload::Region::R1];
    _dataExport.timings[workload::Region::R3] = std::chrono::duration<float, std::milli>(timings[1]).count();

    for (auto& outView : outViews) {
        outView.regionBackFronts[workload::Region::R1] = regionBackFronts[workload::Region::R1];
        outView.regionBackFronts[workload::Region::R2] = regionBackFronts[workload::Region::R2];
        outView.regionBackFronts[workload::Region::R3] = regionBackFronts[workload::Region::R3];

        workload::View::updateRegionsFromBackFronts(outView);
    }
}

void ControlViews::enforceRegionContainment() {
    // inner regions should never overreach outer
    // and each region should never exceed its min/max limits
    const glm::vec2 MIN_REGION_GAP = { 1.0f, 2.0f };
    // enforce outside --> in
    for (int32_t i = workload::Region::NUM_VIEW_REGIONS - 2; i >= 0; --i) {
        int32_t j = i + 1;
        regionBackFronts[i] = regionRegulators[i].clamp(glm::min(regionBackFronts[i], regionBackFronts[j] - MIN_REGION_GAP));
    }
    // enforce inside --> out
    for (int32_t i = 1; i < workload::Region::NUM_VIEW_REGIONS; ++i) {
        int32_t j = i - 1;
        regionBackFronts[i] = regionRegulators[i].clamp(glm::max(regionBackFronts[i], regionBackFronts[j] + MIN_REGION_GAP));
    }
}

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

        const auto controlViewsIn = workload::ControlViews::Input(usedViews, inTimings).asVarying();
        const auto fixedViews = model.addJob<workload::ControlViews>("controlViews", controlViewsIn);

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
