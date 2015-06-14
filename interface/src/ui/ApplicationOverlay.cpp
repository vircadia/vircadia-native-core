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
const float CONNECTION_STATUS_BORDER_COLOR[] = { 1.0f, 0.0f, 0.0f };
const float CONNECTION_STATUS_BORDER_LINE_WIDTH = 4.0f;

ApplicationOverlay::ApplicationOverlay() :
    _alpha(1.0f),
    _trailingAudioLoudness(0.0f),
    _previousBorderWidth(-1),
    _previousBorderHeight(-1),
    _framebufferObject(nullptr)
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

    Overlays& overlays = qApp->getOverlays();
    glm::vec2 size = qApp->getCanvasSize();
    // TODO Handle fading and deactivation/activation of UI
    
    // Render 2D overlay
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    buildFramebufferObject();

    _framebufferObject->bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, size.x, size.y);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); {
        const float NEAR_CLIP = -10000;
        const float FAR_CLIP = 10000;
        glLoadIdentity();
        glOrtho(0, size.x, size.y, 0, NEAR_CLIP, FAR_CLIP);

        glMatrixMode(GL_MODELVIEW);

        renderAudioMeter();
        renderCameraToggle();

        renderStatsAndLogs();

        // give external parties a change to hook in
        emit qApp->renderingOverlay();

        overlays.renderHUD(renderArgs);

        renderDomainConnectionStatusBorder();
        if (_uiTexture) {
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            glEnable(GL_TEXTURE_2D);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glBindTexture(GL_TEXTURE_2D, _uiTexture);
            DependencyManager::get<GeometryCache>()->renderUnitQuad();
            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_BLEND);
        }
        glLoadIdentity();


        glMatrixMode(GL_PROJECTION);
    } glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);

    _framebufferObject->release();
}

void ApplicationOverlay::renderCameraToggle() {
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

void ApplicationOverlay::renderAudioMeter() {
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

void ApplicationOverlay::renderStatsAndLogs() {
    Application* application = Application::getInstance();
    QSharedPointer<BandwidthRecorder> bandwidthRecorder = DependencyManager::get<BandwidthRecorder>();
    
    const OctreePacketProcessor& octreePacketProcessor = application->getOctreePacketProcessor();
    NodeBounds& nodeBoundsDisplay = application->getNodeBoundsDisplay();

    //  Display stats and log text onscreen
    glLineWidth(1.0f);
    glPointSize(1.0f);

    // Determine whether to compute timing details
    bool shouldDisplayTimingDetail = Menu::getInstance()->isOptionChecked(MenuOption::DisplayDebugTimingDetails) &&
                                     Menu::getInstance()->isOptionChecked(MenuOption::Stats) &&
                                     Stats::getInstance()->isExpanded();
    if (shouldDisplayTimingDetail != PerformanceTimer::isActive()) {
        PerformanceTimer::setActive(shouldDisplayTimingDetail);
    }
    
    if (Menu::getInstance()->isOptionChecked(MenuOption::Stats)) {
        // let's set horizontal offset to give stats some margin to mirror
        int horizontalOffset = MIRROR_VIEW_WIDTH + MIRROR_VIEW_LEFT_PADDING * 2;
        int voxelPacketsToProcess = octreePacketProcessor.packetsToProcessCount();
        //  Onscreen text about position, servers, etc
        Stats::getInstance()->display(WHITE_TEXT, horizontalOffset, application->getFps(),
                                      bandwidthRecorder->getCachedTotalAverageInputPacketsPerSecond(),
                                      bandwidthRecorder->getCachedTotalAverageOutputPacketsPerSecond(),
                                      bandwidthRecorder->getCachedTotalAverageInputKilobitsPerSecond(),
                                      bandwidthRecorder->getCachedTotalAverageOutputKilobitsPerSecond(),
                                      voxelPacketsToProcess);
    }

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
    nodeBoundsDisplay.drawOverlay();
}

void ApplicationOverlay::renderDomainConnectionStatusBorder() {
    auto nodeList = DependencyManager::get<NodeList>();

    if (nodeList && !nodeList->getDomainHandler().isConnected()) {
        auto geometryCache = DependencyManager::get<GeometryCache>();
        auto canvasSize = qApp->getCanvasSize();
        if ((int)canvasSize.x != _previousBorderWidth || (int)canvasSize.y != _previousBorderHeight) {
            glm::vec4 color(CONNECTION_STATUS_BORDER_COLOR[0],
                            CONNECTION_STATUS_BORDER_COLOR[1],
                            CONNECTION_STATUS_BORDER_COLOR[2], 1.0f);

            QVector<glm::vec2> border;
            border << glm::vec2(0, 0);
            border << glm::vec2(0, canvasSize.y);
            border << glm::vec2(canvasSize.x, canvasSize.y);
            border << glm::vec2(canvasSize.x, 0);
            border << glm::vec2(0, 0);
            geometryCache->updateVertices(_domainStatusBorder, border, color);
            _previousBorderWidth = canvasSize.x;
            _previousBorderHeight = canvasSize.y;
        }

        glLineWidth(CONNECTION_STATUS_BORDER_LINE_WIDTH);

        geometryCache->renderVertices(gpu::LINE_STRIP, _domainStatusBorder);
    }
}

GLuint ApplicationOverlay::getOverlayTexture() {
    if (!_framebufferObject) {
        return 0;
    }
    return _framebufferObject->texture();
}

void ApplicationOverlay::buildFramebufferObject() {
    auto canvasSize = qApp->getCanvasSize();
    QSize fboSize = QSize(canvasSize.x, canvasSize.y);
    if (_framebufferObject != NULL && fboSize == _framebufferObject->size()) {
        // Already built
        return;
    }
    
    if (_framebufferObject != NULL) {
        delete _framebufferObject;
    }
    
    _framebufferObject = new QOpenGLFramebufferObject(fboSize, QOpenGLFramebufferObject::Depth);
    glBindTexture(GL_TEXTURE_2D, getOverlayTexture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    GLfloat borderColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindTexture(GL_TEXTURE_2D, 0);
}

