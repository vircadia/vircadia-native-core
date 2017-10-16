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


#include "surfaceGeometry_copyDepth_frag.h"
#include "debug_deferred_buffer_vert.h"
#include "debug_deferred_buffer_frag.h"
#include "Outline_frag.h"
#include "Outline_filled_frag.h"

using namespace render;

OutlineRessources::OutlineRessources() {
}

void OutlineRessources::update(const gpu::TexturePointer& colorBuffer) {
    // If the depth buffer or size changed, we need to delete our FBOs and recreate them at the
    // new correct dimensions.
    if (_depthTexture) {
        auto newFrameSize = glm::ivec2(colorBuffer->getDimensions());
        if (_frameSize != newFrameSize) {
            _frameSize = newFrameSize;
            clear();
        }
    }
}

void OutlineRessources::clear() {
    _frameBuffer.reset();
    _depthTexture.reset();
    _idTexture.reset();
}

void OutlineRessources::allocate() {
    
    auto width = _frameSize.x;
    auto height = _frameSize.y;
    auto depthFormat = gpu::Element(gpu::SCALAR, gpu::FLOAT, gpu::DEPTH);

    _idTexture = gpu::TexturePointer(gpu::Texture::createRenderBuffer(gpu::Element::COLOR_RGBA_2, width, height));
    _depthTexture = gpu::TexturePointer(gpu::Texture::createRenderBuffer(depthFormat, width, height));
    
    _frameBuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("outlineDepth"));
    _frameBuffer->setDepthStencilBuffer(_depthTexture, depthFormat);
    _frameBuffer->setRenderBuffer(0, _idTexture);
}

gpu::FramebufferPointer OutlineRessources::getFramebuffer() {
    if (!_frameBuffer) {
        allocate();
    }
    return _frameBuffer;
}

gpu::TexturePointer OutlineRessources::getDepthTexture() {
    if (!_depthTexture) {
        allocate();
    }
    return _depthTexture;
}

gpu::TexturePointer OutlineRessources::getIdTexture() {
    if (!_idTexture) {
        allocate();
    }
    return _idTexture;
}

glm::vec4 encodeIdToColor(unsigned int id) {
    union {
        struct {
            unsigned int r : 2;
            unsigned int g : 2;
            unsigned int b : 2;
            unsigned int a : 2;
        };
        unsigned char id;
    } groupId;

    static_assert(GROUP_ID_COLOR_COMPONENT_BITS == 2, "Assuming two bits per component contrary to GLSL shader code. See Outline_shared.slh");

    assert(id < 254);
    groupId.id = id+1;

    glm::vec4 idColor{ groupId.r, groupId.g, groupId.b, groupId.a };

    // Normalize. Since we put 2 bits into each color component, each component has a maximum
    // value of 3.
    idColor /= GROUP_ID_COLOR_COMPONENT_MAX;
    return idColor;
}

