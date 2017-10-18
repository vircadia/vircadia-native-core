//
//  OutlineEffect.cpp
//  render-utils/src/
//
//  Created by Olivier Prat on 08/08/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OutlineEffect.h"

#include "GeometryCache.h"

#include <render/FilterTask.h>
#include <render/SortTask.h>

#include "gpu/Context.h"
#include "gpu/StandardShaderLib.h"

#include <sstream>

#include "surfaceGeometry_copyDepth_frag.h"
#include "debug_deferred_buffer_vert.h"
#include "debug_deferred_buffer_frag.h"
#include "Outline_frag.h"
#include "Outline_filled_frag.h"

using namespace render;

OutlineRessources::OutlineRessources() {
}

void OutlineRessources::update(const gpu::FramebufferPointer& primaryFrameBuffer) {
    auto newFrameSize = glm::ivec2(primaryFrameBuffer->getSize());

    // If the depth buffer or size changed, we need to delete our FBOs and recreate them at the
    // new correct dimensions.
    if (_depthFrameBuffer) {
        if (_frameSize != newFrameSize) {
            _frameSize = newFrameSize;
            clear();
        }
    }
    if (!_colorFrameBuffer) {
        if (_frameSize != newFrameSize) {
            _frameSize = newFrameSize;
            // Failing to recreate this frame buffer when the screen has been resized creates a bug on Mac
            _colorFrameBuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("primaryWithoutDepth"));
            _colorFrameBuffer->setRenderBuffer(0, primaryFrameBuffer->getRenderBuffer(0));
        }
    }
}

void OutlineRessources::clear() {
    _depthFrameBuffer.reset();
}

void OutlineRessources::allocate() {
    
    auto width = _frameSize.x;
    auto height = _frameSize.y;
    auto depthFormat = gpu::Element(gpu::SCALAR, gpu::FLOAT, gpu::DEPTH);
    auto depthTexture = gpu::TexturePointer(gpu::Texture::createRenderBuffer(depthFormat, width, height));
    
    _depthFrameBuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("outlineDepth"));
    _depthFrameBuffer->setDepthStencilBuffer(depthTexture, depthFormat);
}

gpu::FramebufferPointer OutlineRessources::getDepthFramebuffer() {
    if (!_depthFrameBuffer) {
        allocate();
    }
    return _depthFrameBuffer;
}

gpu::TexturePointer OutlineRessources::getDepthTexture() {
    return getDepthFramebuffer()->getDepthStencilBuffer();
}

PrepareDrawOutline::PrepareDrawOutline() {
    _ressources = std::make_shared<OutlineRessources>();
}

void PrepareDrawOutline::run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs) {
    auto destinationFrameBuffer = inputs;

    _ressources->update(destinationFrameBuffer);
    outputs = _ressources;
}

void DrawOutlineMask::run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    auto& inShapes = inputs.get0();

    if (!inShapes.empty()) {
        auto ressources = inputs.get1();

        RenderArgs* args = renderContext->args;
        ShapeKey::Builder defaultKeyBuilder;

        outputs = args->_viewport;

        gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
            args->_batch = &batch;

            auto maskPipeline = _shapePlumber->pickPipeline(args, defaultKeyBuilder);
            auto maskSkinnedPipeline = _shapePlumber->pickPipeline(args, defaultKeyBuilder.withSkinned());

            batch.setFramebuffer(ressources->getDepthFramebuffer());
            batch.clearDepthFramebuffer(1.0f);

            // Setup camera, projection and viewport for all items
            batch.setViewportTransform(args->_viewport);
            batch.setStateScissorRect(args->_viewport);

            glm::mat4 projMat;
            Transform viewMat;
            args->getViewFrustum().evalProjectionMatrix(projMat);
            args->getViewFrustum().evalViewTransform(viewMat);

            batch.setProjectionTransform(projMat);
            batch.setViewTransform(viewMat);

            std::vector<ShapeKey> skinnedShapeKeys{};

            // Iterate through all inShapes and render the unskinned
            args->_shapePipeline = maskPipeline;
            batch.setPipeline(maskPipeline->pipeline);
            for (auto items : inShapes) {
                if (items.first.isSkinned()) {
                    skinnedShapeKeys.push_back(items.first);
                } else {
                    renderItems(renderContext, items.second);
                }
            }

            // Reiterate to render the skinned
            args->_shapePipeline = maskSkinnedPipeline;
            batch.setPipeline(maskSkinnedPipeline->pipeline);
            for (const auto& key : skinnedShapeKeys) {
                renderItems(renderContext, inShapes.at(key));
            }

            args->_shapePipeline = nullptr;
            args->_batch = nullptr;
        });
    } else {
        // Outline rect should be null as there are no outlined shapes
        outputs = glm::ivec4(0, 0, 0, 0);
    }
}

