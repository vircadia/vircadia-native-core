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

class PrepareStencilConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(int maskMode MEMBER maskMode NOTIFY dirty)
    Q_PROPERTY(bool forceDraw MEMBER forceDraw NOTIFY dirty)

public:
    PrepareStencilConfig(bool enabled = true) : JobConfig(enabled) {}

    int maskMode { 0 };
    bool forceDraw { false };

signals:
    void dirty();
};

class PrepareStencil {
public:
    using Config = PrepareStencilConfig;

    using JobModel = render::Job::ModelI<PrepareStencil, gpu::FramebufferPointer, Config>;

    void configure(const Config& config);

    void run(const render::RenderContextPointer& renderContext, const gpu::FramebufferPointer& dstFramebuffer);

    static const gpu::int8 STENCIL_SHAPE = 0;
    static const gpu::int8 STENCIL_BACKGROUND = 1 << 0;
    static const gpu::int8 STENCIL_MASK = 1 << 1;
    static const gpu::int8 STENCIL_NO_AA = 1 << 2;


    static void drawMask(gpu::State& state);
    static void testMask(gpu::State& state);
    static void testMaskNoAA(gpu::State& state);
    static void testBackground(gpu::State& state);
    static void testMaskDrawShape(gpu::State& state);
    static void testMaskDrawShapeNoAA(gpu::State& state);
    static void testShape(gpu::State& state);


private:
    gpu::PipelinePointer _meshStencilPipeline;
    gpu::PipelinePointer getMeshStencilPipeline();

    gpu::PipelinePointer _paintStencilPipeline;
    gpu::PipelinePointer getPaintStencilPipeline();

    model::MeshPointer _mesh;
    model::MeshPointer getMesh();

    int _maskMode { 0 };
    bool _forceDraw { false };
};


#endif // hifi_StencilMaskPass_h
