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
#include <gpu/StandardShaderLib.h>
#include <gpu/gl/GLBackend.h>

#include <PathUtils.h>

#include "../Logging.h"
#include "../CompositorHelper.h"

static const QString MONO_PREVIEW = "Mono Preview";
static const QString REPROJECTION = "Allow Reprojection";
static const QString FRAMERATE = DisplayPlugin::MENU_PATH() + ">Framerate";
static const QString DEVELOPER_MENU_PATH = "Developer>" + DisplayPlugin::MENU_PATH();
static const bool DEFAULT_MONO_VIEW = true;
static const glm::mat4 IDENTITY_MATRIX;

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

bool HmdDisplayPlugin::internalActivate() {
    _monoPreview = _container->getBoolSetting("monoPreview", DEFAULT_MONO_VIEW);

    _container->addMenuItem(PluginType::DISPLAY_PLUGIN, MENU_PATH(), MONO_PREVIEW,
        [this](bool clicked) {
        _monoPreview = clicked;
        _container->setBoolSetting("monoPreview", _monoPreview);
    }, true, _monoPreview);
    _container->removeMenu(FRAMERATE);
    _container->addMenu(DEVELOPER_MENU_PATH);
    _container->addMenuItem(PluginType::DISPLAY_PLUGIN, DEVELOPER_MENU_PATH, REPROJECTION,
        [this](bool clicked) {
            _enableReprojection = clicked;
            _container->setBoolSetting("enableReprojection", _enableReprojection);
    }, true, _enableReprojection);
    
    for_each_eye([&](Eye eye) {
        _eyeInverseProjections[eye] = glm::inverse(_eyeProjections[eye]);
    });

#if 0
    if (_previewTextureID == 0) {
        QImage previewTexture(PathUtils::resourcesPath() + "images/preview.png");
        if (!previewTexture.isNull()) {
            glGenTextures(1, &_previewTextureID);
            glBindTexture(GL_TEXTURE_2D, _previewTextureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, previewTexture.width(), previewTexture.height(), 0,
                         GL_BGRA, GL_UNSIGNED_BYTE, previewTexture.mirrored(false, true).bits());
            using namespace oglplus;
            Texture::MinFilter(TextureTarget::_2D, TextureMinFilter::Linear);
            Texture::MagFilter(TextureTarget::_2D, TextureMagFilter::Linear);
            glBindTexture(GL_TEXTURE_2D, 0);
            _previewAspect = ((float)previewTexture.width())/((float)previewTexture.height());
            _firstPreview = true;
        }
    }
#endif

    return Parent::internalActivate();
}

void HmdDisplayPlugin::internalDeactivate() {
    Parent::internalDeactivate();
}

void HmdDisplayPlugin::customizeContext() {
    Parent::customizeContext();
    // Only enable mirroring if we know vsync is disabled
    // On Mac, this won't work due to how the contexts are handled, so don't try
#if !defined(Q_OS_MAC)
    enableVsync(false);
#endif
    _enablePreview = !isVsyncEnabled();
    _overlay.build();
#if 0
    updateReprojectionProgram();
    updateLaserProgram();
    _laserGeometry = loadLaser(_laserProgram);
#endif
}

void HmdDisplayPlugin::uncustomizeContext() {
    _overlay = OverlayRenderer();
#if 0
    _previewProgram.reset();
    _laserProgram.reset();
    _laserGeometry.reset();
#endif
    getGLBackend()->setCameraCorrection(mat4());
    Parent::uncustomizeContext();
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
    vertices->flush();

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
    this->indices->flush();
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
    for_each_eye([&](Eye eye){
        uniforms.mvp = mvps[eye];
        uniformBuffers[eye]->setSubData(0, uniforms);
        uniformBuffers[eye]->flush();
    });
    gpu::Batch batch;
    batch.enableStereo(false);
    batch.setResourceTexture(0, plugin._currentFrame->overlay);
    batch.setPipeline(pipeline);
    batch.setInputFormat(format);
    gpu::BufferView posView(vertices, VERTEX_OFFSET, vertices->getSize(), VERTEX_STRIDE, format->getAttributes().at(gpu::Stream::POSITION)._element);
    gpu::BufferView uvView(vertices, TEXTURE_OFFSET, vertices->getSize(), VERTEX_STRIDE, format->getAttributes().at(gpu::Stream::TEXCOORD)._element);
    batch.setInputBuffer(gpu::Stream::POSITION, posView);
    batch.setInputBuffer(gpu::Stream::TEXCOORD, uvView);
    batch.setIndexBuffer(gpu::UINT16, indices, 0);
    for_each_eye([&](Eye eye){
        batch.setUniformBuffer(uniformsLocation, uniformBuffers[eye]);
        batch.setViewportTransform(plugin.eyeViewport(eye));
        batch.drawIndexed(gpu::TRIANGLES, indexCount);
    });
    // FIXME use stereo information input to set both MVPs in the uniforms
    plugin._backend->render(batch);
}