void DrawOutlineMask::run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& output) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    auto& groups = inputs.get0();
    auto& deferredFrameBuffer = inputs.get1();

    RenderArgs* args = renderContext->args;
    ShapeKey::Builder defaultKeyBuilder;
    auto hasOutline = false;

    if (!_outlineRessources) {
        _outlineRessources = std::make_shared<OutlineRessources>();
    }
    _outlineRessources->update(deferredFrameBuffer->getDeferredColorTexture());

    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;

        auto maskPipeline = _shapePlumber->pickPipeline(args, defaultKeyBuilder);
        auto maskSkinnedPipeline = _shapePlumber->pickPipeline(args, defaultKeyBuilder.withSkinned());
        auto colorLoc = maskPipeline.get()->pipeline->getProgram()->getUniforms().findLocation("color");
        auto skinnedColorLoc = maskSkinnedPipeline.get()->pipeline->getProgram()->getUniforms().findLocation("color");
        unsigned int groupId = 0;

        for (auto& inShapeBounds : groups) {
            if (!inShapeBounds.isNull()) {
                auto& inShapes = inShapeBounds.get<render::ShapeBounds>();

                if (!inShapes.empty()) {
                    if (!hasOutline) {
                        batch.setFramebuffer(_outlineRessources->getFramebuffer());
                        // Clear it only if it hasn't been done before
                        batch.clearFramebuffer(
                            gpu::Framebuffer::BUFFER_COLOR0 | gpu::Framebuffer::BUFFER_DEPTH,
                            vec4(0.0f, 0.0f, 0.0f, 0.0f), 1.0f, 0, false);

                        // Setup camera, projection and viewport for all items
                        batch.setViewportTransform(args->_viewport);
                        batch.setStateScissorRect(args->_viewport);

                        glm::mat4 projMat;
                        Transform viewMat;
                        args->getViewFrustum().evalProjectionMatrix(projMat);
                        args->getViewFrustum().evalViewTransform(viewMat);

                        batch.setProjectionTransform(projMat);
                        batch.setViewTransform(viewMat);
                        hasOutline = true;
                    }

                    std::vector<ShapeKey> skinnedShapeKeys{};
                    // Encode group id in quantized color
                    glm::vec4 idColor = encodeIdToColor(groupId);

                    // Iterate through all inShapes and render the unskinned
                    args->_shapePipeline = maskPipeline;
                    batch.setPipeline(maskPipeline->pipeline);
                    batch._glUniform4f(colorLoc, idColor.r, idColor.g, idColor.b, idColor.a);
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
                    batch._glUniform4f(skinnedColorLoc, idColor.r, idColor.g, idColor.b, idColor.a);
                    for (const auto& key : skinnedShapeKeys) {
                        renderItems(renderContext, inShapes.at(key));
                    }
                }
            }

            ++groupId;
        }

        args->_shapePipeline = nullptr;
        args->_batch = nullptr;
    });

    if (hasOutline) {
        output = _outlineRessources;
    } else {
        output = nullptr;
    }
}

PrepareDrawOutline::PrepareDrawOutline() {

}

void PrepareDrawOutline::run(const render::RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs) {
    auto destinationFrameBuffer = inputs;
    auto framebufferSize = destinationFrameBuffer->getSize();

    if (!_primaryWithoutDepthBuffer || framebufferSize != _frameBufferSize) {
        // Failing to recreate this frame buffer when the screen has been resized creates a bug on Mac
        _primaryWithoutDepthBuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("primaryWithoutDepth"));
        _primaryWithoutDepthBuffer->setRenderBuffer(0, destinationFrameBuffer->getRenderBuffer(0));
        _frameBufferSize = framebufferSize;
    }

    outputs = _primaryWithoutDepthBuffer;
}

gpu::PipelinePointer DrawOutline::_pipeline;
gpu::PipelinePointer DrawOutline::_pipelineFilled;

DrawOutline::DrawOutline() {
}

void DrawOutline::configure(const Config& config) {
    _color = config.color;
    _blurKernelSize = std::min(10, std::max(2, (int)floorf(config.width*2 + 0.5f)));
    // Size is in normalized screen height. We decide that for outline width = 1, this is equal to 1/400.
    _size = config.width / 400.f;
    _fillOpacityUnoccluded = config.fillOpacityUnoccluded;
    _fillOpacityOccluded = config.fillOpacityOccluded;
    _threshold = config.glow ? 1.f : 1e-3f;
    _intensity = config.intensity * (config.glow ? 2.f : 1.f);
    _hasConfigurationChanged = true;
}

