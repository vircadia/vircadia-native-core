//
//  Created by Bradley Austin Davis on 2016/02/15
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HmdDisplayPlugin.h"

#include <memory>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/intersect.hpp>

#include <QtCore/QLoggingCategory>
#include <QtCore/QFileInfo>
#include <QtCore/QDateTime>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

#include <GLMHelpers.h>
#include <ui-plugins/PluginContainer.h>
#include <CursorManager.h>
#include <gl/GLWidget.h>
#include <shared/NsightHelpers.h>
#include <gpu/Context.h>
#include <gpu/gl/GLBackend.h>
#include <shaders/Shaders.h>

#include <TextureCache.h>
#include <PathUtils.h>

#include "../Logging.h"
#include "../CompositorHelper.h"

#include "DesktopPreviewProvider.h"

static const QString MONO_PREVIEW = "Mono Preview";
static const QString DISABLE_PREVIEW = "Disable Preview";
static const QString FRAMERATE = DisplayPlugin::MENU_PATH() + ">Framerate";
static const QString DEVELOPER_MENU_PATH = "Developer>" + DisplayPlugin::MENU_PATH();
static const bool DEFAULT_MONO_VIEW = true;
#if !defined(Q_OS_MAC)
static const bool DEFAULT_DISABLE_PREVIEW = false;
#endif
static const glm::mat4 IDENTITY_MATRIX;

//#define LIVE_SHADER_RELOAD 1
extern glm::vec3 getPoint(float yaw, float pitch);

glm::uvec2 HmdDisplayPlugin::getRecommendedUiSize() const {
    return CompositorHelper::VIRTUAL_SCREEN_SIZE;
}

QRect HmdDisplayPlugin::getRecommendedHUDRect() const {
    return CompositorHelper::VIRTUAL_SCREEN_RECOMMENDED_OVERLAY_RECT;
}

glm::mat4 HmdDisplayPlugin::getEyeToHeadTransform(Eye eye) const { 
    return _eyeOffsets[eye]; 
}

glm::mat4 HmdDisplayPlugin::getEyeProjection(Eye eye, const glm::mat4& baseProjection) const { 
    return _eyeProjections[eye]; 
}

glm::mat4 HmdDisplayPlugin::getCullingProjection(const glm::mat4& baseProjection) const { 
    return _cullingProjection; 
}

glm::ivec4 HmdDisplayPlugin::eyeViewport(Eye eye) const {
    uvec2 vpSize = getRecommendedRenderSize();
    vpSize.x /= 2;
    uvec2 vpPos;
    if (eye == Eye::Right) {
        vpPos.x = vpSize.x;
    }
    return ivec4(vpPos, vpSize);
}

#define DISABLE_PREVIEW_MENU_ITEM_DELAY_MS 500

bool HmdDisplayPlugin::internalActivate() {
    _monoPreview = _container->getBoolSetting("monoPreview", DEFAULT_MONO_VIEW);
    _clearPreviewFlag = true;
    _container->addMenuItem(PluginType::DISPLAY_PLUGIN, MENU_PATH(), MONO_PREVIEW,
        [this](bool clicked) {
        _monoPreview = clicked;
        _container->setBoolSetting("monoPreview", _monoPreview);
    }, true, _monoPreview);

#if defined(Q_OS_MAC)
    _disablePreview = true;
#else
    _disablePreview = _container->getBoolSetting("disableHmdPreview", DEFAULT_DISABLE_PREVIEW || _vsyncEnabled);
#endif

    QTimer::singleShot(DISABLE_PREVIEW_MENU_ITEM_DELAY_MS, [this] {
        if (isActive() && !_vsyncEnabled) {
            _container->addMenuItem(PluginType::DISPLAY_PLUGIN, MENU_PATH(), DISABLE_PREVIEW,
                [this](bool clicked) {
                _disablePreview = clicked;
                _container->setBoolSetting("disableHmdPreview", _disablePreview);
                if (_disablePreview) {
                    _clearPreviewFlag = true;
                }
            }, true, _disablePreview);
        }
    });

    _container->removeMenu(FRAMERATE);
    for_each_eye([&](Eye eye) {
        _eyeInverseProjections[eye] = glm::inverse(_eyeProjections[eye]);
    });

    _clearPreviewFlag = true;

    return Parent::internalActivate();
}