void HmdDisplayPlugin::updateLaserProgram() {
#if 0
    static const QString vsFile = PathUtils::resourcesPath() + "/shaders/hmd_hand_lasers.vert";
    static const QString gsFile = PathUtils::resourcesPath() + "/shaders/hmd_hand_lasers.geom";
    static const QString fsFile = PathUtils::resourcesPath() + "/shaders/hmd_hand_lasers.frag";

#if LIVE_SHADER_RELOAD
    static qint64 vsBuiltAge = 0;
    static qint64 gsBuiltAge = 0;
    static qint64 fsBuiltAge = 0;
    QFileInfo vsInfo(vsFile);
    QFileInfo fsInfo(fsFile);
    QFileInfo gsInfo(fsFile);
    auto vsAge = vsInfo.lastModified().toMSecsSinceEpoch();
    auto fsAge = fsInfo.lastModified().toMSecsSinceEpoch();
    auto gsAge = gsInfo.lastModified().toMSecsSinceEpoch();
    if (!_laserProgram || vsAge > vsBuiltAge || fsAge > fsBuiltAge || gsAge > gsBuiltAge) {
        vsBuiltAge = vsAge;
        gsBuiltAge = gsAge;
        fsBuiltAge = fsAge;
#else
    if (!_laserProgram) {
#endif

        QString vsSource = readFile(vsFile);
        QString fsSource = readFile(fsFile);
        QString gsSource = readFile(gsFile);
        ProgramPtr program;
        try {
            compileProgram(program, vsSource.toLocal8Bit().toStdString(), gsSource.toLocal8Bit().toStdString(), fsSource.toLocal8Bit().toStdString());
            if (program) {
                using namespace oglplus;
                _laserUniforms.color = Uniform<glm::vec4>(*program, "color").Location();
                _laserUniforms.mvp = Uniform<glm::mat4>(*program, "mvp").Location();
                _laserProgram = program;
            }
        } catch (std::runtime_error& error) {
            qWarning() << "Error building hand laser composite shader " << error.what();
        }
    }
#endif
}

// By default assume we'll present with the same pose as the render
void HmdDisplayPlugin::updatePresentPose() {
    _currentPresentFrameInfo.presentPose = _currentPresentFrameInfo.renderPose;
}

void HmdDisplayPlugin::compositeScene() {
    gpu::Batch batch;
    batch.enableStereo(false);
    batch.setFramebuffer(_compositeFramebuffer);

    batch.setViewportTransform(ivec4(uvec2(), _renderTargetSize));
    batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(1, 1, 0, 1));
    batch.clearViewTransform();
    batch.setProjectionTransform(mat4());

    batch.setPipeline(_presentPipeline);
    batch.setResourceTexture(0, _currentFrame->framebuffer->getRenderBuffer(0));
    batch.draw(gpu::TRIANGLE_STRIP, 4);
    _backend->render(batch);
}

