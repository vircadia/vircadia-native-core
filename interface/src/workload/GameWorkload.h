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

class ControlViewsConfig : public workload::Job::Config {
    Q_OBJECT
    Q_PROPERTY(bool regulateViewRanges READ regulateViewRanges WRITE setRegulateViewRanges NOTIFY dirty)

public:

    bool regulateViewRanges() const { return data.regulateViewRanges; }
    void setRegulateViewRanges(bool use) { data.regulateViewRanges = use; emit dirty(); }

    struct Data {
        bool regulateViewRanges{ true };

    } data;
signals:
    void dirty();
};

class ControlViews {
public:
    using Config = ControlViewsConfig;
    using Input = workload::VaryingSet2<workload::Views, workload::Timings>;
    using Output = workload::Views;
    using JobModel = workload::Job::ModelIO<ControlViews, Input, Output, Config>;

    ControlViews();

    void configure(const Config& config);
    void run(const workload::WorkloadContextPointer& runContext, const Input& inputs, Output& outputs);
    glm::vec2 regionBackFronts[workload::Region::NUM_VIEW_REGIONS + 1];


    void regulateViews(workload::Views& views, const workload::Timings& timings);

protected:
    Config::Data _data;
};

#endif // hifi_GameWorkload_h