gpu::PipelinePointer DrawOutline::_pipeline;
gpu::PipelinePointer DrawOutline::_pipelineFilled;

DrawOutline::DrawOutline() {
}

void DrawOutline::configure(const Config& config) {
    auto& configuration = _configuration.edit();
    const auto OPACITY_EPSILON = 5e-3f;

    configuration._color = config.color;
    configuration._intensity = config.intensity * (config.glow ? 2.f : 1.f);
    configuration._unoccludedFillOpacity = config.unoccludedFillOpacity;
    configuration._occludedFillOpacity = config.occludedFillOpacity;
    configuration._threshold = config.glow ? 1.f : 1e-3f;
    configuration._blurKernelSize = std::min(10, std::max(2, (int)floorf(config.width * 3 + 0.5f)));
    // Size is in normalized screen height. We decide that for outline width = 1, this is equal to 1/400.
    _size = config.width / 400.0f;
    configuration._size.x = (_size * _framebufferSize.y) / _framebufferSize.x;
    configuration._size.y = _size;

    _isFilled = (config.unoccludedFillOpacity > OPACITY_EPSILON || config.occludedFillOpacity > OPACITY_EPSILON);
}

void DrawOutline::run(const render::RenderContextPointer& renderContext, const Inputs& inputs) {
    auto outlineFrameBuffer = inputs.get1();
    auto outlineRect = inputs.get4();

    if (outlineFrameBuffer && outlineRect.z>0 && outlineRect.w>0) {
        auto sceneDepthBuffer = inputs.get2();
        const auto frameTransform = inputs.get0();
        auto outlinedDepthTexture = outlineFrameBuffer->getDepthTexture();
        auto destinationFrameBuffer = inputs.get3();
        auto framebufferSize = glm::ivec2(outlinedDepthTexture->getDimensions());

        if (sceneDepthBuffer) {
            auto pipeline = getPipeline();
            auto args = renderContext->args;

            if (_framebufferSize != framebufferSize)
            {
                auto& configuration = _configuration.edit();
                configuration._size.x = (_size * framebufferSize.y) / framebufferSize.x;
                configuration._size.y = _size;
                _framebufferSize = framebufferSize;
            }

            gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
                batch.enableStereo(false);
                batch.setFramebuffer(destinationFrameBuffer);

                batch.setViewportTransform(args->_viewport);
                batch.setProjectionTransform(glm::mat4());
                batch.resetViewTransform();
                batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(framebufferSize, args->_viewport));
                batch.setPipeline(pipeline);

                batch.setUniformBuffer(OUTLINE_PARAMS_SLOT, _configuration);
                batch.setUniformBuffer(FRAME_TRANSFORM_SLOT, frameTransform->getFrameTransformBuffer());
                batch.setResourceTexture(SCENE_DEPTH_SLOT, sceneDepthBuffer->getPrimaryDepthTexture());
                batch.setResourceTexture(OUTLINED_DEPTH_SLOT, outlinedDepthTexture);
                batch.draw(gpu::TRIANGLE_STRIP, 4);
            });
        }
    }
}

