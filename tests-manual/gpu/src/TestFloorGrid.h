//
//  Created by Bradley Austin Davis on 2016/05/16
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include <vector>
#include <GLMHelpers.h>
#include <Transform.h>

#include <gpu/Resource.h>
#include <gpu/Stream.h>

#include "TestHelpers.h"

class TestFloorGrid : public GpuTestBase {
public:
    TestFloorGrid();
    void renderTest(size_t testId, RenderArgs* args) override;
};


