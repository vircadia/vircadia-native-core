//
//  ApplicationOverlay.cpp
//  interface/src/ui/overlays
//
//  Created by Benjamin Arnold on 5/27/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InterfaceConfig.h"

#include <QOpenGLFramebufferObject>
#include <QOpenGLTexture>

#include <glm/gtc/type_ptr.hpp>

#include <avatar/AvatarManager.h>
#include <DeferredLightingEffect.h>
#include <GLMHelpers.h>
#include <gpu/GLBackend.h>
#include <GLMHelpers.h>
#include <OffscreenUi.h>
#include <CursorManager.h>
#include <PerfStat.h>

#include "AudioClient.h"
#include "audio/AudioIOStatsRenderer.h"
#include "audio/AudioScope.h"
#include "audio/AudioToolBox.h"
#include "Application.h"
#include "ApplicationOverlay.h"
#include "devices/CameraToolBox.h"

#include "Util.h"
#include "ui/Stats.h"

const float WHITE_TEXT[] = { 0.93f, 0.93f, 0.93f };
const int AUDIO_METER_GAP = 5;
const int MUTE_ICON_PADDING = 10;
const vec4 CONNECTION_STATUS_BORDER_COLOR{ 1.0f, 0.0f, 0.0f, 0.8f };
const float CONNECTION_STATUS_BORDER_LINE_WIDTH = 4.0f;
static const int MIRROR_VIEW_TOP_PADDING = 5;
static const int MIRROR_VIEW_LEFT_PADDING = 10;
static const int MIRROR_VIEW_WIDTH = 265;
static const int MIRROR_VIEW_HEIGHT = 215;
static const int STATS_HORIZONTAL_OFFSET = MIRROR_VIEW_WIDTH + MIRROR_VIEW_LEFT_PADDING * 2;
static const float MIRROR_REARVIEW_DISTANCE = 0.722f;
static const float MIRROR_REARVIEW_BODY_DISTANCE = 2.56f;
static const float MIRROR_FIELD_OF_VIEW = 30.0f;
static const float ORTHO_NEAR_CLIP = -10000;
static const float ORTHO_FAR_CLIP = 10000;

// TODO move somewhere useful
static void fboViewport(QOpenGLFramebufferObject* fbo) {
    auto size = fbo->size();
    glViewport(0, 0, size.width(), size.height());
}



ApplicationOverlay::ApplicationOverlay() :
    _mirrorViewRect(QRect(MIRROR_VIEW_LEFT_PADDING, MIRROR_VIEW_TOP_PADDING, MIRROR_VIEW_WIDTH, MIRROR_VIEW_HEIGHT))
{
    auto geometryCache = DependencyManager::get<GeometryCache>();
    _audioRedQuad = geometryCache->allocateID();
    _audioGreenQuad = geometryCache->allocateID();
    _audioBlueQuad = geometryCache->allocateID();
    _domainStatusBorder = geometryCache->allocateID();
    _magnifierBorder = geometryCache->allocateID();
    
    // Once we move UI rendering and screen rendering to different
    // threads, we need to use a sync object to deteremine when
    // the current UI texture is no longer being read from, and only 
    // then release it back to the UI for re-use
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    connect(offscreenUi.data(), &OffscreenUi::textureUpdated, this, [&](GLuint textureId) {
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        offscreenUi->lockTexture(textureId);
        assert(!glGetError());
        std::swap(_uiTexture, textureId);
        if (textureId) {
            offscreenUi->releaseTexture(textureId);
        }
    });
}

ApplicationOverlay::~ApplicationOverlay() {
}

// Renders the overlays either to a texture or to the screen
void ApplicationOverlay::renderOverlay(RenderArgs* renderArgs) {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), "ApplicationOverlay::displayOverlay()");

    buildFramebufferObject();

    // First render the mirror to the mirror FBO
    renderRearViewToFbo(renderArgs);

    // Execute the batch into our framebuffer
    _overlayFramebuffer->bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    fboViewport(_overlayFramebuffer);

    // Now render the overlay components together into a single texture
    //renderOverlays(renderArgs);
    //renderAudioMeter(renderArgs);
    //renderCameraToggle(renderArgs);
    renderStatsAndLogs(renderArgs);
    renderRearView(renderArgs);

    renderDomainConnectionStatusBorder(renderArgs);
    renderQmlUi(renderArgs);

    _overlayFramebuffer->release();
}

