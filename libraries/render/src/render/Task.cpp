//
//  Task.cpp
//  render/src/render
//
//  Created by Zach Pomerantz on 1/21/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Task.h"

#include <qscriptengine.h> // QObject

#include "Context.h"

#include "gpu/Batch.h"
#include <PerfStat.h>

using namespace render;

void TaskConfig::refresh() {
    _task.configure(*this);
}
