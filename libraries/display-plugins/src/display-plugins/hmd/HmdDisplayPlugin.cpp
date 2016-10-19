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
#include <GeometryCache.h>
#include <gpu/Context.h>
#include <gpu/StandardShaderLib.h>
#include <gpu/gl/GLBackend.h>

#include <PathUtils.h>

#include "../Logging.h"
#include "../CompositorHelper.h"

static const QString MONO_PREVIEW = "Mono Preview";
static const QString DISABLE_PREVIEW = "Disable Preview";
static const QString FRAMERATE = DisplayPlugin::MENU_PATH() + ">Framerate";
static const QString DEVELOPER_MENU_PATH = "Developer>" + DisplayPlugin::MENU_PATH();
static const bool DEFAULT_MONO_VIEW = true;
#if !defined(Q_OS_MAC)
static const bool DEFAULT_DISABLE_PREVIEW = false;
#endif
static const glm::mat4 IDENTITY_MATRIX;
static const size_t NUMBER_OF_HANDS = 2;

//#define LIVE_SHADER_RELOAD 1
extern glm::vec3 getPoint(float yaw, float pitch);

static QString readFile(const QString& filename) {
    QFile file(filename);
    file.open(QFile::Text | QFile::ReadOnly);
    QString result;
    result.append(QTextStream(&file).readAll());
    return result;
}

glm::uvec2 HmdDisplayPlugin::getRecommendedUiSize() const {
    return CompositorHelper::VIRTUAL_SCREEN_SIZE;
}

QRect HmdDisplayPlugin::getRecommendedOverlayRect() const {
    return CompositorHelper::VIRTUAL_SCREEN_RECOMMENDED_OVERLAY_RECT;
}

bool HmdDisplayPlugin::beginFrameRender(uint32_t frameIndex) {
    if (!_vsyncEnabled && !_disablePreviewItemAdded) {
        _container->addMenuItem(PluginType::DISPLAY_PLUGIN, MENU_PATH(), DISABLE_PREVIEW,
            [this](bool clicked) {
                _disablePreview = clicked;
                _container->setBoolSetting("disableHmdPreview", _disablePreview);
                if (_disablePreview) {
                    _clearPreviewFlag = true;
                }
            }, true, _disablePreview);
        _disablePreviewItemAdded = true;
    }
    return Parent::beginFrameRender(frameIndex);
}