void ApplicationOverlay::renderQmlUi(RenderArgs* renderArgs) {
    if (_uiTexture) {
        gpu::Batch batch;
        auto geometryCache = DependencyManager::get<GeometryCache>();
        geometryCache->useSimpleDrawPipeline(batch);
        batch._glDisable(GL_DEPTH_TEST);
        batch._glDisable(GL_LIGHTING);
        batch._glEnable(GL_BLEND);
        batch.setProjectionTransform(mat4());
        batch.setModelTransform(mat4());
        batch._glBindTexture(GL_TEXTURE_2D, _uiTexture);
        geometryCache->renderUnitQuad(batch, glm::vec4(1));
        renderArgs->_context->syncCache();
        renderArgs->_context->render(batch);
    }
}

void ApplicationOverlay::renderOverlays(RenderArgs* renderArgs) {
    glm::vec2 size = qApp->getCanvasSize();
    mat4 legacyProjection = glm::ortho<float>(0, size.x, size.y, 0, ORTHO_NEAR_CLIP, ORTHO_FAR_CLIP);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixf(glm::value_ptr(legacyProjection));
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // give external parties a change to hook in
    emit qApp->renderingOverlay();
    qApp->getOverlays().renderHUD(renderArgs);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void ApplicationOverlay::renderCameraToggle(RenderArgs* renderArgs) {
    if (Menu::getInstance()->isOptionChecked(MenuOption::NoFaceTracking)) {
        return;
    }

    int audioMeterY;
    bool smallMirrorVisible = Menu::getInstance()->isOptionChecked(MenuOption::Mirror) && !qApp->isHMDMode();
    bool boxed = smallMirrorVisible &&
        !Menu::getInstance()->isOptionChecked(MenuOption::FullscreenMirror);
    if (boxed) {
        audioMeterY = MIRROR_VIEW_HEIGHT + AUDIO_METER_GAP + MUTE_ICON_PADDING;
    } else {
        audioMeterY = AUDIO_METER_GAP + MUTE_ICON_PADDING;
    }

    DependencyManager::get<CameraToolBox>()->render(MIRROR_VIEW_LEFT_PADDING + AUDIO_METER_GAP, audioMeterY, boxed);
}

void ApplicationOverlay::renderAudioMeter(RenderArgs* renderArgs) {
    auto audio = DependencyManager::get<AudioClient>();

    //  Audio VU Meter and Mute Icon
    const int MUTE_ICON_SIZE = 24;
    const int AUDIO_METER_HEIGHT = 8;
    const int INTER_ICON_GAP = 2;

    int cameraSpace = 0;
    int audioMeterWidth = MIRROR_VIEW_WIDTH - MUTE_ICON_SIZE - MUTE_ICON_PADDING;
    int audioMeterScaleWidth = audioMeterWidth - 2;
    int audioMeterX = MIRROR_VIEW_LEFT_PADDING + MUTE_ICON_SIZE + AUDIO_METER_GAP;
    if (!Menu::getInstance()->isOptionChecked(MenuOption::NoFaceTracking)) {
        cameraSpace = MUTE_ICON_SIZE + INTER_ICON_GAP;
        audioMeterWidth -= cameraSpace;
        audioMeterScaleWidth -= cameraSpace;
        audioMeterX += cameraSpace;
    }

    int audioMeterY;
    bool smallMirrorVisible = Menu::getInstance()->isOptionChecked(MenuOption::Mirror) && !qApp->isHMDMode();
    bool boxed = smallMirrorVisible &&
        !Menu::getInstance()->isOptionChecked(MenuOption::FullscreenMirror);
    if (boxed) {
        audioMeterY = MIRROR_VIEW_HEIGHT + AUDIO_METER_GAP + MUTE_ICON_PADDING;
    } else {
        audioMeterY = AUDIO_METER_GAP + MUTE_ICON_PADDING;
    }

    const glm::vec4 AUDIO_METER_BLUE = { 0.0, 0.0, 1.0, 1.0 };
    const glm::vec4 AUDIO_METER_GREEN = { 0.0, 1.0, 0.0, 1.0 };
    const glm::vec4 AUDIO_METER_RED = { 1.0, 0.0, 0.0, 1.0 };
    const float CLIPPING_INDICATOR_TIME = 1.0f;
    const float AUDIO_METER_AVERAGING = 0.5;
    const float LOG2 = log(2.0f);
    const float METER_LOUDNESS_SCALE = 2.8f / 5.0f;
    const float LOG2_LOUDNESS_FLOOR = 11.0f;
    float audioGreenStart = 0.25f * audioMeterScaleWidth;
    float audioRedStart = 0.8f * audioMeterScaleWidth;
    float audioLevel = 0.0f;
    float loudness = audio->getLastInputLoudness() + 1.0f;

    _trailingAudioLoudness = AUDIO_METER_AVERAGING * _trailingAudioLoudness + (1.0f - AUDIO_METER_AVERAGING) * loudness;
    float log2loudness = log(_trailingAudioLoudness) / LOG2;

    if (log2loudness <= LOG2_LOUDNESS_FLOOR) {
        audioLevel = (log2loudness / LOG2_LOUDNESS_FLOOR) * METER_LOUDNESS_SCALE * audioMeterScaleWidth;
    } else {
        audioLevel = (log2loudness - (LOG2_LOUDNESS_FLOOR - 1.0f)) * METER_LOUDNESS_SCALE * audioMeterScaleWidth;
    }
    if (audioLevel > audioMeterScaleWidth) {
        audioLevel = audioMeterScaleWidth;
    }
    bool isClipping = ((audio->getTimeSinceLastClip() > 0.0f) && (audio->getTimeSinceLastClip() < CLIPPING_INDICATOR_TIME));

    DependencyManager::get<AudioToolBox>()->render(MIRROR_VIEW_LEFT_PADDING + AUDIO_METER_GAP, audioMeterY, cameraSpace, boxed);
    
    auto canvasSize = qApp->getCanvasSize();
    DependencyManager::get<AudioScope>()->render(canvasSize.x, canvasSize.y);
    DependencyManager::get<AudioIOStatsRenderer>()->render(WHITE_TEXT, canvasSize.x, canvasSize.y);

    audioMeterY += AUDIO_METER_HEIGHT;

    //  Draw audio meter background Quad
    DependencyManager::get<GeometryCache>()->renderQuad(audioMeterX, audioMeterY, audioMeterWidth, AUDIO_METER_HEIGHT,
                                                            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

    if (audioLevel > audioRedStart) {
        glm::vec4 quadColor;
        if (!isClipping) {
            quadColor = AUDIO_METER_RED;
        } else {
            quadColor = glm::vec4(1, 1, 1, 1);
        }
        // Draw Red Quad
        DependencyManager::get<GeometryCache>()->renderQuad(audioMeterX + audioRedStart,
                                                            audioMeterY, 
                                                            audioLevel - audioRedStart, 
                                                            AUDIO_METER_HEIGHT, quadColor,
                                                            _audioRedQuad);
        
        audioLevel = audioRedStart;
    }

    if (audioLevel > audioGreenStart) {
        glm::vec4 quadColor;
        if (!isClipping) {
            quadColor = AUDIO_METER_GREEN;
        } else {
            quadColor = glm::vec4(1, 1, 1, 1);
        }
        // Draw Green Quad
        DependencyManager::get<GeometryCache>()->renderQuad(audioMeterX + audioGreenStart,
                                                            audioMeterY, 
                                                            audioLevel - audioGreenStart, 
                                                            AUDIO_METER_HEIGHT, quadColor,
                                                            _audioGreenQuad);

        audioLevel = audioGreenStart;
    }

    if (audioLevel >= 0) {
        glm::vec4 quadColor;
        if (!isClipping) {
            quadColor = AUDIO_METER_BLUE;
        } else {
            quadColor = glm::vec4(1, 1, 1, 1);
        }
        // Draw Blue (low level) quad
        DependencyManager::get<GeometryCache>()->renderQuad(audioMeterX,
                                                            audioMeterY,
                                                            audioLevel, AUDIO_METER_HEIGHT, quadColor,
                                                            _audioBlueQuad);
    }
}

void ApplicationOverlay::renderRearViewToFbo(RenderArgs* renderArgs) {
    // Grab current viewport to reset it at the end
    auto mirrorSize = _mirrorFramebuffer->size();
    float aspect = (float)mirrorSize.width() / mirrorSize.height();
    float fov = MIRROR_FIELD_OF_VIEW;
    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    // bool eyeRelativeCamera = false;
    bool billboard = false;
    if (billboard) {
        fov = BILLBOARD_FIELD_OF_VIEW;  // degees
        _mirrorCamera.setPosition(myAvatar->getPosition() +
            myAvatar->getOrientation() * glm::vec3(0.0f, 0.0f, -1.0f) * BILLBOARD_DISTANCE * myAvatar->getScale());

    } else if (RearMirrorTools::rearViewZoomLevel.get() == BODY) {
        _mirrorCamera.setPosition(myAvatar->getChestPosition() +
            myAvatar->getOrientation() * glm::vec3(0.0f, 0.0f, -1.0f) * MIRROR_REARVIEW_BODY_DISTANCE * myAvatar->getScale());

    } else { // HEAD zoom level
        // FIXME note that the positioing of the camera relative to the avatar can suffer limited
        // precision as the user's position moves further away from the origin.  Thus at
        // /1e7,1e7,1e7 (well outside the buildable volume) the mirror camera veers and sways
        // wildly as you rotate your avatar because the floating point values are becoming
        // larger, squeezing out the available digits of precision you have available at the
        // human scale for camera positioning.

        // Previously there was a hack to correct this using the mechanism of repositioning
        // the avatar at the origin of the world for the purposes of rendering the mirror,
        // but it resulted in failing to render the avatar's head model in the mirror view
        // when in first person mode.  Presumably this was because of some missed culling logic
        // that was not accounted for in the hack.

        // This was removed in commit 71e59cfa88c6563749594e25494102fe01db38e9 but could be further
        // investigated in order to adapt the technique while fixing the head rendering issue,
        // but the complexity of the hack suggests that a better approach
        _mirrorCamera.setPosition(myAvatar->getHead()->getEyePosition() +
            myAvatar->getOrientation() * glm::vec3(0.0f, 0.0f, -1.0f) * MIRROR_REARVIEW_DISTANCE * myAvatar->getScale());
    }
    _mirrorCamera.setProjection(glm::perspective(glm::radians(fov), aspect, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP));
    _mirrorCamera.setRotation(myAvatar->getWorldAlignedOrientation() * glm::quat(glm::vec3(0.0f, PI, 0.0f)));

    // set the bounds of rear mirror view
    if (billboard) {
        //QSize size = DependencyManager::get<TextureCache>()->getFrameBufferSize();
        //glViewport(region.x(), size.height() - region.y() - region.height(), region.width(), region.height());
        //glScissor(region.x(), size.height() - region.y() - region.height(), region.width(), region.height());
    } else {
        auto mirrorSize = _mirrorFramebuffer->size();
        fboViewport(_mirrorFramebuffer);
    }

    _mirrorFramebuffer->bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glLoadMatrixf(glm::value_ptr(_mirrorCamera.getProjection()));
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glLoadMatrixf(glm::value_ptr(glm::mat4_cast(_mirrorCamera.getOrientation()) * glm::translate(glm::mat4(), _mirrorCamera.getPosition())));
    {
        renderArgs->_context->syncCache();
        qApp->displaySide(renderArgs, _mirrorCamera, true, billboard);
    }
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    _mirrorFramebuffer->release();

    //    if (!billboard) {
    //        _rearMirrorTools->render(renderArgs, false, _glWidget->mapFromGlobal(QCursor::pos()));
    //    }

    //    // reset Viewport and projection matrix
    //    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    //    glDisable(GL_SCISSOR_TEST);
    //    updateProjectionMatrix(_myCamera, updateViewFrustum);
    //}
}

void ApplicationOverlay::renderRearView(RenderArgs* renderArgs) {
    if (_mirrorFramebuffer && _mirrorFramebuffer->texture()) {
        gpu::Batch batch;
        auto geometryCache = DependencyManager::get<GeometryCache>();
        geometryCache->useSimpleDrawPipeline(batch);
        batch._glDisable(GL_DEPTH_TEST);
        batch._glDisable(GL_LIGHTING);
        batch._glEnable(GL_BLEND);
        batch.setProjectionTransform(mat4());
        batch.setModelTransform(mat4());
        auto textureCache = DependencyManager::get<TextureCache>();
        batch.setUniformTexture(0, textureCache->getBlueTexture());
        geometryCache->renderUnitQuad(batch, glm::vec4(1));
        batch._glBindTexture(GL_TEXTURE_2D, _mirrorFramebuffer->texture());
        geometryCache->renderUnitQuad(batch, glm::vec4(1));
        renderArgs->_context->syncCache();
        auto overlaySize = _overlayFramebuffer->size();
        glViewport(MIRROR_VIEW_LEFT_PADDING, overlaySize.height() - MIRROR_VIEW_TOP_PADDING - MIRROR_VIEW_HEIGHT,
            MIRROR_VIEW_WIDTH, MIRROR_VIEW_HEIGHT);
        renderArgs->_context->render(batch);
        fboViewport(_overlayFramebuffer);
    }
}

void ApplicationOverlay::renderStatsAndLogs(RenderArgs* renderArgs) {

    //  Display stats and log text onscreen

    // Determine whether to compute timing details
    Stats::getInstance()->updateStats();

    /*
    //  Show on-screen msec timer
    if (Menu::getInstance()->isOptionChecked(MenuOption::FrameTimer)) {
        auto canvasSize = qApp->getCanvasSize();
        quint64 mSecsNow = floor(usecTimestampNow() / 1000.0 + 0.5);
        QString frameTimer = QString("%1\n").arg((int)(mSecsNow % 1000));
        int timerBottom =
            (Menu::getInstance()->isOptionChecked(MenuOption::Stats))
            ? 80 : 20;
        drawText(canvasSize.x - 100, canvasSize.y - timerBottom,
            0.30f, 0.0f, 0, frameTimer.toUtf8().constData(), WHITE_TEXT);
    }

    glPointSize(1.0f);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    NodeBounds& nodeBoundsDisplay = qApp->getNodeBoundsDisplay();
    nodeBoundsDisplay.drawOverlay();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);
    */
}

void ApplicationOverlay::renderDomainConnectionStatusBorder(RenderArgs* renderArgs) {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    std::once_flag once;
    std::call_once(once, [&] {
        QVector<vec2> points;
        static const float B = 0.99f;
        points.push_back(vec2(-B));
        points.push_back(vec2(B, -B));
        points.push_back(vec2(B));
        points.push_back(vec2(-B, B));
        points.push_back(vec2(-B));
        geometryCache->updateVertices(_domainStatusBorder, points, CONNECTION_STATUS_BORDER_COLOR);
    });
    auto nodeList = DependencyManager::get<NodeList>();
    if (nodeList && !nodeList->getDomainHandler().isConnected()) {
        gpu::Batch batch;
        auto geometryCache = DependencyManager::get<GeometryCache>();
        geometryCache->useSimpleDrawPipeline(batch);
        batch._glDisable(GL_DEPTH);
        batch._glDisable(GL_LIGHTING);
        batch._glEnable(GL_BLEND);
        batch.setProjectionTransform(mat4());
        batch.setModelTransform(mat4());
        batch.setUniformTexture(0, DependencyManager::get<TextureCache>()->getWhiteTexture());
        batch._glLineWidth(CONNECTION_STATUS_BORDER_LINE_WIDTH);

        // TODO animate the disconnect border for some excitement while not connected?
        //double usecs = usecTimestampNow();
        //double secs = usecs / 1000000.0;
        //float scaleAmount = 1.0f + (0.01f * sin(secs * 5.0f));
        //batch.setModelTransform(glm::scale(mat4(), vec3(scaleAmount)));

        geometryCache->renderVertices(batch, gpu::LINE_STRIP, _domainStatusBorder);
        renderArgs->_context->syncCache();
        renderArgs->_context->render(batch);
    }
}

GLuint ApplicationOverlay::getOverlayTexture() {
    if (!_overlayFramebuffer) {
        return 0;
    }
    return _overlayFramebuffer->texture();
}

void ApplicationOverlay::buildFramebufferObject() {
    if (!_mirrorFramebuffer) {
        _mirrorFramebuffer = new QOpenGLFramebufferObject(QSize(MIRROR_VIEW_WIDTH, MIRROR_VIEW_HEIGHT), QOpenGLFramebufferObject::Depth);
        glBindTexture(GL_TEXTURE_2D, _mirrorFramebuffer->texture());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        GLfloat borderColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    auto canvasSize = qApp->getCanvasSize();
    QSize fboSize = QSize(canvasSize.x, canvasSize.y);
    if (_overlayFramebuffer && fboSize == _overlayFramebuffer->size()) {
        // Already built
        return;
    }
    
    if (_overlayFramebuffer) {
        delete _overlayFramebuffer;
    }
    
    _overlayFramebuffer = new QOpenGLFramebufferObject(fboSize, QOpenGLFramebufferObject::Depth);
    glBindTexture(GL_TEXTURE_2D, getOverlayTexture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    GLfloat borderColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindTexture(GL_TEXTURE_2D, 0);
}

