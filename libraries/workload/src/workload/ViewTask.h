//
//  ViewTask.h
//  libraries/workload/src/workload
//
//  Created by Sam Gateau 2018.03.05
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_workload_ViewTask_h
#define hifi_workload_ViewTask_h

#include "Engine.h"

namespace workload {
    class SetupViewsConfig : public Job::Config{
        Q_OBJECT
    public:
        SetupViewsConfig() : Job::Config(true) {}
    };

    class SetupViews {
    public:
        using Config = SetupViewsConfig;
        using Input = Views;
        using JobModel = Job::ModelI<SetupViews, Input, Config>;

        void configure(const Config& config);
        void run(const workload::WorkloadContextPointer& renderContext, const Input& inputs);
    };

} // namespace workload

#endif // hifi_workload_ViewTask_h