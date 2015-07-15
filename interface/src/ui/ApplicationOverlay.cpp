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
#include <gpu/GLBackendShared.h>
#include <GLMHelpers.h>
#include <OffscreenUi.h>
#include <CursorManager.h>
#include <PerfStat.h>

#include "AudioClient.h"
#include "audio/AudioScope.h"
#include "Application.h"
#include "ApplicationOverlay.h"

#include "Util.h"
#include "ui/Stats.h"
#include "ui/AvatarInputs.h"

const vec4 CONNECTION_STATUS_BORDER_COLOR{ 1.0f, 0.0f, 0.0f, 0.8f };
const float CONNECTION_STATUS_BORDER_LINE_WIDTH = 4.0f;
static const float ORTHO_NEAR_CLIP = -10000;
static const float ORTHO_FAR_CLIP = 10000;

// TODO move somewhere useful
static void fboViewport(QOpenGLFramebufferObject* fbo) {
    auto size = fbo->size();
    glViewport(0, 0, size.width(), size.height());
}

ApplicationOverlay::ApplicationOverlay()
{
    auto geometryCache = DependencyManager::get<GeometryCache>();
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
    PROFILE_RANGE(__FUNCTION__);
    CHECK_GL_ERROR();
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), "ApplicationOverlay::displayOverlay()");

    // TODO move to Application::idle()?
    Stats::getInstance()->updateStats();
    AvatarInputs::getInstance()->update();

    buildFramebufferObject();

    // Execute the batch into our framebuffer
    _overlayFramebuffer->bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    fboViewport(_overlayFramebuffer);

    // Now render the overlay components together into a single texture
    renderOverlays(renderArgs);
    renderStatsAndLogs(renderArgs);
    renderDomainConnectionStatusBorder(renderArgs);
    renderQmlUi(renderArgs);
    _overlayFramebuffer->release();
    CHECK_GL_ERROR();
}

void ApplicationOverlay::renderQmlUi(RenderArgs* renderArgs) {
    PROFILE_RANGE(__FUNCTION__);
    if (_uiTexture) {
        gpu::Batch batch;
        auto geometryCache = DependencyManager::get<GeometryCache>();
        geometryCache->useSimpleDrawPipeline(batch);
        batch.setProjectionTransform(mat4());
        batch.setModelTransform(mat4());
        batch._glBindTexture(GL_TEXTURE_2D, _uiTexture);
        geometryCache->renderUnitQuad(batch, glm::vec4(1));
        renderArgs->_context->syncCache();
        renderArgs->_context->render(batch);
    }
}

void ApplicationOverlay::renderOverlays(RenderArgs* renderArgs) {
    PROFILE_RANGE(__FUNCTION__);
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
    glUseProgram(0);

    // give external parties a change to hook in
    emit qApp->renderingOverlay();
    qApp->getOverlays().renderHUD(renderArgs);

    DependencyManager::get<AudioScope>()->render(renderArgs, _overlayFramebuffer->size().width(), _overlayFramebuffer->size().height());

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    fboViewport(_overlayFramebuffer);
}

void ApplicationOverlay::renderRearViewToFbo(RenderArgs* renderArgs) {
}

void ApplicationOverlay::renderRearView(RenderArgs* renderArgs) {
}

void ApplicationOverlay::renderStatsAndLogs(RenderArgs* renderArgs) {
    //  Display stats and log text onscreen

    // Determine whether to compute timing details

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
    */
}

void ApplicationOverlay::renderDomainConnectionStatusBorder(RenderArgs* renderArgs) {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    static std::once_flag once;
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
        batch.setProjectionTransform(mat4());
        batch.setModelTransform(mat4());
        batch.setResourceTexture(0, DependencyManager::get<TextureCache>()->getWhiteTexture());
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
    PROFILE_RANGE(__FUNCTION__);
    QSize fboSize = qApp->getDeviceSize();
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
