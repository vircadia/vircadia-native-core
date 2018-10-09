//
//  Created by Bradley Austin Davis on 2018/01/09
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderCommonTask.h"

#include <gpu/Context.h>
#include <graphics/ShaderConstants.h>

#include "render-utils/ShaderConstants.h"
#include "DeferredLightingEffect.h"
#include "RenderUtilsLogging.h"

namespace ru {
    using render_utils::slot::texture::Texture;
    using render_utils::slot::buffer::Buffer;
}

namespace gr {
    using graphics::slot::texture::Texture;
    using graphics::slot::buffer::Buffer;
}


using namespace render;
extern void initForwardPipelines(ShapePlumber& plumber);

void BeginGPURangeTimer::run(const render::RenderContextPointer& renderContext, gpu::RangeTimerPointer& timer) {
    timer = _gpuTimer;
    gpu::doInBatch("BeginGPURangeTimer", renderContext->args->_context, [&](gpu::Batch& batch) {
        _gpuTimer->begin(batch);
    });
}

void EndGPURangeTimer::run(const render::RenderContextPointer& renderContext, const gpu::RangeTimerPointer& timer) {
    gpu::doInBatch("EndGPURangeTimer", renderContext->args->_context, [&](gpu::Batch& batch) {
        timer->end(batch);
    });
    
    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);
    config->setGPUBatchRunTime(timer->getGPUAverage(), timer->getBatchAverage());
}

DrawOverlay3D::DrawOverlay3D(bool opaque) :
    _shapePlumber(std::make_shared<ShapePlumber>()),
    _opaquePass(opaque) {
    initForwardPipelines(*_shapePlumber);
}

void DrawOverlay3D::run(const RenderContextPointer& renderContext, const Inputs& inputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);

    const auto& inItems = inputs.get0();
    const auto& lightingModel = inputs.get1();
    const auto jitter = inputs.get2();
    
    config->setNumDrawn((int)inItems.size());
    emit config->numDrawnChanged();

    RenderArgs* args = renderContext->args;

    // Clear the framebuffer without stereo
    // Needs to be distinct from the other batch because using the clear call 
    // while stereo is enabled triggers a warning
    if (_opaquePass) {
        gpu::doInBatch("DrawOverlay3D::run::clear", args->_context, [&](gpu::Batch& batch) {
            batch.enableStereo(false);
            batch.clearFramebuffer(gpu::Framebuffer::BUFFER_DEPTH, glm::vec4(), 1.f, 0, false);
        });
    }

    if (!inItems.empty()) {
        // Render the items
        gpu::doInBatch("DrawOverlay3D::main", args->_context, [&](gpu::Batch& batch) {
            args->_batch = &batch;
            batch.setViewportTransform(args->_viewport);
            batch.setStateScissorRect(args->_viewport);

            glm::mat4 projMat;
            Transform viewMat;
            args->getViewFrustum().evalProjectionMatrix(projMat);
            args->getViewFrustum().evalViewTransform(viewMat);

            batch.setProjectionTransform(projMat);
            batch.setProjectionJitter(jitter.x, jitter.y);
            batch.setViewTransform(viewMat);

            // Setup lighting model for all items;
            batch.setUniformBuffer(ru::Buffer::LightModel, lightingModel->getParametersBuffer());

            renderShapes(renderContext, _shapePlumber, inItems, _maxDrawn);
            args->_batch = nullptr;
        });
    }
}

void CompositeHUD::run(const RenderContextPointer& renderContext) {
    assert(renderContext->args);
    assert(renderContext->args->_context);

    // We do not want to render HUD elements in secondary camera
    if (renderContext->args->_renderMode == RenderArgs::RenderMode::SECONDARY_CAMERA_RENDER_MODE) {
        return;
    }

    // Grab the HUD texture
#if !defined(DISABLE_QML)
    gpu::doInBatch("CompositeHUD", renderContext->args->_context, [&](gpu::Batch& batch) {
        glm::mat4 projMat;
        Transform viewMat;
        renderContext->args->getViewFrustum().evalProjectionMatrix(projMat);
        renderContext->args->getViewFrustum().evalViewTransform(viewMat);
        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat, true);
        if (renderContext->args->_hudOperator) {
            renderContext->args->_hudOperator(batch, renderContext->args->_hudTexture, renderContext->args->_renderMode == RenderArgs::RenderMode::MIRROR_RENDER_MODE);
        }
    });
#endif
}

