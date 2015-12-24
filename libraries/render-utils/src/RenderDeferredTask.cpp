
//
//  RenderDeferredTask.cpp
//  render-utils/src/
//
//  Created by Sam Gateau on 5/29/15.
//  Copyright 20154 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "RenderDeferredTask.h"

#include <PerfStat.h>
#include <PathUtils.h>
#include <RenderArgs.h>
#include <ViewFrustum.h>
#include <gpu/Context.h>
#include <gpu/StandardShaderLib.h>

#include "DebugDeferredBuffer.h"
#include "DeferredLightingEffect.h"
#include "FramebufferCache.h"
#include "HitEffect.h"
#include "TextureCache.h"

#include "render/DrawStatus.h"
#include "AmbientOcclusionEffect.h"
#include "AntialiasingEffect.h"

#include "overlay3D_vert.h"
#include "overlay3D_frag.h"

#include "drawOpaqueStencil_frag.h"

using namespace render;


void PrepareDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    DependencyManager::get<DeferredLightingEffect>()->prepare(renderContext->getArgs());
}

void RenderDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    DependencyManager::get<DeferredLightingEffect>()->render(renderContext->getArgs());
}

void ToneMappingDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    PerformanceTimer perfTimer("ToneMappingDeferred");
    _toneMappingEffect.render(renderContext->getArgs());
}

RenderDeferredTask::RenderDeferredTask() : Task() {
    // CPU only, create the list of renderedOpaques items
    _jobs.push_back(Job(new FetchItems::JobModel("FetchOpaque",
        FetchItems([](const RenderContextPointer& context, int count) {
            context->getItemsConfig().opaque.numFeed = count;
        })
    )));
    _jobs.push_back(Job(new CullItemsOpaque::JobModel("CullOpaque", _jobs.back().getOutput())));
    _jobs.push_back(Job(new DepthSortItems::JobModel("DepthSortOpaque", _jobs.back().getOutput())));
    auto& renderedOpaques = _jobs.back().getOutput();

    // CPU only, create the list of renderedTransparents items
    _jobs.push_back(Job(new FetchItems::JobModel("FetchTransparent",
        FetchItems(ItemFilter::Builder::transparentShape().withoutLayered(),
            [](const RenderContextPointer& context, int count) {
                context->getItemsConfig().transparent.numFeed = count;
        })
     )));
    _jobs.push_back(Job(new CullItemsTransparent::JobModel("CullTransparent", _jobs.back().getOutput())));
    _jobs.push_back(Job(new DepthSortItems::JobModel("DepthSortTransparent", _jobs.back().getOutput(), DepthSortItems(false))));
    auto& renderedTransparents = _jobs.back().getOutput();

    // GPU Jobs: Start preparing the deferred and lighting buffer
    _jobs.push_back(Job(new PrepareDeferred::JobModel("PrepareDeferred")));

    // Render opaque objects in DeferredBuffer
    _jobs.push_back(Job(new DrawOpaqueDeferred::JobModel("DrawOpaqueDeferred", renderedOpaques)));

    // Once opaque is all rendered create stencil background
    _jobs.push_back(Job(new DrawStencilDeferred::JobModel("DrawOpaqueStencil")));

    // Use Stencil and start drawing background in Lighting buffer
    _jobs.push_back(Job(new DrawBackgroundDeferred::JobModel("DrawBackgroundDeferred")));

    // Draw Lights just add the lights to the current list of lights to deal with. NOt really gpu job for now.
    _jobs.push_back(Job(new DrawLight::JobModel("DrawLight")));

    // DeferredBuffer is complete, now let's shade it into the LightingBuffer
    _jobs.push_back(Job(new RenderDeferred::JobModel("RenderDeferred")));

    // AO job, to be revisited
    _jobs.push_back(Job(new AmbientOcclusion::JobModel("AmbientOcclusion")));
    _jobs.back().setEnabled(false);
    _occlusionJobIndex = (int)_jobs.size() - 1;

    // AA job to be revisited
    _jobs.push_back(Job(new Antialiasing::JobModel("Antialiasing")));
    _jobs.back().setEnabled(false);
    _antialiasingJobIndex = (int)_jobs.size() - 1;

    // Render transparent objects forward in LigthingBuffer
    _jobs.push_back(Job(new DrawTransparentDeferred::JobModel("TransparentDeferred", renderedTransparents)));
    
    // Lighting Buffer ready for tone mapping
    _jobs.push_back(Job(new ToneMappingDeferred::JobModel("ToneMapping")));
    _toneMappingJobIndex = (int)_jobs.size() - 1;

    // Debugging Deferred buffer job
    _jobs.push_back(Job(new DebugDeferredBuffer::JobModel("DebugDeferredBuffer")));
    _jobs.back().setEnabled(false);
    _drawDebugDeferredBufferIndex = (int)_jobs.size() - 1;

    // Status icon rendering job
    {
        // Grab a texture map representing the different status icons and assign that to the drawStatsuJob
        auto iconMapPath = PathUtils::resourcesPath() + "icons/statusIconAtlas.svg";
        auto statusIconMap = DependencyManager::get<TextureCache>()->getImageTexture(iconMapPath);
        _jobs.push_back(Job(new render::DrawStatus::JobModel("DrawStatus", renderedOpaques, DrawStatus(statusIconMap))));
        _jobs.back().setEnabled(false);
        _drawStatusJobIndex = (int)_jobs.size() - 1;
    }

    _jobs.push_back(Job(new DrawOverlay3D::JobModel("DrawOverlay3D")));

    _jobs.push_back(Job(new HitEffect::JobModel("HitEffect")));
    _jobs.back().setEnabled(false);
    _drawHitEffectJobIndex = (int)_jobs.size() -1;

    _jobs.push_back(Job(new Blit::JobModel("Blit")));
}

