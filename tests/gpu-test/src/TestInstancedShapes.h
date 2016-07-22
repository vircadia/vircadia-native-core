//
//  Created by Bradley Austin Davis on 2016/05/16
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include "TestHelpers.h"

class TestInstancedShapes : public GpuTestBase {

    std::vector<std::vector<mat4>> transforms;
    std::vector<vec4> colors;
    gpu::BufferPointer colorBuffer;
    gpu::BufferView instanceXfmView;
public:
    TestInstancedShapes();
    void renderTest(size_t testId, RenderArgs* args) override;
};

