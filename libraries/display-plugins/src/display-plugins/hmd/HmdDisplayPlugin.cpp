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

#include <gpu/DrawUnitQuadTexcoord_vert.h>
#include <gpu/DrawTexture_frag.h>

#include <PathUtils.h>

#include "../Logging.h"
#include "../CompositorHelper.h"

static const QString MONO_PREVIEW = "Mono Preview";
static const QString REPROJECTION = "Allow Reprojection";
static const QString FRAMERATE = DisplayPlugin::MENU_PATH() + ">Framerate";
static const QString DEVELOPER_MENU_PATH = "Developer>" + DisplayPlugin::MENU_PATH();
static const bool DEFAULT_MONO_VIEW = true;
static const int NUMBER_OF_HANDS = 2;
static const glm::mat4 IDENTITY_MATRIX;


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

    return Parent::internalActivate();
}

void HmdDisplayPlugin::internalDeactivate() {
    if (_previewTextureID != 0) {
        glDeleteTextures(1, &_previewTextureID);
        _previewTextureID = 0;
    }
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
    _sphereSection = loadSphereSection(_program, CompositorHelper::VIRTUAL_UI_TARGET_FOV.y, CompositorHelper::VIRTUAL_UI_ASPECT_RATIO);
    using namespace oglplus;
    if (!_enablePreview) {
        const std::string version("#version 410 core\n");
        compileProgram(_previewProgram, version + DrawUnitQuadTexcoord_vert, version + DrawTexture_frag);
        _previewUniforms.previewTexture = Uniform<int>(*_previewProgram, "colorMap").Location();
    }

    updateReprojectionProgram();
    updateOverlayProgram();
    updateLaserProgram();

    _laserGeometry = loadLaser(_laserProgram);
}
//#define LIVE_SHADER_RELOAD 1

static QString readFile(const QString& filename) {
    QFile file(filename);
    file.open(QFile::Text | QFile::ReadOnly);
    QString result;
    result.append(QTextStream(&file).readAll());
    return result;
}

