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

const Selection::Name ZoneRendererTask::ZONES_SELECTION { "RankedZones" };

ZoneRendererTask::ZoneRendererTask(const Inputs& inputs) {

    const auto zoneItems = addJob<SelectItems>("FilterZones", inputs, ZONES_SELECTION);

    // just draw them...
    addJob<DrawBounds>("DrawZones", zoneItems);

}