void HmdDisplayPlugin::compositeOverlay() {
#if 0
    if (!_currentFrame) {
        return;
    }

    auto compositorHelper = DependencyManager::get<CompositorHelper>();
    glm::mat4 modelMat = compositorHelper->getModelTransform().getMatrix();
    withPresentThreadLock([&] {
        _presentHandLasers = _handLasers;
        _presentHandPoses = _handPoses;
        _presentUiModelTransform = _uiModelTransform;
    });

    std::array<vec2, NUMBER_OF_HANDS> handGlowPoints{ { vec2(-1), vec2(-1) } };
    // compute the glow point interesections
    for (int i = 0; i < NUMBER_OF_HANDS; ++i) {
        if (_presentHandPoses[i] == IDENTITY_MATRIX) {
            continue;
        }
        const auto& handLaser = _presentHandLasers[i];
        if (!handLaser.valid()) {
            continue;
        }

        const auto& laserDirection = handLaser.direction;
        auto model = _presentHandPoses[i];
        auto castDirection = glm::quat_cast(model) * laserDirection;
        if (glm::abs(glm::length2(castDirection) - 1.0f) > EPSILON) {
            castDirection = glm::normalize(castDirection);
            castDirection = glm::inverse(_presentUiModelTransform.getRotation()) * castDirection;
        }

        // FIXME fetch the actual UI radius from... somewhere?
        float uiRadius = 1.0f;

        // Find the intersection of the laser with he UI and use it to scale the model matrix
        float distance;
        if (!glm::intersectRaySphere(vec3(_presentHandPoses[i][3]), castDirection, _presentUiModelTransform.getTranslation(), uiRadius * uiRadius, distance)) {
            continue;
        }

        vec3 intersectionPosition = vec3(_presentHandPoses[i][3]) + (castDirection * distance) - _presentUiModelTransform.getTranslation();
        intersectionPosition = glm::inverse(_presentUiModelTransform.getRotation()) * intersectionPosition;

        // Take the interesection normal and convert it to a texture coordinate
        vec2 yawPitch;
        {
            vec2 xdir = glm::normalize(vec2(intersectionPosition.x, -intersectionPosition.z));
            yawPitch.x = glm::atan(xdir.x, xdir.y);
            yawPitch.y = (acosf(intersectionPosition.y) * -1.0f) + M_PI_2;
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

    if (!_currentFrame->overlay) {
        return;
    }

    for_each_eye([&](Eye eye){
        auto modelView = glm::inverse(_currentPresentFrameInfo.presentPose * getEyeToHeadTransform(eye)) * modelMat;
        _overlay.mvps[eye] = _eyeProjections[eye] * modelView;
    });

    // Setup the uniforms
    {
        _overlay.uniforms.alpha = _compositeOverlayAlpha;
        _overlay.uniforms.glowPoints = vec4(handGlowPoints[0], handGlowPoints[1]);
        _overlay.uniforms.glowColors[0] = _presentHandLasers[0].color;
        _overlay.uniforms.glowColors[1] = _presentHandLasers[1].color;
    }
    _overlay.render();
#endif
}

void HmdDisplayPlugin::compositePointer() {
#if 0
    auto& cursorManager = Cursor::Manager::instance();
    const auto& cursorData = _cursorsData[cursorManager.getCursor()->getIcon()];
    auto compositorHelper = DependencyManager::get<CompositorHelper>();
    // Reconstruct the headpose from the eye poses
    auto headPosition = vec3(_currentPresentFrameInfo.presentPose[3]);
    gpu::Batch batch;
    batch.enableStereo(false);
    batch.setProjectionTransform(mat4());
    batch.setFramebuffer(_currentFrame->framebuffer);
    batch.setPipeline(_cursorPipeline);
    batch.setResourceTexture(0, cursorData.texture);
    batch.clearViewTransform();
    for_each_eye([&](Eye eye) {
        auto eyePose = _currentPresentFrameInfo.presentPose * getEyeToHeadTransform(eye);
        auto reticleTransform = compositorHelper->getReticleTransform(eyePose, headPosition);
        batch.setViewportTransform(eyeViewport(eye));
        batch.setModelTransform(reticleTransform);
        batch.setProjectionTransform(_eyeProjections[eye]);
        batch.draw(gpu::TRIANGLE_STRIP, 4);
    });
    _backend->render(batch);
#endif
}

void HmdDisplayPlugin::internalPresent() {
    PROFILE_RANGE_EX(__FUNCTION__, 0xff00ff00, (uint64_t)presentCount())

    // Composite together the scene, overlay and mouse cursor
    hmdPresent();

    /*
    // screen preview mirroring
    auto window = _container->getPrimaryWidget();
    auto devicePixelRatio = window->devicePixelRatio();
    auto windowSize = toGlm(window->size());
    windowSize *= devicePixelRatio;
    float windowAspect = aspect(windowSize);
    float sceneAspect = _enablePreview ? aspect(_renderTargetSize) : _previewAspect;
    if (_enablePreview && _monoPreview) {
        sceneAspect /= 2.0f;
    }
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
    */

    if (_enablePreview) {
        gpu::Batch presentBatch;
        presentBatch.enableStereo(false);
        presentBatch.clearViewTransform();
        presentBatch.setFramebuffer(gpu::FramebufferPointer());
        presentBatch.setViewportTransform(ivec4(uvec2(0), getSurfacePixels()));
        presentBatch.setResourceTexture(0, _compositeTexture);
        presentBatch.setPipeline(_presentPipeline);
        presentBatch.draw(gpu::TRIANGLE_STRIP, 4);
        _backend->render(presentBatch);
        swapBuffers();
    } 

    postPreview();
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

}

glm::mat4 HmdDisplayPlugin::getHeadPose() const {
    return _currentRenderFrameInfo.renderPose;
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
#if 0
    // If neither hand laser is activated, exit
    if (!_presentHandLasers[0].valid() && !_presentHandLasers[1].valid()) {
        return;
    }

    if (_presentHandPoses[0] == IDENTITY_MATRIX && _presentHandPoses[1] == IDENTITY_MATRIX) {
        return;
    }

    updateLaserProgram();

    // Render hand lasers
    using namespace oglplus;
    useProgram(_laserProgram);
    _laserGeometry->Use();
    std::array<mat4, NUMBER_OF_HANDS> handLaserModelMatrices;

    for (int i = 0; i < NUMBER_OF_HANDS; ++i) {
        if (_presentHandPoses[i] == IDENTITY_MATRIX) {
            continue;
        }
        const auto& handLaser = _presentHandLasers[i];
        if (!handLaser.valid()) {
            continue;
        }

        const auto& laserDirection = handLaser.direction;
        auto model = _presentHandPoses[i];
        auto castDirection = glm::quat_cast(model) * laserDirection;
        if (glm::abs(glm::length2(castDirection) - 1.0f) > EPSILON) {
            castDirection = glm::normalize(castDirection);
        }

        // FIXME fetch the actual UI radius from... somewhere?
        float uiRadius = 1.0f;

        // Find the intersection of the laser with he UI and use it to scale the model matrix
        float distance; 
        if (!glm::intersectRaySphere(vec3(_presentHandPoses[i][3]), castDirection, _presentUiModelTransform.getTranslation(), uiRadius * uiRadius, distance)) {
            continue;
        }

        // Make sure we rotate to match the desired laser direction
        if (laserDirection != Vectors::UNIT_NEG_Z) {
            auto rotation = glm::rotation(Vectors::UNIT_NEG_Z, laserDirection);
            model = model * glm::mat4_cast(rotation);
        }

        model = glm::scale(model, vec3(distance));
        handLaserModelMatrices[i] = model;
    }

    glEnable(GL_BLEND);
    for_each_eye([&](Eye eye) {
        eyeViewport(eye);
        auto eyePose = _currentPresentFrameInfo.presentPose * getEyeToHeadTransform(eye);
        auto view = glm::inverse(eyePose);
        const auto& projection = _eyeProjections[eye];
        for (int i = 0; i < NUMBER_OF_HANDS; ++i) {
            if (handLaserModelMatrices[i] == IDENTITY_MATRIX) {
                continue;
            }
            Uniform<glm::mat4>(*_laserProgram, "mvp").Set(projection * view * handLaserModelMatrices[i]);
            Uniform<glm::vec4>(*_laserProgram, "color").Set(_presentHandLasers[i].color);
            _laserGeometry->Draw();
        }
    });
    glDisable(GL_BLEND);
#endif

}

