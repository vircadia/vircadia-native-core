//
//  SurfaceGeometryPass.cpp
//  libraries/render-utils/src/
//
//  Created by Sam Gateau 6/3/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "SurfaceGeometryPass.h"

#include <gpu/Context.h>
#include <gpu/StandardShaderLib.h>

#include "FramebufferCache.h"

const int SurfaceGeometryPass_FrameTransformSlot = 0;
const int SurfaceGeometryPass_ParamsSlot = 1;
const int SurfaceGeometryPass_DepthMapSlot = 0;

#include "surfaceGeometry_makeCurvature_frag.h"

SurfaceGeometryPass::SurfaceGeometryPass() {
}

void SurfaceGeometryPass::configure(const Config& config) {
}

void SurfaceGeometryPass::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext, const DeferredFrameTransformPointer& frameTransform) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    RenderArgs* args = renderContext->args;


    auto framebufferCache = DependencyManager::get<FramebufferCache>();
    auto depthBuffer = framebufferCache->getPrimaryDepthTexture();
  //  auto normalBuffer = framebufferCache->getDeferredNormalTexture();
 //   auto pyramidFBO = framebufferCache->getDepthPyramidFramebuffer();
    auto curvatureFBO = framebufferCache->getCurvatureFramebuffer();

    QSize framebufferSize = framebufferCache->getFrameBufferSize();
    float sMin = args->_viewport.x / (float)framebufferSize.width();
    float sWidth = args->_viewport.z / (float)framebufferSize.width();
    float tMin = args->_viewport.y / (float)framebufferSize.height();
    float tHeight = args->_viewport.w / (float)framebufferSize.height();


    auto curvaturePipeline = getCurvaturePipeline();

    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        batch.enableStereo(false);

   //     _gpuTimer.begin(batch);

        batch.setViewportTransform(args->_viewport);
        batch.setProjectionTransform(glm::mat4());
        batch.setViewTransform(Transform());

        Transform model;
        model.setTranslation(glm::vec3(sMin, tMin, 0.0f));
        model.setScale(glm::vec3(sWidth, tHeight, 1.0f));
        batch.setModelTransform(model);

        batch.setUniformBuffer(SurfaceGeometryPass_FrameTransformSlot, frameTransform->getFrameTransformBuffer());
        //   batch.setUniformBuffer(SurfaceGeometryPass_ParamsSlot, _parametersBuffer);


        // Pyramid pass
        batch.setFramebuffer(curvatureFBO);
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(args->getViewFrustum().getFarClip(), 0.0f, 0.0f, 0.0f));
        batch.setPipeline(curvaturePipeline);
        batch.setResourceTexture(SurfaceGeometryPass_DepthMapSlot, depthBuffer);
        batch.draw(gpu::TRIANGLE_STRIP, 4);

    });

}

const gpu::PipelinePointer& SurfaceGeometryPass::getCurvaturePipeline() {
    if (!_curvaturePipeline) {
        auto vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(surfaceGeometry_makeCurvature_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("deferredFrameTransformBuffer"), SurfaceGeometryPass_FrameTransformSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("ambientOcclusionParamsBuffer"), SurfaceGeometryPass_ParamsSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("depthMap"), SurfaceGeometryPass_DepthMapSlot));
        gpu::Shader::makeProgram(*program, slotBindings);


        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        // Stencil test the curvature pass for objects pixels only, not the background
        state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));

      //  state->setColorWriteMask(true, false, false, false);

        // Good to go add the brand new pipeline
        _curvaturePipeline = gpu::Pipeline::create(program, state);
    }

    return _curvaturePipeline;
}