const gpu::PipelinePointer& DrawOutline::getPipeline() {
    if (!_pipeline) {
        auto vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(Outline_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding("outlineParamsBuffer", OUTLINE_PARAMS_SLOT));
        slotBindings.insert(gpu::Shader::Binding("deferredFrameTransformBuffer", FRAME_TRANSFORM_SLOT));
        slotBindings.insert(gpu::Shader::Binding("sceneDepthMap", SCENE_DEPTH_SLOT));
        slotBindings.insert(gpu::Shader::Binding("outlinedDepthMap", OUTLINED_DEPTH_SLOT));
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        state->setDepthTest(gpu::State::DepthTest(false, false));
        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);
        _pipeline = gpu::Pipeline::create(program, state);

        ps = gpu::Shader::createPixel(std::string(Outline_filled_frag));
        program = gpu::Shader::createProgram(vs, ps);
        gpu::Shader::makeProgram(*program, slotBindings);
        _pipelineFilled = gpu::Pipeline::create(program, state);
    }
    return _isFilled ? _pipelineFilled : _pipeline;
}

DebugOutline::DebugOutline() {
    _geometryDepthId = DependencyManager::get<GeometryCache>()->allocateID();
}

DebugOutline::~DebugOutline() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        geometryCache->releaseID(_geometryDepthId);
    }
}

void DebugOutline::configure(const Config& config) {
    _isDisplayEnabled = config.viewMask;
}

void DebugOutline::run(const render::RenderContextPointer& renderContext, const Inputs& input) {
    const auto outlineRessources = input;

    if (_isDisplayEnabled && outlineRessources) {
        assert(renderContext->args);
        assert(renderContext->args->hasViewFrustum());
        RenderArgs* args = renderContext->args;

        gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
            batch.enableStereo(false);
            batch.setViewportTransform(args->_viewport);

            const auto geometryBuffer = DependencyManager::get<GeometryCache>();

            glm::mat4 projMat;
            Transform viewMat;
            args->getViewFrustum().evalProjectionMatrix(projMat);
            args->getViewFrustum().evalViewTransform(viewMat);
            batch.setProjectionTransform(projMat);
            batch.setViewTransform(viewMat, true);
            batch.setModelTransform(Transform());

            const glm::vec4 color(1.0f, 1.0f, 1.0f, 1.0f);

            batch.setPipeline(getDepthPipeline());
            batch.setResourceTexture(0, outlineRessources->getDepthTexture());
            const glm::vec2 bottomLeft(-1.0f, -1.0f);
            const glm::vec2 topRight(1.0f, 1.0f);
            geometryBuffer->renderQuad(batch, bottomLeft, topRight, color, _geometryDepthId);

            batch.setResourceTexture(0, nullptr);
        });
    }
}

void DebugOutline::initializePipelines() {
    static const std::string VERTEX_SHADER{ debug_deferred_buffer_vert };
    static const std::string FRAGMENT_SHADER{ debug_deferred_buffer_frag };
    static const std::string SOURCE_PLACEHOLDER{ "//SOURCE_PLACEHOLDER" };
    static const auto SOURCE_PLACEHOLDER_INDEX = FRAGMENT_SHADER.find(SOURCE_PLACEHOLDER);
    Q_ASSERT_X(SOURCE_PLACEHOLDER_INDEX != std::string::npos, Q_FUNC_INFO,
               "Could not find source placeholder");

    auto state = std::make_shared<gpu::State>();
    state->setDepthTest(gpu::State::DepthTest(false));

    const auto vs = gpu::Shader::createVertex(VERTEX_SHADER);

    // Depth shader
    {
        static const std::string DEPTH_SHADER{
            "vec4 getFragmentColor() {"
            "   float Zdb = texelFetch(depthMap, ivec2(gl_FragCoord.xy), 0).x;"
            "   Zdb = 1.0-(1.0-Zdb)*100;"
            "   return vec4(Zdb, Zdb, Zdb, 1.0); "
            "}"
        };

        auto fragmentShader = FRAGMENT_SHADER;
        fragmentShader.replace(SOURCE_PLACEHOLDER_INDEX, SOURCE_PLACEHOLDER.size(), DEPTH_SHADER);

        const auto ps = gpu::Shader::createPixel(fragmentShader);
        const auto program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding("depthMap", 0));
        gpu::Shader::makeProgram(*program, slotBindings);

        _depthPipeline = gpu::Pipeline::create(program, state);
    }
}

