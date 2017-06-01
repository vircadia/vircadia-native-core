//
//  StencilMaskPass.h
//  render-utils/src/
//
//  Created by Sam Gateau on 5/31/17.
//  Copyright 20154 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_StencilMaskPass_h
#define hifi_StencilMaskPass_h

#include <render/Engine.h>
#include <gpu/Pipeline.h>
#include <model/Geometry.h>

class PrepareStencil {
public:
    class Config : public render::JobConfig {
    public:
        Config(bool enabled = true) : JobConfig(enabled) {}
    };

    using JobModel = render::Job::ModelI<PrepareStencil, gpu::FramebufferPointer, Config>;

    void configure(const Config& config) {}

    void run(const render::RenderContextPointer& renderContext, const gpu::FramebufferPointer& dstFramebuffer);

    static const gpu::int8 STENCIL_MASK = 2;
    static const gpu::int8 STENCIL_BACKGROUND = 1;
    static const gpu::int8 STENCIL_SHAPE = 0;


    static void drawMask(gpu::State& state);
    static void testMask(gpu::State& state);
    static void testBackground(gpu::State& state);
    static void testMaskDrawShape(gpu::State& state);


private:
    gpu::PipelinePointer _meshStencilPipeline;
    gpu::PipelinePointer getMeshStencilPipeline();

    gpu::PipelinePointer _paintStencilPipeline;
    gpu::PipelinePointer getPaintStencilPipeline();

    model::MeshPointer _mesh;
    model::MeshPointer getMesh();

};


#endif // hifi_StencilMaskPass_h
