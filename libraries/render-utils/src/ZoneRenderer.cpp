//
//  ZoneRenderer.cpp
//  render/src/render-utils
//
//  Created by Sam Gateau on 4/4/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "ZoneRenderer.h"

#include <render/FilterTask.h>
#include <render/DrawTask.h>

using namespace render;

class SetupZones {
public:
    using Inputs = render::ItemBounds;
    using JobModel = render::Job::ModelI<SetupZones, Inputs>;

    SetupZones() {}

    void run(const RenderContextPointer& context, const Inputs& inputs);

protected:
};

const Selection::Name ZoneRendererTask::ZONES_SELECTION { "RankedZones" };

void ZoneRendererTask::build(JobModel& task, const Varying& input, Varying& ouput) {
    // Filter out the sorted list of zones
    const auto zoneItems = task.addJob<render::SelectSortItems>("FilterZones", input, ZONES_SELECTION.c_str());

    // just setup the current zone env
    task.addJob<SetupZones>("SetupZones", zoneItems);

    ouput = zoneItems;
}

void SetupZones::run(const RenderContextPointer& context, const Inputs& inputs) {
    
    // call render in the correct order first...
    render::renderItems(context, inputs);

}