void Blit::run(const RenderContextPointer& renderContext, const gpu::FramebufferPointer& srcFramebuffer) {
    assert(renderContext->args);
    assert(renderContext->args->_context);

    RenderArgs* renderArgs = renderContext->args;
    auto blitFbo = renderArgs->_blitFramebuffer;

    if (!blitFbo) {
        qCWarning(renderutils) << "Blit::run - no blit frame buffer.";
        return;
    }

    // Determine size from viewport
    int width = renderArgs->_viewport.z;
    int height = renderArgs->_viewport.w;

    // Blit primary to blit FBO
    auto primaryFbo = srcFramebuffer;

    gpu::doInBatch("Blit", renderArgs->_context, [&](gpu::Batch& batch) {
        batch.setFramebuffer(blitFbo);

        if (renderArgs->_renderMode == RenderArgs::MIRROR_RENDER_MODE) {
            if (renderArgs->isStereo()) {
                gpu::Vec4i srcRectLeft;
                srcRectLeft.z = width / 2;
                srcRectLeft.w = height;

                gpu::Vec4i srcRectRight;
                srcRectRight.x = width / 2;
                srcRectRight.z = width;
                srcRectRight.w = height;

                gpu::Vec4i destRectLeft;
                destRectLeft.x = srcRectLeft.z;
                destRectLeft.z = srcRectLeft.x;
                destRectLeft.y = srcRectLeft.y;
                destRectLeft.w = srcRectLeft.w;

                gpu::Vec4i destRectRight;
                destRectRight.x = srcRectRight.z;
                destRectRight.z = srcRectRight.x;
                destRectRight.y = srcRectRight.y;
                destRectRight.w = srcRectRight.w;

                // Blit left to right and right to left in stereo
                batch.blit(primaryFbo, srcRectRight, blitFbo, destRectLeft);
                batch.blit(primaryFbo, srcRectLeft, blitFbo, destRectRight);
            } else {
                gpu::Vec4i srcRect;
                srcRect.z = width;
                srcRect.w = height;

                gpu::Vec4i destRect;
                destRect.x = width;
                destRect.y = 0;
                destRect.z = 0;
                destRect.w = height;

                batch.blit(primaryFbo, srcRect, blitFbo, destRect);
            }
        } else {
            gpu::Vec4i rect;
            rect.z = width;
            rect.w = height;

            batch.blit(primaryFbo, rect, blitFbo, rect);
        }
    });
}

void ExtractFrustums::run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& output) {
    assert(renderContext->args);
    assert(renderContext->args->_context);

    RenderArgs* args = renderContext->args;

    const auto& lightFrame = inputs;

    // Return view frustum
    auto& viewFrustum = output[VIEW_FRUSTUM].edit<ViewFrustumPointer>();
    if (!viewFrustum) {
        viewFrustum = std::make_shared<ViewFrustum>(args->getViewFrustum());
    } else {
        *viewFrustum = args->getViewFrustum();
    }

    // Return shadow frustum
    auto lightStage = args->_scene->getStage<LightStage>(LightStage::getName());
    for (auto i = 0; i < SHADOW_CASCADE_FRUSTUM_COUNT; i++) {
        auto& shadowFrustum = output[SHADOW_CASCADE0_FRUSTUM+i].edit<ViewFrustumPointer>();
        if (lightStage) {
            auto globalShadow = lightStage->getCurrentKeyShadow(*lightFrame);

            if (globalShadow && i<(int)globalShadow->getCascadeCount()) {
                auto& cascade = globalShadow->getCascade(i);
                shadowFrustum = cascade.getFrustum();
            } else {
                shadowFrustum.reset();
            }
        } else {
            shadowFrustum.reset();
        }
    }
}

void FetchCurrentFrames::run(const render::RenderContextPointer& renderContext, Outputs& outputs) {
    auto lightStage = renderContext->_scene->getStage<LightStage>();
    assert(lightStage);
    outputs.edit0() = std::make_shared<LightStage::Frame>(lightStage->_currentFrame);

    auto backgroundStage = renderContext->_scene->getStage<BackgroundStage>();
    assert(backgroundStage);
    outputs.edit1() = std::make_shared<BackgroundStage::Frame>(backgroundStage->_currentFrame);

    auto hazeStage = renderContext->_scene->getStage<HazeStage>();
    assert(hazeStage);
    outputs.edit2() = std::make_shared<HazeStage::Frame>(hazeStage->_currentFrame);

    auto bloomStage = renderContext->_scene->getStage<BloomStage>();
    assert(bloomStage);
    outputs.edit3() = std::make_shared<BloomStage::Frame>(bloomStage->_currentFrame);
}