bool HmdDisplayPlugin::internalActivate() {
    _disablePreviewItemAdded = false;
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
    Parent::customizeContext();
    _overlayRenderer.build();
    auto geometryCache = DependencyManager::get<GeometryCache>();
    for (size_t i = 0; i < _geometryIds.size(); ++i) {
        _geometryIds[i] = geometryCache->allocateID();
    }
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
    _overlayRenderer = OverlayRenderer();
    _previewTexture.reset();

    auto geometryCache = DependencyManager::get<GeometryCache>();
    for (size_t i = 0; i < _geometryIds.size(); ++i) {
        geometryCache->releaseID(_geometryIds[i]);
    }
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
    PROFILE_RANGE_EX(__FUNCTION__, 0xff00ff00, (uint64_t)presentCount())

    // Composite together the scene, overlay and mouse cursor
    hmdPresent();

    if (!_disablePreview) {
        // screen preview mirroring
        auto sourceSize = _renderTargetSize;
        if (_monoPreview) {
            sourceSize.x >>= 1;
        }

        float shiftLeftBy = getLeftCenterPixel() - (sourceSize.x / 2);
        float newWidth = sourceSize.x - shiftLeftBy;

        const unsigned int RATIO_Y = 9;
        const unsigned int RATIO_X = 16;
        glm::uvec2 originalClippedSize { newWidth, newWidth * RATIO_Y / RATIO_X };

        glm::ivec4 viewport = getViewportForSourceSize(sourceSize);
        glm::ivec4 scissor = viewport;

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

                viewport.z *= 2;
            }

            batch.enableStereo(false);
            batch.resetViewTransform();
            batch.setFramebuffer(gpu::FramebufferPointer());
            batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, vec4(0));
            batch.setStateScissorRect(scissor); // was viewport
            batch.setViewportTransform(viewport);
            batch.setResourceTexture(0, _compositeFramebuffer->getRenderBuffer(0));
            batch.setPipeline(_presentPipeline);
            batch.draw(gpu::TRIANGLE_STRIP, 4);
        });
        swapBuffers();
    } else if (_clearPreviewFlag) {
        QImage image;
        if (_vsyncEnabled) {
            image = QImage(PathUtils::resourcesPath() + "images/preview.png");
        } else {
            image = QImage(PathUtils::resourcesPath() + "images/preview-disabled.png");
        }

        image = image.mirrored();
        image = image.convertToFormat(QImage::Format_RGBA8888);
        if (!_previewTexture) {
            _previewTexture.reset(
                gpu::Texture::create2D(
                gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA),
                image.width(), image.height(),
                gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR)));
            _previewTexture->setSource("HMD Preview Texture");
            _previewTexture->setUsage(gpu::Texture::Usage::Builder().withColor().build());
            _previewTexture->assignStoredMip(0, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA), image.byteCount(), image.constBits());
            _previewTexture->autoGenerateMips(-1);
        }
        
        if (getGLBackend()->isTextureReady(_previewTexture)) {
            auto viewport = getViewportForSourceSize(uvec2(_previewTexture->getDimensions()));

            render([&](gpu::Batch& batch) {
                batch.enableStereo(false);
                batch.resetViewTransform();
                batch.setFramebuffer(gpu::FramebufferPointer());
                batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, vec4(0));
                batch.setStateScissorRect(viewport);
                batch.setViewportTransform(viewport);
                batch.setResourceTexture(0, _previewTexture);
                batch.setPipeline(_presentPipeline);
                batch.draw(gpu::TRIANGLE_STRIP, 4);
            });
            _clearPreviewFlag = false;
            swapBuffers();
        }
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

    if (_currentFrame) {
        auto batchPose = _currentFrame->pose;
        auto currentPose = _currentPresentFrameInfo.presentPose;
        auto correction = glm::inverse(batchPose) * currentPose;
        getGLBackend()->setCameraCorrection(correction);
    }

    withPresentThreadLock([&] {
        _presentHandLasers = _handLasers;
        _presentHandPoses = _handPoses;
        _presentUiModelTransform = _uiModelTransform;
    });

    auto compositorHelper = DependencyManager::get<CompositorHelper>();
    glm::mat4 modelMat = compositorHelper->getModelTransform().getMatrix();
    std::array<vec2, NUMBER_OF_HANDS> handGlowPoints{ { vec2(-1), vec2(-1) } };

    // compute the glow point interesections
    for (size_t i = 0; i < NUMBER_OF_HANDS; ++i) {
        if (_presentHandPoses[i] == IDENTITY_MATRIX) {
            continue;
        }
        const auto& handLaser = _presentHandLasers[i];
        if (!handLaser.valid()) {
            continue;
        }

        const vec3& laserDirection = handLaser.direction;
        mat4 model = _presentHandPoses[i];
        vec3 castStart = vec3(model[3]);
        vec3 castDirection = glm::quat_cast(model) * laserDirection;
        if (glm::abs(glm::length2(castDirection) - 1.0f) > EPSILON) {
            castDirection = glm::normalize(castDirection);
            castDirection = glm::inverse(_presentUiModelTransform.getRotation()) * castDirection;
        }

        // this offset needs to match GRAB_POINT_SPHERE_OFFSET in scripts/system/libraries/controllers.js
        //static const vec3 GRAB_POINT_SPHERE_OFFSET = vec3(0.1f, 0.04f, -0.32f);
        static const vec3 GRAB_POINT_SPHERE_OFFSET = vec3(0.0f, 0.0f, -0.175f);
        vec3 grabPointOffset = GRAB_POINT_SPHERE_OFFSET;
        if (i == 0) {
            grabPointOffset.x *= -1.0f; // this changes between left and right hands
        }
        castStart += glm::quat_cast(model) * grabPointOffset;

        // FIXME fetch the actual UI radius from... somewhere?
        float uiRadius = 1.0f;

        // Find the intersection of the laser with he UI and use it to scale the model matrix
        float distance;
        if (!glm::intersectRaySphere(castStart, castDirection,
                                     _presentUiModelTransform.getTranslation(), uiRadius * uiRadius, distance)) {
            continue;
        }

        _presentHandLaserPoints[i].first = castStart;
        _presentHandLaserPoints[i].second = _presentHandLaserPoints[i].first + (castDirection * distance);

        vec3 intersectionPosition = castStart + (castDirection * distance) - _presentUiModelTransform.getTranslation();
        intersectionPosition = glm::inverse(_presentUiModelTransform.getRotation()) * intersectionPosition;

        // Take the interesection normal and convert it to a texture coordinate
        vec2 yawPitch;
        {
            vec2 xdir = glm::normalize(vec2(intersectionPosition.x, -intersectionPosition.z));
            yawPitch.x = glm::atan(xdir.x, xdir.y);
            yawPitch.y = (acosf(intersectionPosition.y) * -1.0f) + (float)M_PI_2;
        }
        vec2 halfFov = CompositorHelper::VIRTUAL_UI_TARGET_FOV / 2.0f;

        // Are we out of range
        if (glm::any(glm::greaterThan(glm::abs(yawPitch), halfFov))) {
            continue;
        }

        yawPitch /= CompositorHelper::VIRTUAL_UI_TARGET_FOV;
        yawPitch += 0.5f;
        handGlowPoints[i] = yawPitch;
    }


    for_each_eye([&](Eye eye) {
        auto modelView = glm::inverse(_currentPresentFrameInfo.presentPose * getEyeToHeadTransform(eye)) * modelMat;
        _overlayRenderer.mvps[eye] = _eyeProjections[eye] * modelView;
    });

    // Setup the uniforms
    {
        auto& uniforms = _overlayRenderer.uniforms;
        uniforms.alpha = _compositeOverlayAlpha;
        uniforms.glowPoints = vec4(handGlowPoints[0], handGlowPoints[1]);
        uniforms.glowColors[0] = _presentHandLasers[0].color;
        uniforms.glowColors[1] = _presentHandLasers[1].color;
    }
}

