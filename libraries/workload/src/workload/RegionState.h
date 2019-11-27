//
//  RegionState.h
//  libraries/workload/src/workload
//
//  Created by Andrew Meadows 2018.03.07
//  Copyright 2018 High Fidelity, Inc.
//
//  Originally from lighthouse3d. Modified to utilize glm::vec3 and clean up to our coding standards
//  Simple plane class.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_workload_RegionState_h
#define hifi_workload_RegionState_h

#include "Space.h"
#include "Engine.h"

namespace workload {
    class RegionStateConfig : public Job::Config{
        Q_OBJECT
        Q_PROPERTY(float numR0 READ getNumR0 NOTIFY dirty)
        Q_PROPERTY(float numR1 READ getNumR1 NOTIFY dirty)
        Q_PROPERTY(float numR2 READ getNumR2 NOTIFY dirty)
        Q_PROPERTY(float numR3 READ getNumR3 NOTIFY dirty)
    public:

        uint32_t getNumR0() const { return data.numR0; }
        uint32_t getNumR1() const { return data.numR1; }
        uint32_t getNumR2() const { return data.numR2; }
        uint32_t getNumR3() const { return data.numR3; }

        void setNum(const uint32_t r0, const uint32_t r1, const uint32_t r2, const uint32_t r3) {
            data.numR0 = r0; data.numR1 = r1; data.numR2 = r2; data.numR3 = r3; emit dirty();
        }

        struct Data {
            uint32_t numR0{ 0 };
            uint32_t numR1{ 0 };
            uint32_t numR2{ 0 };
            uint32_t numR3{ 0 };
        } data;

    signals:
        void dirty();
    };

    class RegionState {
    public:
        using Config = RegionStateConfig;
        using Inputs = IndexVectors;
        using JobModel = workload::Job::ModelI<RegionState, Inputs, Config>;

        RegionState() { _state.resize(workload::Region::NUM_TRACKED_REGIONS); }

        void configure(const Config& config);
        void run(const workload::WorkloadContextPointer& renderContext, const Inputs& inputs);

    protected:
        IndexVectors _state;
    };
}
#endif // hifi_workload_RegionState_h