void HmdDisplayPlugin::updateReprojectionProgram() {
    static const QString vsFile = PathUtils::resourcesPath() + "/shaders/hmd_reproject.vert";
    static const QString fsFile = PathUtils::resourcesPath() + "/shaders/hmd_reproject.frag";
#if LIVE_SHADER_RELOAD
    static qint64 vsBuiltAge = 0;
    static qint64 fsBuiltAge = 0;
    QFileInfo vsInfo(vsFile);
    QFileInfo fsInfo(fsFile);
    auto vsAge = vsInfo.lastModified().toMSecsSinceEpoch();
    auto fsAge = fsInfo.lastModified().toMSecsSinceEpoch();
    if (!_reprojectionProgram || vsAge > vsBuiltAge || fsAge > fsBuiltAge) {
        vsBuiltAge = vsAge;
        fsBuiltAge = fsAge;
#else
    if (!_reprojectionProgram) {
#endif
        QString vsSource = readFile(vsFile);
        QString fsSource = readFile(fsFile);
        ProgramPtr program;
        try {
            compileProgram(program, vsSource.toLocal8Bit().toStdString(), fsSource.toLocal8Bit().toStdString());
            if (program) {
                using namespace oglplus;
                _reprojectionUniforms.reprojectionMatrix = Uniform<glm::mat3>(*program, "reprojection").Location();
                _reprojectionUniforms.inverseProjectionMatrix = Uniform<glm::mat4>(*program, "inverseProjections").Location();
                _reprojectionUniforms.projectionMatrix = Uniform<glm::mat4>(*program, "projections").Location();
                _reprojectionProgram = program;
            }
        } catch (std::runtime_error& error) {
            qWarning() << "Error building reprojection shader " << error.what();
        }
    }

}

void HmdDisplayPlugin::updateLaserProgram() {
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
}

void HmdDisplayPlugin::updateOverlayProgram() {
    static const QString vsFile = PathUtils::resourcesPath() + "/shaders/hmd_ui_glow.vert";
    static const QString fsFile = PathUtils::resourcesPath() + "/shaders/hmd_ui_glow.frag";

#if LIVE_SHADER_RELOAD
    static qint64 vsBuiltAge = 0;
    static qint64 fsBuiltAge = 0;
    QFileInfo vsInfo(vsFile);
    QFileInfo fsInfo(fsFile);
    auto vsAge = vsInfo.lastModified().toMSecsSinceEpoch();
    auto fsAge = fsInfo.lastModified().toMSecsSinceEpoch();
    if (!_overlayProgram || vsAge > vsBuiltAge || fsAge > fsBuiltAge) {
        vsBuiltAge = vsAge;
        fsBuiltAge = fsAge;
#else
    if (!_overlayProgram) {
#endif
        QString vsSource = readFile(vsFile);
        QString fsSource = readFile(fsFile);
        ProgramPtr program;
        try {
            compileProgram(program, vsSource.toLocal8Bit().toStdString(), fsSource.toLocal8Bit().toStdString());
            if (program) {
                using namespace oglplus;
                _overlayUniforms.mvp = Uniform<glm::mat4>(*program, "mvp").Location();
                _overlayUniforms.alpha = Uniform<float>(*program, "alpha").Location();
                _overlayUniforms.glowColors = Uniform<glm::vec4>(*program, "glowColors").Location();
                _overlayUniforms.glowPoints = Uniform<glm::vec4>(*program, "glowPoints").Location();
                _overlayUniforms.resolution = Uniform<glm::vec2>(*program, "resolution").Location();
                _overlayUniforms.radius = Uniform<float>(*program, "radius").Location();
                _overlayProgram = program;
                useProgram(_overlayProgram);
                Uniform<glm::vec2>(*_overlayProgram, _overlayUniforms.resolution).Set(CompositorHelper::VIRTUAL_SCREEN_SIZE);
            }
        } catch (std::runtime_error& error) {
            qWarning() << "Error building overlay composite shader " << error.what();
        }
    }
}

void HmdDisplayPlugin::uncustomizeContext() {
    _overlayProgram.reset();
    _sphereSection.reset();
    _compositeFramebuffer.reset();
    _previewProgram.reset();
    _reprojectionProgram.reset();
    _laserProgram.reset();
    _laserGeometry.reset();
    Parent::uncustomizeContext();
}

// By default assume we'll present with the same pose as the render
void HmdDisplayPlugin::updatePresentPose() {
    _currentPresentFrameInfo.presentPose = _currentPresentFrameInfo.renderPose;
}

void HmdDisplayPlugin::compositeScene() {
    updatePresentPose();

    if (!_enableReprojection || glm::mat3() == _currentPresentFrameInfo.presentReprojection) {
        // No reprojection required
        Parent::compositeScene();
        return;
    }

#ifdef DEBUG_REPROJECTION_SHADER
    _reprojectionProgram = getReprojectionProgram();
#endif
    useProgram(_reprojectionProgram);

    using namespace oglplus;
    Texture::MinFilter(TextureTarget::_2D, TextureMinFilter::Linear);
    Texture::MagFilter(TextureTarget::_2D, TextureMagFilter::Linear);
    Uniform<glm::mat3>(*_reprojectionProgram, _reprojectionUniforms.reprojectionMatrix).Set(_currentPresentFrameInfo.presentReprojection);
    //Uniform<glm::mat4>(*_reprojectionProgram, PROJECTION_MATRIX_LOCATION).Set(_eyeProjections);
    //Uniform<glm::mat4>(*_reprojectionProgram, INVERSE_PROJECTION_MATRIX_LOCATION).Set(_eyeInverseProjections);
    // FIXME what's the right oglplus mechanism to do this?  It's not that ^^^ ... better yet, switch to a uniform buffer
    glUniformMatrix4fv(_reprojectionUniforms.inverseProjectionMatrix, 2, GL_FALSE, &(_eyeInverseProjections[0][0][0]));
    glUniformMatrix4fv(_reprojectionUniforms.projectionMatrix, 2, GL_FALSE, &(_eyeProjections[0][0][0]));
    _plane->UseInProgram(*_reprojectionProgram);
    _plane->Draw();
}

void HmdDisplayPlugin::compositeOverlay() {
    using namespace oglplus;
    auto compositorHelper = DependencyManager::get<CompositorHelper>();
    glm::mat4 modelMat = compositorHelper->getModelTransform().getMatrix();

    withPresentThreadLock([&] {
        _presentHandLasers = _handLasers;
        _presentHandPoses = _handPoses;
        _presentUiModelTransform = _uiModelTransform;
    });
    std::array<vec2, NUMBER_OF_HANDS> handGlowPoints { { vec2(-1), vec2(-1) } };

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

    updateOverlayProgram();
    if (!_overlayProgram) {
        return;
    }

    useProgram(_overlayProgram);
    // Setup the uniforms
    {
        if (_overlayUniforms.alpha >= 0) {
            Uniform<float>(*_overlayProgram, _overlayUniforms.alpha).Set(_compositeOverlayAlpha);
        }
        if (_overlayUniforms.glowPoints >= 0) {
            vec4 glowPoints(handGlowPoints[0], handGlowPoints[1]);
            Uniform<glm::vec4>(*_overlayProgram, _overlayUniforms.glowPoints).Set(glowPoints);
        }
        if (_overlayUniforms.glowColors >= 0) {
            std::array<glm::vec4, NUMBER_OF_HANDS> glowColors;
            glowColors[0] = _presentHandLasers[0].color;
            glowColors[1] = _presentHandLasers[1].color;
            glProgramUniform4fv(GetName(*_overlayProgram), _overlayUniforms.glowColors, 2, &glowColors[0].r);
        }
    }

    _sphereSection->Use();
    for_each_eye([&](Eye eye) {
        eyeViewport(eye);
        auto modelView = glm::inverse(_currentPresentFrameInfo.presentPose * getEyeToHeadTransform(eye)) * modelMat;
        auto mvp = _eyeProjections[eye] * modelView;
        Uniform<glm::mat4>(*_overlayProgram, _overlayUniforms.mvp).Set(mvp);
        _sphereSection->Draw();
    });
}

void HmdDisplayPlugin::compositePointer() {
    using namespace oglplus;

    auto compositorHelper = DependencyManager::get<CompositorHelper>();

    useProgram(_program);
    // set the alpha
    Uniform<float>(*_program, _alphaUniform).Set(_compositeOverlayAlpha);

    // Mouse pointer
    _plane->Use();
    // Reconstruct the headpose from the eye poses
    auto headPosition = vec3(_currentPresentFrameInfo.presentPose[3]);
    for_each_eye([&](Eye eye) {
        eyeViewport(eye);
        auto eyePose = _currentPresentFrameInfo.presentPose * getEyeToHeadTransform(eye);
        auto reticleTransform = compositorHelper->getReticleTransform(eyePose, headPosition);
        auto mvp = _eyeProjections[eye] * reticleTransform;
        Uniform<glm::mat4>(*_program, _mvpUniform).Set(mvp);
        _plane->Draw();
    });
    // restore the alpha
    Uniform<float>(*_program, _alphaUniform).Set(1.0);
}


void HmdDisplayPlugin::internalPresent() {

    PROFILE_RANGE_EX(__FUNCTION__, 0xff00ff00, (uint64_t)presentCount())

    // Composite together the scene, overlay and mouse cursor
    hmdPresent();

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

    if (_enablePreview) {
        using namespace oglplus;
        Context::Clear().ColorBuffer();
        auto sourceSize = _compositeFramebuffer->size;
        if (_monoPreview) {
            sourceSize.x /= 2;
        }
        _compositeFramebuffer->Bound(Framebuffer::Target::Read, [&] {
            Context::BlitFramebuffer(
                0, 0, sourceSize.x, sourceSize.y,
                targetViewportPosition.x, targetViewportPosition.y, 
                targetViewportPosition.x + targetViewportSize.x, targetViewportPosition.y + targetViewportSize.y,
                BufferSelectBit::ColorBuffer, BlitFilter::Nearest);
        });
        swapBuffers();
    } else if (_firstPreview || windowSize != _prevWindowSize || devicePixelRatio != _prevDevicePixelRatio) {
        useProgram(_previewProgram);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(targetViewportPosition.x, targetViewportPosition.y, targetViewportSize.x, targetViewportSize.y);
        glUniform1i(_previewUniforms.previewTexture, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _previewTextureID);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        swapBuffers();
        _firstPreview = false;
        _prevWindowSize = windowSize;
        _prevDevicePixelRatio = devicePixelRatio;
    }

    postPreview();
}

void HmdDisplayPlugin::setEyeRenderPose(uint32_t frameIndex, Eye eye, const glm::mat4& pose) {
}

void HmdDisplayPlugin::updateFrameData() {
    // Check if we have old frame data to discard
    withPresentThreadLock([&] {
        auto itr = _frameInfos.find(_currentPresentFrameIndex);
        if (itr != _frameInfos.end()) {
            _frameInfos.erase(itr);
        }
    });

    Parent::updateFrameData();

    withPresentThreadLock([&] {
        _currentPresentFrameInfo = _frameInfos[_currentPresentFrameIndex];
    });
}

glm::mat4 HmdDisplayPlugin::getHeadPose() const {
    return _currentRenderFrameInfo.renderPose;
}

bool HmdDisplayPlugin::setHandLaser(uint32_t hands, HandLaserMode mode, const vec4& color, const vec3& direction) {
    HandLaserInfo info;
    info.mode = mode;
    info.color = color;
    info.direction = direction;
    withRenderThreadLock([&] {
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
}
