//
//  RenderDeferredTask.h
//  render-utils/src/
//
//  Created by Sam Gateau on 5/29/15.
//  Copyright 20154 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderDeferredTask_h
#define hifi_RenderDeferredTask_h

#include <gpu/Pipeline.h>
#include <render/RenderFetchCullSortTask.h>
#include "LightingModel.h"
#include "LightClusters.h"

class DrawDeferredConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(int numDrawn READ getNumDrawn NOTIFY newStats)
    Q_PROPERTY(int maxDrawn MEMBER maxDrawn NOTIFY dirty)

public:
    int getNumDrawn() { return _numDrawn; }
    void setNumDrawn(int numDrawn) {
        _numDrawn = numDrawn;
        emit newStats();
    }

    int maxDrawn{ -1 };

signals:
    void newStats();
    void dirty();

protected:
    int _numDrawn{ 0 };
};

class DrawDeferred {
public:
    using Inputs = render::VaryingSet4<render::ItemBounds, LightingModelPointer, LightClustersPointer, glm::vec2>;
    using Config = DrawDeferredConfig;
    using JobModel = render::Job::ModelI<DrawDeferred, Inputs, Config>;

    DrawDeferred(render::ShapePlumberPointer shapePlumber)
        : _shapePlumber{ shapePlumber } {}

    void configure(const Config& config) { _maxDrawn = config.maxDrawn; }
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

protected:
    render::ShapePlumberPointer _shapePlumber;
    int _maxDrawn;  // initialized by Config
};

class DrawStateSortConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(int numDrawn READ getNumDrawn NOTIFY numDrawnChanged)
    Q_PROPERTY(int maxDrawn MEMBER maxDrawn NOTIFY dirty)
    Q_PROPERTY(bool stateSort MEMBER stateSort NOTIFY dirty)
public:
    int getNumDrawn() { return numDrawn; }
    void setNumDrawn(int num) {
        numDrawn = num;
        emit numDrawnChanged();
    }

    int maxDrawn{ -1 };
    bool stateSort{ true };

signals:
    void numDrawnChanged();
    void dirty();

protected:
    int numDrawn{ 0 };
};

class DrawStateSortDeferred {
public:
    using Inputs = render::VaryingSet3<render::ItemBounds, LightingModelPointer, glm::vec2>;

    using Config = DrawStateSortConfig;
    using JobModel = render::Job::ModelI<DrawStateSortDeferred, Inputs, Config>;

    DrawStateSortDeferred(render::ShapePlumberPointer shapePlumber)
        : _shapePlumber{ shapePlumber } {}

    void configure(const Config& config) {
        _maxDrawn = config.maxDrawn;
        _stateSort = config.stateSort;
    }
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

protected:
    render::ShapePlumberPointer _shapePlumber;
    int _maxDrawn;  // initialized by Config
    bool _stateSort;
};

class RenderDeferredTaskConfig : public render::Task::Config {
    Q_OBJECT
    Q_PROPERTY(float fadeScale MEMBER fadeScale NOTIFY dirty)
    Q_PROPERTY(float fadeDuration MEMBER fadeDuration NOTIFY dirty)
    Q_PROPERTY(float resolutionScale MEMBER resolutionScale NOTIFY dirty)
    Q_PROPERTY(bool debugFade MEMBER debugFade NOTIFY dirty)
    Q_PROPERTY(float debugFadePercent MEMBER debugFadePercent NOTIFY dirty)
public:
    float fadeScale{ 0.5f };
    float fadeDuration{ 3.0f };
    float resolutionScale{ 1.f };
    float debugFadePercent{ 0.f };
    bool debugFade{ false };

signals:
    void dirty();
};

class RenderDeferredTask {
public:
    using Input = RenderFetchCullSortTask::Output;
    using Config = RenderDeferredTaskConfig;
    using JobModel = render::Task::ModelI<RenderDeferredTask, Input, Config>;

    RenderDeferredTask();

    void configure(const Config& config);
    void build(JobModel& task, const render::Varying& inputs, render::Varying& outputs, bool renderShadows);

private:
    static const render::Varying addSelectItemJobs(JobModel& task,
                                                   const char* selectionName,
                                                   const render::Varying& metas,
                                                   const render::Varying& opaques,
                                                   const render::Varying& transparents);
};

#endif  // hifi_RenderDeferredTask_h
