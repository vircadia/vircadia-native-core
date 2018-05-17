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
#include <GeometryCache.h>
#include <NumericalConstants.h>

#include <gpu/Resource.h>
#include <gpu/Forward.h>
#include <gpu/Shader.h>
#include <gpu/Stream.h>

class GpuTestBase {
public:
    virtual ~GpuTestBase() {}
    virtual bool isReady() const { return true; }
    virtual size_t getTestCount() const { return 1; }
    virtual void renderTest(size_t test, RenderArgs* args) = 0;
};

uint32_t toCompactColor(const glm::vec4& color);
gpu::ShaderPointer makeShader(const std::string & vertexShaderSrc, const std::string & fragmentShaderSrc, const gpu::Shader::BindingSet & bindings);