void HmdDisplayPlugin::internalDeactivate() {
    Parent::internalDeactivate();
}

void HmdDisplayPlugin::customizeContext() {

    VisionSqueezeParameters parameters;
    _visionSqueezeParametersBuffer =
        gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(VisionSqueezeParameters), (const gpu::Byte*) &parameters));

    Parent::customizeContext();
    _hudRenderer.build();
}

void HmdDisplayPlugin::uncustomizeContext() {
    // This stops the weirdness where if the preview was disabled, on switching back to 2D,
    // the vsync was stuck in the disabled state.  No idea why that happens though.
    _disablePreview = false;
    render([&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.resetViewTransform();
        batch.setFramebuffer(_compositeFramebuffer);
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, vec4(0));
    });
    _hudRenderer = HUDRenderer();
    _previewTexture.reset();
    Parent::uncustomizeContext();
}

ivec4 HmdDisplayPlugin::getViewportForSourceSize(const uvec2& size) const {
    // screen preview mirroring
    auto window = _container->getPrimaryWidget();
    auto devicePixelRatio = window->devicePixelRatio();
    auto windowSize = toGlm(window->size());
    windowSize *= devicePixelRatio;
    float windowAspect = aspect(windowSize);
    float sceneAspect = aspect(size);
    float aspectRatio = sceneAspect / windowAspect;

    uvec2 targetViewportSize = windowSize;
    if (aspectRatio < 1.0f) {
        targetViewportSize.x *= aspectRatio;
    } else {
        targetViewportSize.y /= aspectRatio;
    }
    uvec2 targetViewportPosition;
    if (targetViewportSize.x < windowSize.x) {
        targetViewportPosition.x = (windowSize.x - targetViewportSize.x) / 2;
    } else if (targetViewportSize.y < windowSize.y) {
        targetViewportPosition.y = (windowSize.y - targetViewportSize.y) / 2;
    }
    return ivec4(targetViewportPosition, targetViewportSize);
}

float HmdDisplayPlugin::getLeftCenterPixel() const {
    glm::mat4 eyeProjection = _eyeProjections[Left];
    glm::mat4 inverseEyeProjection = glm::inverse(eyeProjection);
    vec2 eyeRenderTargetSize = { _renderTargetSize.x / 2, _renderTargetSize.y };

    vec4 left = vec4(-1, 0, -1, 1);
    vec4 right = vec4(1, 0, -1, 1);
    vec4 right2 = inverseEyeProjection * right;
    vec4 left2 = inverseEyeProjection * left;
    left2 /= left2.w;
    right2 /= right2.w;
    float width = -left2.x + right2.x;
    float leftBias = -left2.x / width;
    float leftCenterPixel = eyeRenderTargetSize.x * leftBias;
    return leftCenterPixel;
}

