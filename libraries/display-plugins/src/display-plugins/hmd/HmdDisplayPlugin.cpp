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
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

#include <GLMHelpers.h>
#include <ui-plugins/PluginContainer.h>
#include <CursorManager.h>
#include <gl/GLWidget.h>
#include <shared/NsightHelpers.h>

#include "../Logging.h"
#include "../CompositorHelper.h"

static const QString MONO_PREVIEW = "Mono Preview";
static const QString REPROJECTION = "Allow Reprojection";
static const QString FRAMERATE = DisplayPlugin::MENU_PATH() + ">Framerate";
static const QString DEVELOPER_MENU_PATH = "Developer>" + DisplayPlugin::MENU_PATH();
static const bool DEFAULT_MONO_VIEW = true;

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

    return Parent::internalActivate();
}


static const char * REPROJECTION_VS = R"VS(#version 410 core
in vec3 Position;
in vec2 TexCoord;

out vec3 vPosition;
out vec2 vTexCoord;

void main() {
  gl_Position = vec4(Position, 1);
  vTexCoord = TexCoord;
  vPosition = Position;
}

)VS";

static GLint REPROJECTION_MATRIX_LOCATION = -1;
static GLint INVERSE_PROJECTION_MATRIX_LOCATION = -1;
static GLint PROJECTION_MATRIX_LOCATION = -1;
static const char * REPROJECTION_FS = R"FS(#version 410 core

uniform sampler2D sampler;
uniform mat3 reprojection = mat3(1);
uniform mat4 inverseProjections[2];
uniform mat4 projections[2];

in vec2 vTexCoord;
in vec3 vPosition;

out vec4 FragColor;

void main() {
    vec2 uv = vTexCoord;

    mat4 eyeInverseProjection;
    mat4 eyeProjection;
    
    float xoffset = 1.0;
    vec2 uvmin = vec2(0.0);
    vec2 uvmax = vec2(1.0);
    // determine the correct projection and inverse projection to use.
    if (vTexCoord.x < 0.5) {
        uvmax.x = 0.5;
        eyeInverseProjection = inverseProjections[0];
        eyeProjection = projections[0];
    } else {
        xoffset = -1.0;
        uvmin.x = 0.5;
        uvmax.x = 1.0;
        eyeInverseProjection = inverseProjections[1];
        eyeProjection = projections[1];
    }

    // Account for stereo in calculating the per-eye NDC coordinates
    vec4 ndcSpace = vec4(vPosition, 1.0);
    ndcSpace.x *= 2.0;
    ndcSpace.x += xoffset;
    
    // Convert from NDC to eyespace
    vec4 eyeSpace = eyeInverseProjection * ndcSpace;
    eyeSpace /= eyeSpace.w;

    // Convert to a noramlized ray 
    vec3 ray = eyeSpace.xyz;
    ray = normalize(ray);

    // Adjust the ray by the rotation
    ray = reprojection * ray;

    // Project back on to the texture plane
    ray *= eyeSpace.z / ray.z;

    // Update the eyespace vector
    eyeSpace.xyz = ray;

    // Reproject back into NDC
    ndcSpace = eyeProjection * eyeSpace;
    ndcSpace /= ndcSpace.w;
    ndcSpace.x -= xoffset;
    ndcSpace.x /= 2.0;
    
    // Calculate the new UV coordinates
    uv = (ndcSpace.xy / 2.0) + 0.5;
    if (any(greaterThan(uv, uvmax)) || any(lessThan(uv, uvmin))) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        FragColor = texture(sampler, uv);
    }
}
)FS";

#ifdef DEBUG_REPROJECTION_SHADER
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDateTime>
#include <PathUtils.h>

static const QString REPROJECTION_FS_FILE =  "c:/Users/bdavis/Git/hifi/interface/resources/shaders/reproject.frag";

static ProgramPtr getReprojectionProgram() {
    static ProgramPtr _currentProgram;
    uint64_t now = usecTimestampNow();
    static uint64_t _lastFileCheck = now;

    bool modified = false;
    if ((now - _lastFileCheck) > USECS_PER_MSEC * 100) {
        QFileInfo info(REPROJECTION_FS_FILE);
        QDateTime lastModified = info.lastModified();
        static QDateTime _lastModified = lastModified;
        qDebug() << lastModified.toTime_t();
        qDebug() << _lastModified.toTime_t();
        if (lastModified > _lastModified) {
            _lastModified = lastModified;
            modified = true;
        }
    }

    if (!_currentProgram || modified) {
        _currentProgram.reset();
        try {
            QFile shaderFile(REPROJECTION_FS_FILE);
            shaderFile.open(QIODevice::ReadOnly);
            QString fragment = shaderFile.readAll();
            compileProgram(_currentProgram, REPROJECTION_VS, fragment.toLocal8Bit().data());
        } catch (const std::runtime_error& error) {
            qDebug() << "Failed to build: " << error.what();
        }
        if (!_currentProgram) {
            _currentProgram = loadDefaultShader();
        }
    }
    return _currentProgram;
}
#endif


static const char * LASER_VS = R"VS(#version 410 core
uniform mat4 mvp = mat4(1);

in vec3 Position;

out vec3 vPosition;

void main() {
  gl_Position = mvp * vec4(Position, 1);
  vPosition = Position;
}

)VS";

static const char * LASER_FS = R"FS(#version 410 core

uniform vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
in vec3 vPosition;

out vec4 FragColor;

void main() {
    FragColor = color;
}

)FS";