RenderDeferredTask::~RenderDeferredTask() {
}

void RenderDeferredTask::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    // sanity checks
    assert(sceneContext);
    if (!sceneContext->_scene) {
        return;
    }


    // Is it possible that we render without a viewFrustum ?
    if (!(renderContext->getArgs() && renderContext->getArgs()->_viewFrustum)) {
        return;
    }

    // Make sure we turn the deferred buffer debug on/off
    setDrawDebugDeferredBuffer(renderContext->_deferredDebugMode);
    
    // Make sure we turn the displayItemStatus on/off
    setDrawItemStatus(renderContext->getDrawStatus());
    
    // Make sure we display hit effect on screen, as desired from a script
    setDrawHitEffect(renderContext->getDrawHitEffect());
    

    // TODO: turn on/off AO through menu item
    setOcclusionStatus(renderContext->getOcclusionStatus());

    setAntialiasingStatus(renderContext->getFxaaStatus());

    setToneMappingExposure(renderContext->getTone().exposure);
    setToneMappingToneCurve(renderContext->getTone().toneCurve);

    renderContext->getArgs()->_context->syncCache();

    for (auto job : _jobs) {
        job.run(sceneContext, renderContext);
    }

};

void DrawOpaqueDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems) {
    assert(renderContext->getArgs());
    assert(renderContext->getArgs()->_viewFrustum);

    RenderArgs* args = renderContext->getArgs();
    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);
        args->_batch = &batch;

        auto& opaque = renderContext->getItemsConfig().opaque;
        opaque.numDrawn = (int)inItems.size();

        glm::mat4 projMat;
        Transform viewMat;
        args->_viewFrustum->evalProjectionMatrix(projMat);
        args->_viewFrustum->evalViewTransform(viewMat);

        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);

        renderItems(sceneContext, renderContext, inItems, opaque.maxDrawn);
        args->_batch = nullptr;
    });
}

void DrawTransparentDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems) {
    assert(renderContext->getArgs());
    assert(renderContext->getArgs()->_viewFrustum);

    RenderArgs* args = renderContext->getArgs();
    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);
        args->_batch = &batch;
    
        auto& transparent = renderContext->getItemsConfig().transparent;
        transparent.numDrawn = (int)inItems.size();

        glm::mat4 projMat;
        Transform viewMat;
        args->_viewFrustum->evalProjectionMatrix(projMat);
        args->_viewFrustum->evalViewTransform(viewMat);

        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);

        renderItems(sceneContext, renderContext, inItems, transparent.maxDrawn);
        args->_batch = nullptr;
    });
}