void HmdDisplayPlugin::internalPresent() {
    PROFILE_RANGE_EX(render, __FUNCTION__, 0xff00ff00, (uint64_t)presentCount())

    // Composite together the scene, hud and mouse cursor
    hmdPresent();

    if (_displayTexture) {
        // Note: _displayTexture must currently be the same size as the display.
        uvec2 dims = uvec2(_displayTexture->getDimensions());
        auto viewport = ivec4(uvec2(0), dims);
        render([&](gpu::Batch& batch) {
            renderFromTexture(batch, _displayTexture, viewport, viewport);
        });
        swapBuffers();
    } else if (!_disablePreview) {
        // screen preview mirroring
        auto sourceSize = _renderTargetSize;
        if (_monoPreview) {
            sourceSize.x >>= 1;
        }

        float shiftLeftBy = getLeftCenterPixel() - (sourceSize.x / 2);
        float newWidth = sourceSize.x - shiftLeftBy;

        // Experimentally adjusted the region presented in preview to avoid seeing the masked pixels and recenter the center...
        static float SCALE_WIDTH = 0.8f;
        static float SCALE_OFFSET = 2.0f;
        newWidth *= SCALE_WIDTH;
        shiftLeftBy *= SCALE_OFFSET;

        const unsigned int RATIO_Y = 9;
        const unsigned int RATIO_X = 16;
        glm::uvec2 originalClippedSize { newWidth, newWidth * RATIO_Y / RATIO_X };

        glm::ivec4 viewport = getViewportForSourceSize(sourceSize);
        glm::ivec4 scissor = viewport;

        auto fbo = gpu::FramebufferPointer();

        render([&](gpu::Batch& batch) {

            if (_monoPreview) {
                auto window = _container->getPrimaryWidget();
                float devicePixelRatio = window->devicePixelRatio();
                glm::vec2 windowSize = toGlm(window->size());
                windowSize *= devicePixelRatio;

                float windowAspect = aspect(windowSize);  // example: 1920 x 1080 = 1.78
                float sceneAspect = aspect(originalClippedSize); // usually: 1512 x 850 = 1.78


                bool scaleToWidth = windowAspect < sceneAspect;

                float ratio;
                int scissorOffset;

                if (scaleToWidth) {
                    ratio = (float)windowSize.x / (float)newWidth;
                } else {
                    ratio = (float)windowSize.y / (float)originalClippedSize.y;
                }

                float scaledShiftLeftBy = shiftLeftBy * ratio;

                int scissorSizeX = originalClippedSize.x * ratio;
                int scissorSizeY = originalClippedSize.y * ratio;

                int viewportSizeX = sourceSize.x * ratio;
                int viewportSizeY = sourceSize.y * ratio;
                int viewportOffset = ((int)windowSize.y - viewportSizeY) / 2;

                if (scaleToWidth) {
                    scissorOffset = ((int)windowSize.y - scissorSizeY) / 2;
                    scissor = ivec4(0, scissorOffset, scissorSizeX, scissorSizeY);
                    viewport = ivec4(-scaledShiftLeftBy, viewportOffset, viewportSizeX, viewportSizeY);
                } else {
                    scissorOffset = ((int)windowSize.x - scissorSizeX) / 2;
                    scissor = ivec4(scissorOffset, 0, scissorSizeX, scissorSizeY);
                    viewport = ivec4(scissorOffset - scaledShiftLeftBy, viewportOffset, viewportSizeX, viewportSizeY);
                }

                // TODO: only bother getting and passing in the hmdPreviewFramebuffer if the camera is on
                fbo = DependencyManager::get<TextureCache>()->getHmdPreviewFramebuffer(windowSize.x, windowSize.y);

                viewport.z *= 2;
            }
            renderFromTexture(batch, _compositeFramebuffer->getRenderBuffer(0), viewport, scissor, fbo);
        });
        swapBuffers();

    } else if (_clearPreviewFlag) {

        QImage image = DesktopPreviewProvider::getInstance()->getPreviewDisabledImage(_vsyncEnabled);
        _previewTexture = gpu::Texture::createStrict(
                gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA),
                image.width(), image.height(),
                gpu::Texture::MAX_NUM_MIPS,
                gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR));
            _previewTexture->setSource("HMD Preview Texture");
            _previewTexture->setUsage(gpu::Texture::Usage::Builder().withColor().build());
            _previewTexture->setStoredMipFormat(gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA));
            _previewTexture->assignStoredMip(0, image.sizeInBytes(), image.constBits());
            _previewTexture->setAutoGenerateMips(true);

        auto viewport = getViewportForSourceSize(uvec2(_previewTexture->getDimensions()));

        render([&](gpu::Batch& batch) {
            renderFromTexture(batch, _previewTexture, viewport, viewport);
        });
        _clearPreviewFlag = false;
        swapBuffers();
    }
    postPreview();

    // If preview is disabled, we need to check to see if the window size has changed
    // and re-render the no-preview message
    if (_disablePreview) {
        auto window = _container->getPrimaryWidget();
        glm::vec2 windowSize = toGlm(window->size());
        if (windowSize != _lastWindowSize) {
            _clearPreviewFlag = true;
            _lastWindowSize = windowSize;
        }
    }
}

