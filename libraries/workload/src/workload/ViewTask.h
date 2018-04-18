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
        Q_PROPERTY(float r1Front READ getR1Front WRITE setR1Front NOTIFY dirty)
        Q_PROPERTY(float r1Back READ getR1Back WRITE setR1Back NOTIFY dirty)
        Q_PROPERTY(float r2Front READ getR2Front WRITE setR2Front NOTIFY dirty)
        Q_PROPERTY(float r2Back READ getR2Back WRITE setR2Back NOTIFY dirty)
        Q_PROPERTY(float r3Front READ getR3Front WRITE setR3Front NOTIFY dirty)
        Q_PROPERTY(float r3Back READ getR3Back WRITE setR3Back NOTIFY dirty)
        Q_PROPERTY(bool freezeViews READ getFreezeView WRITE setFreezeView NOTIFY dirty)
        Q_PROPERTY(bool useAvatarView READ useAvatarView WRITE setUseAvatarView NOTIFY dirty)
        Q_PROPERTY(bool forceViewHorizontal READ forceViewHorizontal WRITE setForceViewHorizontal NOTIFY dirty)

        Q_PROPERTY(bool simulateSecondaryCamera READ simulateSecondaryCamera WRITE setSimulateSecondaryCamera NOTIFY dirty)
    public:


        float getR1Front() const { return data.r1Front; }
        float getR1Back() const { return data.r1Back; }
        float getR2Front() const { return data.r2Front; }
        float getR2Back() const { return data.r2Back; }
        float getR3Front() const { return data.r3Front; }
        float getR3Back() const { return data.r3Back; }

        void setR1Front(float d) { data.r1Front = d; emit dirty(); }
        void setR1Back(float d) { data.r1Back = d; emit dirty(); }
        void setR2Front(float d) { data.r2Front = d; emit dirty(); }
        void setR2Back(float d) { data.r2Back = d; emit dirty(); }
        void setR3Front(float d) { data.r3Front = d; emit dirty(); }
        void setR3Back(float d) { data.r3Back = d; emit dirty(); }

        bool getFreezeView() const { return data.freezeViews; }
        void setFreezeView(bool freeze) { data.freezeViews = freeze; emit dirty(); }
        bool useAvatarView() const { return data.useAvatarView; }
        void setUseAvatarView(bool use) { data.useAvatarView = use; emit dirty(); }
        bool forceViewHorizontal() const { return data.forceViewHorizontal; }
        void setForceViewHorizontal(bool use) { data.forceViewHorizontal = use; emit dirty(); }

        bool simulateSecondaryCamera() const { return data.simulateSecondaryCamera; }
        void setSimulateSecondaryCamera(bool use) { data.simulateSecondaryCamera = use; emit dirty(); }

        struct Data {
            float r1Back { 2.0f };
            float r1Front { 10.0f };

            float r2Back{ 5.0f };
            float r2Front{ 30.0f };

            float r3Back{ 10.0f };
            float r3Front{ 100.0f };

            bool freezeViews{ false };
            bool useAvatarView{ false };
            bool forceViewHorizontal{ false };
            bool simulateSecondaryCamera{ false };
        } data;

    signals:
        void dirty();
    };

    class SetupViews {
    public:
        using Config = SetupViewsConfig;
        using Input = Views;
        using Output = Views;
        using JobModel = Job::ModelIO<SetupViews, Input, Output, Config>;

        void configure(const Config& config);
        void run(const workload::WorkloadContextPointer& renderContext, const Input& inputs, Output& outputs);

    protected:
        Config::Data data;
        Views _views;
    };

    class AssignSpaceViews {
    public:
        using Input = Views;
        using JobModel = Job::ModelI<AssignSpaceViews, Input>;

        void run(const workload::WorkloadContextPointer& renderContext, const Input& inputs);
    };

} // namespace workload

#endif // hifi_workload_ViewTask_h