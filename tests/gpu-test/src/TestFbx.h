//
//  Created by Bradley Austin Davis on 2016/05/16
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "TestHelpers.h"

#include <render/ShapePipeline.h>

class FBXGeometry;

class TestFbx : public GpuTestBase {
    size_t _partCount { 0 };
    model::Material _material;
    render::ShapeKey _shapeKey;
    std::vector<mat4> _partTransforms;
    render::ShapePlumberPointer _shapePlumber;
    gpu::Stream::FormatPointer _vertexFormat { std::make_shared<gpu::Stream::Format>() };
    gpu::BufferPointer _vertexBuffer { std::make_shared<gpu::Buffer>() };
    gpu::BufferPointer _indexBuffer { std::make_shared<gpu::Buffer>() };
    gpu::BufferPointer _indirectBuffer { std::make_shared<gpu::Buffer>() };
public:
    TestFbx(const render::ShapePlumberPointer& shapePlumber);
    bool isReady() const override; 
    void renderTest(size_t test, RenderArgs* args) override;

private:
    void parseFbx(const QByteArray& fbxData);
};