// HMD specific stuff

glm::mat4 HmdDisplayPlugin::getHeadPose() const {
    return _currentRenderFrameInfo.renderPose;
}

void HmdDisplayPlugin::updatePresentPose() {
    // By default assume we'll present with the same pose as the render
    _currentPresentFrameInfo.presentPose = _currentPresentFrameInfo.renderPose;
}

void HmdDisplayPlugin::updateFrameData() {
    // Check if we have old frame data to discard
    static const uint32_t INVALID_FRAME = (uint32_t)(~0);
    uint32_t oldFrameIndex = _currentFrame ? _currentFrame->frameIndex : INVALID_FRAME;

    Parent::updateFrameData();
    uint32_t newFrameIndex = _currentFrame ? _currentFrame->frameIndex : INVALID_FRAME;

    if (oldFrameIndex != newFrameIndex) {
        withPresentThreadLock([&] {
            if (oldFrameIndex != INVALID_FRAME) {
                auto itr = _frameInfos.find(oldFrameIndex);
                if (itr != _frameInfos.end()) {
                    _frameInfos.erase(itr);
                }
            }
            if (newFrameIndex != INVALID_FRAME) {
                _currentPresentFrameInfo = _frameInfos[newFrameIndex];
            }
        });
    }

    updatePresentPose();
}

glm::mat4 HmdDisplayPlugin::getViewCorrection() {
    if (_currentFrame) {
        auto batchPose = _currentFrame->pose;
        return glm::inverse(_currentPresentFrameInfo.presentPose) * batchPose;
    } else {
        return glm::mat4();
    }
}

void HmdDisplayPlugin::HUDRenderer::build() {
    vertices = std::make_shared<gpu::Buffer>();
    indices = std::make_shared<gpu::Buffer>();

    //UV mapping source: http://www.mvps.org/directx/articles/spheremap.htm

    static const float fov = CompositorHelper::VIRTUAL_UI_TARGET_FOV.y;
    static const float aspectRatio = CompositorHelper::VIRTUAL_UI_ASPECT_RATIO;
    static const uint16_t stacks = 128;
    static const uint16_t slices = 64;

    Vertex vertex;

    // Compute vertices positions and texture UV coordinate
    // Create and write to buffer
    for (int i = 0; i < stacks; i++) {
        vertex.uv.y = (float)i / (float)(stacks - 1); // First stack is 0.0f, last stack is 1.0f
        // abs(theta) <= fov / 2.0f
        float pitch = -fov * (vertex.uv.y - 0.5f);
        for (int j = 0; j < slices; j++) {
            vertex.uv.x = (float)j / (float)(slices - 1); // First slice is 0.0f, last slice is 1.0f
            // abs(phi) <= fov * aspectRatio / 2.0f
            float yaw = -fov * aspectRatio * (vertex.uv.x - 0.5f);
            vertex.pos = getPoint(yaw, pitch);
            vertices->append(sizeof(Vertex), (gpu::Byte*)&vertex);
        }
    }

    // Compute number of indices needed
    static const int VERTEX_PER_TRANGLE = 3;
    static const int TRIANGLE_PER_RECTANGLE = 2;
    int numberOfRectangles = (slices - 1) * (stacks - 1);
    indexCount = numberOfRectangles * TRIANGLE_PER_RECTANGLE * VERTEX_PER_TRANGLE;

    // Compute indices order
    std::vector<GLushort> indices;
    for (int i = 0; i < stacks - 1; i++) {
        for (int j = 0; j < slices - 1; j++) {
            GLushort bottomLeftIndex = i * slices + j;
            GLushort bottomRightIndex = bottomLeftIndex + 1;
            GLushort topLeftIndex = bottomLeftIndex + slices;
            GLushort topRightIndex = topLeftIndex + 1;
            // FIXME make a z-order curve for better vertex cache locality
            indices.push_back(topLeftIndex);
            indices.push_back(bottomLeftIndex);
            indices.push_back(topRightIndex);

            indices.push_back(topRightIndex);
            indices.push_back(bottomLeftIndex);
            indices.push_back(bottomRightIndex);
        }
    }
    this->indices->append(indices);
    format = std::make_shared<gpu::Stream::Format>(); // 1 for everyone
    format->setAttribute(gpu::Stream::POSITION, gpu::Stream::POSITION, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ), 0);
    format->setAttribute(gpu::Stream::TEXCOORD, gpu::Stream::TEXCOORD, gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::UV));
    uniformsBuffer = std::make_shared<gpu::Buffer>(sizeof(Uniforms), nullptr);

    auto program = gpu::Shader::createProgram(shader::render_utils::program::hmd_ui);
    gpu::StatePointer state = std::make_shared<gpu::State>();
    state->setDepthTest(gpu::State::DepthTest(true, true, gpu::LESS_EQUAL));
    state->setBlendFunction(true,
                            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
                            gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);

    pipeline = gpu::Pipeline::create(program, state);
}