void HmdDisplayPlugin::customizeContext() {
    Parent::customizeContext();
    // Only enable mirroring if we know vsync is disabled
    enableVsync(false);
    _enablePreview = !isVsyncEnabled();
    _sphereSection = loadSphereSection(_program, CompositorHelper::VIRTUAL_UI_TARGET_FOV.y, CompositorHelper::VIRTUAL_UI_ASPECT_RATIO);
    compileProgram(_laserProgram, LASER_VS, LASER_FS);
    _laserGeometry = loadLaser(_laserProgram);
    compileProgram(_reprojectionProgram, REPROJECTION_VS, REPROJECTION_FS);

    using namespace oglplus;
    REPROJECTION_MATRIX_LOCATION = Uniform<glm::mat3>(*_reprojectionProgram, "reprojection").Location();
    INVERSE_PROJECTION_MATRIX_LOCATION = Uniform<glm::mat4>(*_reprojectionProgram, "inverseProjections").Location();
    PROJECTION_MATRIX_LOCATION = Uniform<glm::mat4>(*_reprojectionProgram, "projections").Location();
}

void HmdDisplayPlugin::uncustomizeContext() {
    _sphereSection.reset();
    _compositeFramebuffer.reset();
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
    Uniform<glm::mat3>(*_reprojectionProgram, REPROJECTION_MATRIX_LOCATION).Set(_currentPresentFrameInfo.presentReprojection);
    //Uniform<glm::mat4>(*_reprojectionProgram, PROJECTION_MATRIX_LOCATION).Set(_eyeProjections);
    //Uniform<glm::mat4>(*_reprojectionProgram, INVERSE_PROJECTION_MATRIX_LOCATION).Set(_eyeInverseProjections);
    // FIXME what's the right oglplus mechanism to do this?  It's not that ^^^ ... better yet, switch to a uniform buffer
    glUniformMatrix4fv(INVERSE_PROJECTION_MATRIX_LOCATION, 2, GL_FALSE, &(_eyeInverseProjections[0][0][0]));
    glUniformMatrix4fv(PROJECTION_MATRIX_LOCATION, 2, GL_FALSE, &(_eyeProjections[0][0][0]));
    _plane->UseInProgram(*_reprojectionProgram);
    _plane->Draw();
}

void HmdDisplayPlugin::compositeOverlay() {
    using namespace oglplus;
    auto compositorHelper = DependencyManager::get<CompositorHelper>();
    glm::mat4 modelMat = compositorHelper->getModelTransform().getMatrix();

    useProgram(_program);
    // set the alpha
    Uniform<float>(*_program, _alphaUniform).Set(_compositeOverlayAlpha);
    _sphereSection->Use();
    for_each_eye([&](Eye eye) {
        eyeViewport(eye);
        auto modelView = glm::inverse(_currentPresentFrameInfo.presentPose * getEyeToHeadTransform(eye)) * modelMat;
        auto mvp = _eyeProjections[eye] * modelView;
        Uniform<glm::mat4>(*_program, _mvpUniform).Set(mvp);
        _sphereSection->Draw();
    });
    // restore the alpha
    Uniform<float>(*_program, _alphaUniform).Set(1.0);
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
    if (_enablePreview) {
        auto window = _container->getPrimaryWidget();
        auto windowSize = toGlm(window->size());
        float windowAspect = aspect(windowSize);
        float sceneAspect = aspect(_renderTargetSize);
        if (_monoPreview) {
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
    const int NUMBER_OF_HANDS = 2;
    std::array<HandLaserInfo, NUMBER_OF_HANDS> handLasers;
    std::array<mat4, NUMBER_OF_HANDS> renderHandPoses;
    Transform uiModelTransform;
    withPresentThreadLock([&] {
        handLasers = _handLasers;
        renderHandPoses = _handPoses;
        uiModelTransform = _uiModelTransform;
    });

    // If neither hand laser is activated, exit
    if (!handLasers[0].valid() && !handLasers[1].valid()) {
        return;
    }

    static const glm::mat4 identity;
    if (renderHandPoses[0] == identity && renderHandPoses[1] == identity) {
        return;
    }

    // Render hand lasers
    using namespace oglplus;
    useProgram(_laserProgram);
    _laserGeometry->Use();
    std::array<mat4, NUMBER_OF_HANDS> handLaserModelMatrices;

    for (int i = 0; i < NUMBER_OF_HANDS; ++i) {
        if (renderHandPoses[i] == identity) {
            continue;
        }
        const auto& handLaser = handLasers[i];
        if (!handLaser.valid()) {
            continue;
        }

        const auto& laserDirection = handLaser.direction;
        auto model = renderHandPoses[i];
        auto castDirection = glm::quat_cast(model) * laserDirection;
        if (glm::abs(glm::length2(castDirection) - 1.0f) > EPSILON) {
            castDirection = glm::normalize(castDirection);
        }

        // FIXME fetch the actual UI radius from... somewhere?
        float uiRadius = 1.0f;

        // Find the intersection of the laser with he UI and use it to scale the model matrix
        float distance; 
        if (!glm::intersectRaySphere(vec3(renderHandPoses[i][3]), castDirection, uiModelTransform.getTranslation(), uiRadius * uiRadius, distance)) {
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

    for_each_eye([&](Eye eye) {
        eyeViewport(eye);
        auto eyePose = _currentPresentFrameInfo.presentPose * getEyeToHeadTransform(eye);
        auto view = glm::inverse(eyePose);
        const auto& projection = _eyeProjections[eye];
        for (int i = 0; i < NUMBER_OF_HANDS; ++i) {
            if (handLaserModelMatrices[i] == identity) {
                continue;
            }
            Uniform<glm::mat4>(*_laserProgram, "mvp").Set(projection * view * handLaserModelMatrices[i]);
            Uniform<glm::vec4>(*_laserProgram, "color").Set(handLasers[i].color);
            _laserGeometry->Draw();
            // TODO render some kind of visual indicator at the intersection point with the UI.
        }
    });
}