void DrawOutline::run(const render::RenderContextPointer& renderContext, const Inputs& inputs) {
    auto outlineFrameBuffer = inputs.get1();

    if (outlineFrameBuffer) {
        auto sceneDepthBuffer = inputs.get2();
        const auto frameTransform = inputs.get0();
        auto outlinedDepthTexture = outlineFrameBuffer->getDepthTexture();
        auto outlinedIdTexture = outlineFrameBuffer->getIdTexture();
        auto destinationFrameBuffer = inputs.get3();
        auto framebufferSize = glm::ivec2(outlinedDepthTexture->getDimensions());

        if (sceneDepthBuffer) {
            const auto OPACITY_EPSILON = 5e-3f;
            auto pipeline = getPipeline(_fillOpacityUnoccluded>OPACITY_EPSILON || _fillOpacityOccluded>OPACITY_EPSILON);
            auto args = renderContext->args;

            if (_hasConfigurationChanged)
            {
                auto& configuration = _configuration.edit();

                for (auto groupId = 0; groupId < MAX_GROUP_COUNT; groupId++) {
                    auto& groupConfig = configuration._groups[groupId];

                    groupConfig._color = _color;
                    groupConfig._intensity = _intensity;
                    groupConfig._fillOpacityUnoccluded = _fillOpacityUnoccluded;
                    groupConfig._fillOpacityOccluded = _fillOpacityOccluded;
                    groupConfig._threshold = _threshold;
                    groupConfig._blurKernelSize = _blurKernelSize;
                    groupConfig._size.x = (_size * framebufferSize.y) / framebufferSize.x;
                    groupConfig._size.y = _size;
                }
                _hasConfigurationChanged = false;
            }

            gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
                batch.enableStereo(false);
                batch.setFramebuffer(destinationFrameBuffer);

                batch.setViewportTransform(args->_viewport);
                batch.setProjectionTransform(glm::mat4());
                batch.resetViewTransform();
                batch.setModelTransform(gpu::Framebuffer::evalSubregionTexcoordTransform(framebufferSize, args->_viewport));
                batch.setPipeline(pipeline);

                auto enabledGroupsLoc = pipeline->getProgram()->getUniforms().findLocation("enabledGroupsMask");

                batch.setUniformBuffer(OUTLINE_PARAMS_SLOT, _configuration);
                batch.setUniformBuffer(FRAME_TRANSFORM_SLOT, frameTransform->getFrameTransformBuffer());
                batch.setResourceTexture(SCENE_DEPTH_SLOT, sceneDepthBuffer->getPrimaryDepthTexture());
                batch.setResourceTexture(OUTLINED_DEPTH_SLOT, outlinedDepthTexture);
                batch.setResourceTexture(OUTLINED_ID_SLOT, outlinedIdTexture);
                batch._glUniform1i(enabledGroupsLoc, 1);
                batch.draw(gpu::TRIANGLE_STRIP, 4);
            });
        }
    }
}

const gpu::PipelinePointer& DrawOutline::getPipeline(bool isFilled) {
    if (!_pipeline) {
        auto vs = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        auto ps = gpu::Shader::createPixel(std::string(Outline_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding("outlineParamsBuffer", OUTLINE_PARAMS_SLOT));
        slotBindings.insert(gpu::Shader::Binding("deferredFrameTransformBuffer", FRAME_TRANSFORM_SLOT));
        slotBindings.insert(gpu::Shader::Binding("sceneDepthMap", SCENE_DEPTH_SLOT));
        slotBindings.insert(gpu::Shader::Binding("outlinedDepthMap", OUTLINED_DEPTH_SLOT));
        slotBindings.insert(gpu::Shader::Binding("outlinedIdMap", OUTLINED_ID_SLOT));
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
    return isFilled ? _pipelineFilled : _pipeline;
}

DebugOutline::DebugOutline() {
    _geometryDepthId = DependencyManager::get<GeometryCache>()->allocateID();
    _geometryColorId = DependencyManager::get<GeometryCache>()->allocateID();
}

DebugOutline::~DebugOutline() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        geometryCache->releaseID(_geometryDepthId);
        geometryCache->releaseID(_geometryColorId);
    }
}

void DebugOutline::configure(const Config& config) {
    _isDisplayEnabled = config.viewMask;
}

void DebugOutline::run(const render::RenderContextPointer& renderContext, const Inputs& input) {
    const auto outlineFramebuffer = input;

    if (_isDisplayEnabled && outlineFramebuffer) {
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
            batch.setResourceTexture(0, outlineFramebuffer->getDepthTexture());
            {
                const glm::vec2 bottomLeft(-1.0f, -1.0f);
                const glm::vec2 topRight(0.0f, 1.0f);
                geometryBuffer->renderQuad(batch, bottomLeft, topRight, color, _geometryDepthId);
            }

            batch.setPipeline(getIdPipeline());
            batch.setResourceTexture(0, outlineFramebuffer->getIdTexture());
            {
                const glm::vec2 bottomLeft(0.0f, -1.0f);
                const glm::vec2 topRight(1.0f, 1.0f);
                geometryBuffer->renderQuad(batch, bottomLeft, topRight, color, _geometryColorId);
            }

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

    // ID shader
    {
        static const std::string ID_SHADER{
            "vec4 getFragmentColor() {"
            "   return texelFetch(albedoMap, ivec2(gl_FragCoord.xy), 0); "
            "}"
        };

        auto fragmentShader = FRAGMENT_SHADER;
        fragmentShader.replace(SOURCE_PLACEHOLDER_INDEX, SOURCE_PLACEHOLDER.size(), ID_SHADER);

        const auto ps = gpu::Shader::createPixel(fragmentShader);
        const auto program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding("albedoMap", 0));
        gpu::Shader::makeProgram(*program, slotBindings);

        _idPipeline = gpu::Pipeline::create(program, state);
    }
}