std::function<void(gpu::Batch&, const gpu::TexturePointer&)> HmdDisplayPlugin::HUDRenderer::render() {
    auto hudPipeline = pipeline;
    auto hudFormat = format;
    auto hudVertices = vertices;
    auto hudIndices = indices;
    auto hudUniformBuffer = uniformsBuffer;
    auto hudUniforms = uniforms;
    auto hudIndexCount = indexCount;
    return [=](gpu::Batch& batch, const gpu::TexturePointer& hudTexture) {
        if (hudPipeline && hudTexture) {
            batch.setPipeline(hudPipeline);

            batch.setInputFormat(hudFormat);
            gpu::BufferView posView(hudVertices, VERTEX_OFFSET, hudVertices->getSize(), VERTEX_STRIDE, hudFormat->getAttributes().at(gpu::Stream::POSITION)._element);
            gpu::BufferView uvView(hudVertices, TEXTURE_OFFSET, hudVertices->getSize(), VERTEX_STRIDE, hudFormat->getAttributes().at(gpu::Stream::TEXCOORD)._element);
            batch.setInputBuffer(gpu::Stream::POSITION, posView);
            batch.setInputBuffer(gpu::Stream::TEXCOORD, uvView);
            batch.setIndexBuffer(gpu::UINT16, hudIndices, 0);
            hudUniformBuffer->setSubData(0, hudUniforms);
            batch.setUniformBuffer(0, hudUniformBuffer);

            auto compositorHelper = DependencyManager::get<CompositorHelper>();
            glm::mat4 modelTransform = compositorHelper->getUiTransform();
            batch.setModelTransform(modelTransform);
            batch.setResourceTexture(0, hudTexture);

            batch.drawIndexed(gpu::TRIANGLES, hudIndexCount);
        }
    };
}

void HmdDisplayPlugin::compositePointer() {
    auto& cursorManager = Cursor::Manager::instance();
    const auto& cursorData = _cursorsData[cursorManager.getCursor()->getIcon()];
    auto compositorHelper = DependencyManager::get<CompositorHelper>();
    // Reconstruct the headpose from the eye poses
    auto headPosition = vec3(_currentPresentFrameInfo.presentPose[3]);
    render([&](gpu::Batch& batch) {
        // FIXME use standard gpu stereo rendering for this.
        batch.enableStereo(false);
        batch.setFramebuffer(_compositeFramebuffer);
        batch.setPipeline(_cursorPipeline);
        batch.setResourceTexture(0, cursorData.texture);
        batch.resetViewTransform();
        for_each_eye([&](Eye eye) {
            batch.setViewportTransform(eyeViewport(eye));
            batch.setProjectionTransform(_eyeProjections[eye]);
            auto eyePose = _currentPresentFrameInfo.presentPose * getEyeToHeadTransform(eye);
            auto reticleTransform = compositorHelper->getReticleTransform(eyePose, headPosition);
            batch.setModelTransform(reticleTransform);
            batch.draw(gpu::TRIANGLE_STRIP, 4);
        });
    });
}