const gpu::PipelinePointer& DebugOutline::getDepthPipeline() {
    if (!_depthPipeline) {
        initializePipelines();
    }

    return _depthPipeline;
}

DrawOutlineTask::DrawOutlineTask() {

}

void DrawOutlineTask::configure(const Config& config) {

}

void DrawOutlineTask::build(JobModel& task, const render::Varying& inputs, render::Varying& outputs) {
    const auto groups = inputs.getN<Inputs>(0).get<Groups>();
    const auto sceneFrameBuffer = inputs.getN<Inputs>(1);
    const auto primaryFramebuffer = inputs.getN<Inputs>(2);
    const auto deferredFrameTransform = inputs.getN<Inputs>(3);

    // Prepare the ShapePipeline
    ShapePlumberPointer shapePlumber = std::make_shared<ShapePlumber>();
    {
        auto state = std::make_shared<gpu::State>();
        state->setDepthTest(true, true, gpu::LESS);
        state->setColorWriteMask(false, false, false, false);
        initMaskPipelines(*shapePlumber, state);
    }

    // Prepare for outline group rendering.
    const auto outlineRessources = task.addJob<PrepareDrawOutline>("PrepareOutline", primaryFramebuffer);

    for (auto i = 0; i < render::Scene::MAX_OUTLINE_COUNT; i++) {
        const auto groupItems = groups[i];
        const auto outlinedItemIDs = task.addJob<render::MetaToSubItems>("OutlineMetaToSubItemIDs", groupItems);
        const auto outlinedItems = task.addJob<render::IDsToBounds>("OutlineMetaToSubItems", outlinedItemIDs, true);

        // Sort
        const auto sortedPipelines = task.addJob<render::PipelineSortShapes>("OutlinePipelineSort", outlinedItems);
        const auto sortedBounds = task.addJob<render::DepthSortShapes>("OutlineDepthSort", sortedPipelines);

        // Draw depth of outlined objects in separate buffer
        std::string name;
        {
            std::ostringstream stream;
            stream << "OutlineMask" << i;
            name = stream.str();
        }
        const auto drawMaskInputs = DrawOutlineMask::Inputs(sortedBounds, outlineRessources).asVarying();
        const auto outlinedRect = task.addJob<DrawOutlineMask>(name, drawMaskInputs, shapePlumber);

        // Draw outline
        {
            std::ostringstream stream;
            stream << "OutlineEffect" << i;
            name = stream.str();
        }
        const auto drawOutlineInputs = DrawOutline::Inputs(deferredFrameTransform, outlineRessources, sceneFrameBuffer, primaryFramebuffer, outlinedRect).asVarying();
        task.addJob<DrawOutline>(name, drawOutlineInputs);
    }

    // Debug outline
    task.addJob<DebugOutline>("OutlineDebug", outlineRessources);
}

#include "model_shadow_vert.h"
#include "skin_model_shadow_vert.h"

#include "model_shadow_frag.h"

void DrawOutlineTask::initMaskPipelines(render::ShapePlumber& shapePlumber, gpu::StatePointer state) {
    auto modelVertex = gpu::Shader::createVertex(std::string(model_shadow_vert));
    auto modelPixel = gpu::Shader::createPixel(std::string(model_shadow_frag));
    gpu::ShaderPointer modelProgram = gpu::Shader::createProgram(modelVertex, modelPixel);
    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withoutSkinned(),
        modelProgram, state);

    auto skinVertex = gpu::Shader::createVertex(std::string(skin_model_shadow_vert));
    gpu::ShaderPointer skinProgram = gpu::Shader::createProgram(skinVertex, modelPixel);
    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withSkinned(),
        skinProgram, state);
}