const gpu::PipelinePointer& DebugOutline::getDepthPipeline() {
    if (!_depthPipeline) {
        initializePipelines();
    }

    return _depthPipeline;
}

const gpu::PipelinePointer& DebugOutline::getIdPipeline() {
    if (!_idPipeline) {
        initializePipelines();
    }

    return _idPipeline;
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
        state->setDepthTest(true, true, gpu::LESS_EQUAL);
        state->setColorWriteMask(true, true, true, true);

        initMaskPipelines(*shapePlumber, state);
    }

    DrawOutlineMask::Groups sortedBounds;
    for (auto i = 0; i < DrawOutline::MAX_GROUP_COUNT; i++) {
        const auto groupItems = groups[i];
        const auto outlinedItemIDs = task.addJob<render::MetaToSubItems>("OutlineMetaToSubItemIDs", groupItems);
        const auto outlinedItems = task.addJob<render::IDsToBounds>("OutlineMetaToSubItems", outlinedItemIDs, true);

        // Sort
        const auto sortedPipelines = task.addJob<render::PipelineSortShapes>("OutlinePipelineSort", outlinedItems);
        sortedBounds[i] = task.addJob<render::DepthSortShapes>("OutlineDepthSort", sortedPipelines);
    }

    // Draw depth of outlined objects in separate buffer
    const auto drawMaskInputs = DrawOutlineMask::Inputs(sortedBounds, sceneFrameBuffer).asVarying();
    const auto outlinedFrameBuffer = task.addJob<DrawOutlineMask>("OutlineMask", drawMaskInputs, shapePlumber);

    // Prepare for outline group rendering.
    const auto destFrameBuffer = task.addJob<PrepareDrawOutline>("PrepareOutline", primaryFramebuffer);

    // Draw outline
    const auto drawOutlineInputs = DrawOutline::Inputs(deferredFrameTransform, outlinedFrameBuffer, sceneFrameBuffer, destFrameBuffer).asVarying();
    task.addJob<DrawOutline>("OutlineEffect", drawOutlineInputs);

    // Debug outline
    task.addJob<DebugOutline>("OutlineDebug", outlinedFrameBuffer);
}

#include "model_shadow_vert.h"
#include "model_shadow_fade_vert.h"
#include "skin_model_shadow_vert.h"
#include "skin_model_shadow_fade_vert.h"

#include "model_outline_frag.h"
#include "model_outline_fade_frag.h"

void DrawOutlineTask::initMaskPipelines(render::ShapePlumber& shapePlumber, gpu::StatePointer state) {
    auto modelVertex = gpu::Shader::createVertex(std::string(model_shadow_vert));
    auto modelPixel = gpu::Shader::createPixel(std::string(model_outline_frag));
    gpu::ShaderPointer modelProgram = gpu::Shader::createProgram(modelVertex, modelPixel);
    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withoutSkinned().withoutFade(),
        modelProgram, state);

    auto skinVertex = gpu::Shader::createVertex(std::string(skin_model_shadow_vert));
    gpu::ShaderPointer skinProgram = gpu::Shader::createProgram(skinVertex, modelPixel);
    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withSkinned().withoutFade(),
        skinProgram, state);

    auto modelFadeVertex = gpu::Shader::createVertex(std::string(model_shadow_fade_vert));
    auto modelFadePixel = gpu::Shader::createPixel(std::string(model_outline_fade_frag));
    gpu::ShaderPointer modelFadeProgram = gpu::Shader::createProgram(modelFadeVertex, modelFadePixel);
    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withoutSkinned().withFade(),
        modelFadeProgram, state);

    auto skinFadeVertex = gpu::Shader::createVertex(std::string(skin_model_shadow_fade_vert));
    gpu::ShaderPointer skinFadeProgram = gpu::Shader::createProgram(skinFadeVertex, modelFadePixel);
    shapePlumber.addPipeline(
        ShapeKey::Filter::Builder().withSkinned().withFade(),
        skinFadeProgram, state);
}
