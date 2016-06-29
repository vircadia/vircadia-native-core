//
//  Created by Bradley Austin Davis on 2016/05/16
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "TestHelpers.h"

class FloorTextureTest : public GpuTestBase {
    gpu::BufferPointer vertexBuffer { std::make_shared<gpu::Buffer>() };
    gpu::BufferPointer indexBuffer { std::make_shared<gpu::Buffer>() };
    gpu::Stream::FormatPointer vertexFormat { std::make_shared<gpu::Stream::Format>() };
    gpu::TexturePointer texture;
public:
    FloorTextureTest();
    void renderTest(size_t testId, RenderArgs* args) override;
};