std::function<void(gpu::Batch&, const gpu::TexturePointer&)> HmdDisplayPlugin::getHUDOperator() {
    return _hudRenderer.render();
}

HmdDisplayPlugin::~HmdDisplayPlugin() {
}

float HmdDisplayPlugin::stutterRate() const {
    return _stutterRate.rate();
}

float adjustVisionSqueezeRatioForDevice(float visionSqueezeRatio, float visionSqueezeDeviceLow, float visionSqueezeDeviceHigh) {
    if (visionSqueezeRatio <= 0.0f) {
        return 0.0f;
    }

    float deviceRange = visionSqueezeDeviceHigh - visionSqueezeDeviceLow;
    const float SQUEEZE_ADJUSTMENT = 0.75f; // magic number picked through experimentation
    return deviceRange * (SQUEEZE_ADJUSTMENT * visionSqueezeRatio) + visionSqueezeDeviceLow;
}

void HmdDisplayPlugin::updateVisionSqueezeParameters(float visionSqueezeX, float visionSqueezeY,
                                                     float visionSqueezeTransition,
                                                     int visionSqueezePerEye, float visionSqueezeGroundPlaneY,
                                                     float visionSqueezeSpotlightSize) {

    visionSqueezeX = adjustVisionSqueezeRatioForDevice(visionSqueezeX, _visionSqueezeDeviceLowX, _visionSqueezeDeviceHighX);
    visionSqueezeY = adjustVisionSqueezeRatioForDevice(visionSqueezeY, _visionSqueezeDeviceLowY, _visionSqueezeDeviceHighY);

    auto& params = _visionSqueezeParametersBuffer.get<VisionSqueezeParameters>();
    if (params._visionSqueezeX != visionSqueezeX) {
        _visionSqueezeParametersBuffer.edit<VisionSqueezeParameters>()._visionSqueezeX = visionSqueezeX;
    }
    if (params._visionSqueezeY != visionSqueezeY) {
        _visionSqueezeParametersBuffer.edit<VisionSqueezeParameters>()._visionSqueezeY = visionSqueezeY;
    }
    if (params._visionSqueezeTransition != visionSqueezeTransition) {
        _visionSqueezeParametersBuffer.edit<VisionSqueezeParameters>()._visionSqueezeTransition = visionSqueezeTransition;
    }
    if (params._visionSqueezePerEye != visionSqueezePerEye) {
        _visionSqueezeParametersBuffer.edit<VisionSqueezeParameters>()._visionSqueezePerEye = visionSqueezePerEye;
    }
    if (params._visionSqueezeGroundPlaneY != visionSqueezeGroundPlaneY) {
        _visionSqueezeParametersBuffer.edit<VisionSqueezeParameters>()._visionSqueezeGroundPlaneY = visionSqueezeGroundPlaneY;
    }
    if (params._visionSqueezeSpotlightSize != visionSqueezeSpotlightSize) {
        _visionSqueezeParametersBuffer.edit<VisionSqueezeParameters>()._visionSqueezeSpotlightSize = visionSqueezeSpotlightSize;
    }
}

void HmdDisplayPlugin::setupCompositeScenePipeline(gpu::Batch& batch) {
    batch.setPipeline(_drawTextureSqueezePipeline);
    _visionSqueezeParametersBuffer.edit<VisionSqueezeParameters>()._hmdSensorMatrix = _currentPresentFrameInfo.presentPose;
    batch.setUniformBuffer(drawTextureWithVisionSqueezeParamsSlot, _visionSqueezeParametersBuffer);
}