gpu::PipelinePointer DrawOverlay3D::_opaquePipeline;
const gpu::PipelinePointer& DrawOverlay3D::getOpaquePipeline() {
    if (!_opaquePipeline) {
        auto vs = gpu::Shader::createVertex(std::string(overlay3D_vert));
        auto ps = gpu::Shader::createPixel(std::string(overlay3D_frag));
        auto program = gpu::Shader::createProgram(vs, ps);
        
        auto state = std::make_shared<gpu::State>();
        state->setDepthTest(false);
        // additive blending
        state->setBlendFunction(true, gpu::State::ONE, gpu::State::BLEND_OP_ADD, gpu::State::ONE);

        _opaquePipeline = gpu::Pipeline::create(program, state);
    }
    return _opaquePipeline;
}

void DrawOverlay3D::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    assert(renderContext->getArgs());
    assert(renderContext->getArgs()->_viewFrustum);

    // render backgrounds
    auto& scene = sceneContext->_scene;
    auto& items = scene->getMasterBucket().at(ItemFilter::Builder::opaqueShape().withLayered());


    ItemIDsBounds inItems;
    inItems.reserve(items.size());
    for (auto id : items) {
        auto& item = scene->getItem(id);
        if (item.getKey().isVisible() && (item.getLayer() == 1)) {
            inItems.emplace_back(id);
        }
    }
    auto& overlay3D = renderContext->getItemsConfig().overlay3D;
    overlay3D.numFeed = (int)inItems.size();
    overlay3D.numDrawn = (int)inItems.size();

    if (!inItems.empty()) {
        RenderArgs* args = renderContext->getArgs();

        // Clear the framebuffer without stereo
        // Needs to be distinct from the other batch because using the clear call 
        // while stereo is enabled triggers a warning
        {
            gpu::Batch batch;
            batch.enableStereo(false);
            batch.clearFramebuffer(gpu::Framebuffer::BUFFER_DEPTH, glm::vec4(), 1.f, 0, true);
            args->_context->render(batch);
        }

        // Render the items
        gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
            args->_batch = &batch;
            args->_whiteTexture = DependencyManager::get<TextureCache>()->getWhiteTexture();

            glm::mat4 projMat;
            Transform viewMat;
            args->_viewFrustum->evalProjectionMatrix(projMat);
            args->_viewFrustum->evalViewTransform(viewMat);

            batch.setProjectionTransform(projMat);
            batch.setViewTransform(viewMat);
            batch.setViewportTransform(args->_viewport);
            batch.setStateScissorRect(args->_viewport);

            batch.setPipeline(getOpaquePipeline());
            batch.setResourceTexture(0, args->_whiteTexture);
            renderItems(sceneContext, renderContext, inItems, renderContext->getItemsConfig().overlay3D.maxDrawn);
        });
        args->_batch = nullptr;
        args->_whiteTexture.reset();
    }
}

gpu::PipelinePointer DrawStencilDeferred::_opaquePipeline;
const gpu::PipelinePointer& DrawStencilDeferred::getOpaquePipeline() {
    if (!_opaquePipeline) {
        const gpu::int8 STENCIL_OPAQUE = 1;
        auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(drawOpaqueStencil_frag));
        auto program = gpu::Shader::createProgram(vs, ps);
        

        gpu::Shader::makeProgram((*program));

        auto state = std::make_shared<gpu::State>();
        state->setDepthTest(true, false, gpu::LESS_EQUAL);
        state->setStencilTest(true, 0xFF, gpu::State::StencilTest(STENCIL_OPAQUE, 0xFF, gpu::ALWAYS, gpu::State::STENCIL_OP_REPLACE, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_REPLACE)); 
        state->setColorWriteMask(0);

        _opaquePipeline = gpu::Pipeline::create(program, state);
    }
    return _opaquePipeline;
}

void DrawStencilDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    assert(renderContext->getArgs());
    assert(renderContext->getArgs()->_viewFrustum);

    // from the touched pixel generate the stencil buffer 
    RenderArgs* args = renderContext->getArgs();
    doInBatch(args->_context, [=](gpu::Batch& batch) {
        args->_batch = &batch;

        auto deferredFboColorDepthStencil = DependencyManager::get<FramebufferCache>()->getDeferredFramebufferDepthColor();

        batch.enableStereo(false);

        batch.setFramebuffer(deferredFboColorDepthStencil);
        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);

        batch.setPipeline(getOpaquePipeline());

        batch.draw(gpu::TRIANGLE_STRIP, 4);
        batch.setResourceTexture(0, nullptr);

    });
    args->_batch = nullptr;
}

void DrawBackgroundDeferred::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    assert(renderContext->getArgs());
    assert(renderContext->getArgs()->_viewFrustum);

    // render backgrounds
    auto& scene = sceneContext->_scene;
    auto& items = scene->getMasterBucket().at(ItemFilter::Builder::background());


    ItemIDsBounds inItems;
    inItems.reserve(items.size());
    for (auto id : items) {
        inItems.emplace_back(id);
    }
    RenderArgs* args = renderContext->getArgs();
    doInBatch(args->_context, [=](gpu::Batch& batch) {
        args->_batch = &batch;

        auto lightingFBO = DependencyManager::get<FramebufferCache>()->getLightingFramebuffer();

        batch.enableSkybox(true);

        batch.setFramebuffer(lightingFBO);

        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);

        glm::mat4 projMat;
        Transform viewMat;
        args->_viewFrustum->evalProjectionMatrix(projMat);
        args->_viewFrustum->evalViewTransform(viewMat);

        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);

        renderItems(sceneContext, renderContext, inItems);

    });
    args->_batch = nullptr;
}

void Blit::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    assert(renderContext->getArgs());
    assert(renderContext->getArgs()->_context);

    RenderArgs* renderArgs = renderContext->getArgs();
    auto blitFbo = renderArgs->_blitFramebuffer;

    if (!blitFbo) {
        return;
    }

    // Determine size from viewport
    int width = renderArgs->_viewport.z;
    int height = renderArgs->_viewport.w;

    // Blit primary to blit FBO
    auto framebufferCache = DependencyManager::get<FramebufferCache>();
    auto primaryFbo = framebufferCache->getPrimaryFramebuffer();

    gpu::doInBatch(renderArgs->_context, [=](gpu::Batch& batch) {
        batch.setFramebuffer(blitFbo);

        if (renderArgs->_renderMode == RenderArgs::MIRROR_RENDER_MODE) {
            if (renderArgs->_context->isStereo()) {
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

void RenderDeferredTask::setToneMappingExposure(float exposure) {
    if (_toneMappingJobIndex >= 0) {
        _jobs[_toneMappingJobIndex].edit<ToneMappingDeferred>()._toneMappingEffect.setExposure(exposure);
    }
}

float RenderDeferredTask::getToneMappingExposure() const {
    if (_toneMappingJobIndex >= 0) {
        return _jobs[_toneMappingJobIndex].get<ToneMappingDeferred>()._toneMappingEffect.getExposure();
    } else {
        return 0.0f; 
    }
}

void RenderDeferredTask::setToneMappingToneCurve(int toneCurve) {
    if (_toneMappingJobIndex >= 0) {
        _jobs[_toneMappingJobIndex].edit<ToneMappingDeferred>()._toneMappingEffect.setToneCurve((ToneMappingEffect::ToneCurve)toneCurve);
    }
}

int RenderDeferredTask::getToneMappingToneCurve() const {
    if (_toneMappingJobIndex >= 0) {
        return _jobs[_toneMappingJobIndex].get<ToneMappingDeferred>()._toneMappingEffect.getToneCurve();
    } else {
        return 0.0f;
    }
}