void HmdDisplayPlugin::OverlayRenderer::build() {
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
    uniformBuffers[0] = std::make_shared<gpu::Buffer>(sizeof(Uniforms), nullptr);
    uniformBuffers[1] = std::make_shared<gpu::Buffer>(sizeof(Uniforms), nullptr);
    updatePipeline();
}

void HmdDisplayPlugin::OverlayRenderer::updatePipeline() {
    static const QString vsFile = PathUtils::resourcesPath() + "/shaders/hmd_ui_glow.vert";
    static const QString fsFile = PathUtils::resourcesPath() + "/shaders/hmd_ui_glow.frag";

#if LIVE_SHADER_RELOAD
    static qint64 vsBuiltAge = 0;
    static qint64 fsBuiltAge = 0;
    QFileInfo vsInfo(vsFile);
    QFileInfo fsInfo(fsFile);
    auto vsAge = vsInfo.lastModified().toMSecsSinceEpoch();
    auto fsAge = fsInfo.lastModified().toMSecsSinceEpoch();
    if (!pipeline || vsAge > vsBuiltAge || fsAge > fsBuiltAge) {
        vsBuiltAge = vsAge;
        fsBuiltAge = fsAge;
#else
    if (!pipeline) {
#endif
        QString vsSource = readFile(vsFile);
        QString fsSource = readFile(fsFile);
        auto vs = gpu::Shader::createVertex(vsSource.toLocal8Bit().toStdString());
        auto ps = gpu::Shader::createPixel(fsSource.toLocal8Bit().toStdString());
        auto program = gpu::Shader::createProgram(vs, ps);
        gpu::gl::GLBackend::makeProgram(*program, gpu::Shader::BindingSet());
        this->uniformsLocation = program->getBuffers().findLocation("overlayBuffer");

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        state->setDepthTest(gpu::State::DepthTest(false));
        state->setBlendFunction(true,
            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);

        pipeline = gpu::Pipeline::create(program, state);
    }
}

void HmdDisplayPlugin::OverlayRenderer::render(HmdDisplayPlugin& plugin) {
    updatePipeline();
    for_each_eye([&](Eye eye){
        uniforms.mvp = mvps[eye];
        uniformBuffers[eye]->setSubData(0, uniforms);
    });
    plugin.render([&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setFramebuffer(plugin._compositeFramebuffer);
        batch.setPipeline(pipeline);
        batch.setInputFormat(format);
        gpu::BufferView posView(vertices, VERTEX_OFFSET, vertices->getSize(), VERTEX_STRIDE, format->getAttributes().at(gpu::Stream::POSITION)._element);
        gpu::BufferView uvView(vertices, TEXTURE_OFFSET, vertices->getSize(), VERTEX_STRIDE, format->getAttributes().at(gpu::Stream::TEXCOORD)._element);
        batch.setInputBuffer(gpu::Stream::POSITION, posView);
        batch.setInputBuffer(gpu::Stream::TEXCOORD, uvView);
        batch.setIndexBuffer(gpu::UINT16, indices, 0);
        batch.setResourceTexture(0, plugin._currentFrame->overlay);
        // FIXME use stereo information input to set both MVPs in the uniforms
        for_each_eye([&](Eye eye) {
            batch.setUniformBuffer(uniformsLocation, uniformBuffers[eye]);
            batch.setViewportTransform(plugin.eyeViewport(eye));
            batch.drawIndexed(gpu::TRIANGLES, indexCount);
        });
    });
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

void HmdDisplayPlugin::compositeOverlay() {
    if (!_currentFrame || !_currentFrame->overlay) {
        return;
    }

    _overlayRenderer.render(*this);
}

bool HmdDisplayPlugin::setHandLaser(uint32_t hands, HandLaserMode mode, const vec4& color, const vec3& direction) {
    HandLaserInfo info;
    info.mode = mode;
    info.color = color;
    info.direction = direction;
    withNonPresentThreadLock([&] {
        if (hands & Hand::LeftHand) {
            _handLasers[0] = info;
        }
        if (hands & Hand::RightHand) {
            _handLasers[1] = info;
        }
    });
    // FIXME defer to a child class plugin to determine if hand lasers are actually 
    // available based on the presence or absence of hand controllers
    return true;
}

void HmdDisplayPlugin::compositeExtra() {
    // If neither hand laser is activated, exit
    if (!_presentHandLasers[0].valid() && !_presentHandLasers[1].valid()) {
        return;
    }

    if (_presentHandPoses[0] == IDENTITY_MATRIX && _presentHandPoses[1] == IDENTITY_MATRIX) {
        return;
    }

    auto geometryCache = DependencyManager::get<GeometryCache>();
    render([&](gpu::Batch& batch) {
        batch.setFramebuffer(_compositeFramebuffer);
        batch.setViewportTransform(ivec4(uvec2(0), _renderTargetSize));
        batch.setViewTransform(_currentPresentFrameInfo.presentPose, false);
        bilateral::for_each_side([&](bilateral::Side side){
            auto index = bilateral::index(side);
            if (_presentHandPoses[index] == IDENTITY_MATRIX) {
                return;
            }
            const auto& laser = _presentHandLasers[index];
            if (laser.valid()) {
                const auto& points = _presentHandLaserPoints[index];
                geometryCache->renderGlowLine(batch, points.first, points.second, laser.color, _geometryIds[index]);
            }
        });
    });
}

HmdDisplayPlugin::~HmdDisplayPlugin() {
    qDebug() << "Destroying HmdDisplayPlugin";
}
