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

    Q_PROPERTY(float r1Timing READ r1Timing NOTIFY dirty)
    Q_PROPERTY(float r2Timing READ r2Timing NOTIFY dirty)
    Q_PROPERTY(float r3Timing READ r3Timing NOTIFY dirty)

    Q_PROPERTY(float r1RangeBack READ r1RangeBack NOTIFY dirty)
    Q_PROPERTY(float r2RangeBack READ r2RangeBack NOTIFY dirty)
    Q_PROPERTY(float r3RangeBack READ r3RangeBack NOTIFY dirty)

    Q_PROPERTY(float r1RangeFront READ r1RangeFront NOTIFY dirty)
    Q_PROPERTY(float r2RangeFront READ r2RangeFront NOTIFY dirty)
    Q_PROPERTY(float r3RangeFront READ r3RangeFront NOTIFY dirty)
/*
    Q_PROPERTY(float r1MinRangeBack READ r1MinRangeBack WRITE setR1MinRangeBack NOTIFY dirty)
    Q_PROPERTY(float r2MinRangeBack READ r2MinRangeBack WRITE setR2MinRangeBack NOTIFY dirty)
    Q_PROPERTY(float r3MinRangeBack READ r3MinRangeBack WRITE setR3MinRangeBack NOTIFY dirty)

    Q_PROPERTY(float r1MinRangeFront READ r1MinRangeFront WRITE setR1MinRangeFront NOTIFY dirty)
    Q_PROPERTY(float r2MinRangeFront READ r2MinRangeFront WRITE setR2MinRangeFront NOTIFY dirty)
    Q_PROPERTY(float r3MinRangeFront READ r3MinRangeFront WRITE setR3MinRangeFront NOTIFY dirty)

    Q_PROPERTY(float r1MaxRangeBack READ r1MaxRangeBack WRITE setR1MaxRangeBack NOTIFY dirty)
    Q_PROPERTY(float r2MaxRangeBack READ r2MaxRangeBack WRITE setR2MaxRangeBack NOTIFY dirty)
    Q_PROPERTY(float r3MaxRangeBack READ r3MaxRangeBack WRITE setR3MaxRangeBack NOTIFY dirty)

    Q_PROPERTY(float r1MaxRangeFront READ r1MaxRangeFront WRITE setR1MaxRangeFront NOTIFY dirty)
    Q_PROPERTY(float r2MaxRangeFront READ r2MaxRangeFront WRITE setR2MaxRangeFront NOTIFY dirty)
    Q_PROPERTY(float r3MaxRangeFront READ r3MaxRangeFront WRITE setR3MaxRangeFront NOTIFY dirty)

    Q_PROPERTY(float r1SpeedDownBack READ r1SpeedDownBack WRITE setR1SpeedDownBack NOTIFY dirty)
    Q_PROPERTY(float r2SpeedDownBack READ r2SpeedDownBack WRITE setR2SpeedDownBack NOTIFY dirty)
    Q_PROPERTY(float r3SpeedDownBack READ r3SpeedDownBack WRITE setR3SpeedDownBack NOTIFY dirty)

    Q_PROPERTY(float r1SpeedDownFront READ r1SpeedDownFront WRITE setR1SpeedDownFront NOTIFY dirty)
    Q_PROPERTY(float r2SpeedDownFront READ r2SpeedDownFront WRITE setR2SpeedDownFront NOTIFY dirty)
    Q_PROPERTY(float r3SpeedDownFront READ r3SpeedDownFront WRITE setR3SpeedDownFront NOTIFY dirty)

    Q_PROPERTY(float r1SpeedUpBack READ r1SpeedUpBack WRITE setR1SpeedUpBack NOTIFY dirty)
    Q_PROPERTY(float r2SpeedUpBack READ r2SpeedUpBack WRITE setR2SpeedUpBack NOTIFY dirty)
    Q_PROPERTY(float r3SpeedUpBack READ r3SpeedUpBack WRITE setR3SpeedUpBack NOTIFY dirty)

    Q_PROPERTY(float r1SpeedUpFront READ r1SpeedUpFront WRITE setR1SpeedUpFront NOTIFY dirty)
    Q_PROPERTY(float r2SpeedUpFront READ r2SpeedUpFront WRITE setR2SpeedUpFront NOTIFY dirty)
    Q_PROPERTY(float r3SpeedUpFront READ r3SpeedUpFront WRITE setR3SpeedUpFront NOTIFY dirty)*/

public:

    bool regulateViewRanges() const { return data.regulateViewRanges; }
    void setRegulateViewRanges(bool use) { data.regulateViewRanges = use; emit dirty(); }

    float r1Timing() const { return dataExport.timings[workload::Region::R1]; }
    float r2Timing() const { return dataExport.timings[workload::Region::R2]; }
    float r3Timing() const { return dataExport.timings[workload::Region::R3]; }

    float r1RangeBack() const { return dataExport.ranges[workload::Region::R1].x; }
    float r2RangeBack() const { return dataExport.ranges[workload::Region::R2].x; }
    float r3RangeBack() const { return dataExport.ranges[workload::Region::R3].x; }

    float r1RangeFront() const { return dataExport.ranges[workload::Region::R1].y; }
    float r2RangeFront() const { return dataExport.ranges[workload::Region::R2].y; }
    float r3RangeFront() const { return dataExport.ranges[workload::Region::R3].y; }

    struct Data {
        bool regulateViewRanges{ true };
    } data;

    struct DataExport {
        static const int SIZE{ workload::Region::NUM_VIEW_REGIONS };
        float timings[SIZE];
        glm::vec2 ranges[SIZE];
    } dataExport;

    void emitDirty() { emit dirty(); }
signals:
    void dirty();
};

struct Regulator {
    using Timing_ns = std::chrono::nanoseconds;
    Timing_ns _budget{ std::chrono::milliseconds(2) };
    glm::vec2 _minRange{ 2.0f, 5.0f };
    glm::vec2 _maxRange{ 50.0f, 100.0f };

    glm::vec2 _speedDown{ 0.2f };
    glm::vec2 _speedUp{ 0.1f };


    Regulator() {}
    Regulator(const Timing_ns& budget_ns, const glm::vec2& minRange, const glm::vec2& maxRange, const glm::vec2& speedDown, const glm::vec2& speedUp) :
        _budget(budget_ns), _minRange(minRange), _maxRange(maxRange), _speedDown(speedDown), _speedUp(speedUp) {}

    glm::vec2 run(const Timing_ns& regulationDuration, const Timing_ns& measured, const glm::vec2& current);
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

    std::array<glm::vec2, workload::Region::NUM_VIEW_REGIONS> regionBackFronts;
    std::array<Regulator, workload::Region::NUM_VIEW_REGIONS> regionRegulators;

    void regulateViews(workload::Views& views, const workload::Timings& timings);

protected:
    Config::Data _data;
    Config::DataExport _dataExport;
};

#endif // hifi_GameWorkload_h
