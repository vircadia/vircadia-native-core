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

#include <glm/gtc/type_ptr.hpp>

#include <avatar/AvatarManager.h>
#include <GLMHelpers.h>
#include <FramebufferCache.h>
#include <GLMHelpers.h>
#include <OffscreenUi.h>
#include <CursorManager.h>
#include <PerfStat.h>
#include <gl/Config.h>

#include "AudioClient.h"
#include "audio/AudioScope.h"
#include "Application.h"
#include "ApplicationOverlay.h"

#include "Util.h"
#include "ui/Stats.h"
#include "ui/AvatarInputs.h"
#include "OffscreenUi.h"
#include <QQmlContext>

const vec4 CONNECTION_STATUS_BORDER_COLOR{ 1.0f, 0.0f, 0.0f, 0.8f };
static const float ORTHO_NEAR_CLIP = -1000.0f;
static const float ORTHO_FAR_CLIP = 1000.0f;

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
        _uiTexture = textureId;
    });
}

ApplicationOverlay::~ApplicationOverlay() {
}

// Renders the overlays either to a texture or to the screen
void ApplicationOverlay::renderOverlay(RenderArgs* renderArgs) {
    PROFILE_RANGE(__FUNCTION__);
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), "ApplicationOverlay::displayOverlay()");

    buildFramebufferObject();
    
    if (!_overlayFramebuffer) {
        return; // we can't do anything without our frame buffer.
    }

    // Execute the batch into our framebuffer
    doInBatch(renderArgs->_context, [&](gpu::Batch& batch) {
        PROFILE_RANGE_BATCH(batch, "ApplicationOverlayRender");
        renderArgs->_batch = &batch;
        batch.enableStereo(false);

        int width = _overlayFramebuffer->getWidth();
        int height = _overlayFramebuffer->getHeight();

        batch.setViewportTransform(glm::ivec4(0, 0, width, height));
        batch.setFramebuffer(_overlayFramebuffer);

        glm::vec4 color { 0.0f, 0.0f, 0.0f, 0.0f };
        float depth = 1.0f;
        int stencil = 0;
        batch.clearFramebuffer(gpu::Framebuffer::BUFFER_COLOR0 | gpu::Framebuffer::BUFFER_DEPTH, color, depth, stencil);

        // Now render the overlay components together into a single texture
        renderDomainConnectionStatusBorder(renderArgs); // renders the connected domain line
        renderAudioScope(renderArgs); // audio scope in the very back - NOTE: this is the debug audio scope, not the VU meter
        renderRearView(renderArgs); // renders the mirror view selfie
        renderOverlays(renderArgs); // renders Scripts Overlay and AudioScope
        renderQmlUi(renderArgs); // renders a unit quad with the QML UI texture, and the text overlays from scripts
        renderStatsAndLogs(renderArgs);  // currently renders nothing
    });

    renderArgs->_batch = nullptr; // so future users of renderArgs don't try to use our batch
}

void ApplicationOverlay::renderQmlUi(RenderArgs* renderArgs) {
    PROFILE_RANGE(__FUNCTION__);
    if (_uiTexture) {
        gpu::Batch& batch = *renderArgs->_batch;
        auto geometryCache = DependencyManager::get<GeometryCache>();

        geometryCache->useSimpleDrawPipeline(batch);
        batch.setProjectionTransform(mat4());
        batch.setModelTransform(Transform());
        batch.clearViewTransform();
        batch._glActiveBindTexture(GL_TEXTURE0, GL_TEXTURE_2D, _uiTexture);

        geometryCache->renderUnitQuad(batch, glm::vec4(1));
    }
}

void ApplicationOverlay::renderAudioScope(RenderArgs* renderArgs) {
    PROFILE_RANGE(__FUNCTION__);

    gpu::Batch& batch = *renderArgs->_batch;
    auto geometryCache = DependencyManager::get<GeometryCache>();
    geometryCache->useSimpleDrawPipeline(batch);
    auto textureCache = DependencyManager::get<TextureCache>();
    batch.setResourceTexture(0, textureCache->getWhiteTexture());
    int width = renderArgs->_viewport.z;
    int height = renderArgs->_viewport.w;
    mat4 legacyProjection = glm::ortho<float>(0, width, height, 0, ORTHO_NEAR_CLIP, ORTHO_FAR_CLIP);
    batch.setProjectionTransform(legacyProjection);
    batch.setModelTransform(Transform());
    batch.clearViewTransform();

    // Render the audio scope
    DependencyManager::get<AudioScope>()->render(renderArgs, width, height);
}

void ApplicationOverlay::renderOverlays(RenderArgs* renderArgs) {
    PROFILE_RANGE(__FUNCTION__);

    gpu::Batch& batch = *renderArgs->_batch;
    auto geometryCache = DependencyManager::get<GeometryCache>();
    geometryCache->useSimpleDrawPipeline(batch);
    auto textureCache = DependencyManager::get<TextureCache>();
    batch.setResourceTexture(0, textureCache->getWhiteTexture());
    int width = renderArgs->_viewport.z;
    int height = renderArgs->_viewport.w;
    mat4 legacyProjection = glm::ortho<float>(0, width, height, 0, ORTHO_NEAR_CLIP, ORTHO_FAR_CLIP);
    batch.setProjectionTransform(legacyProjection);
    batch.setModelTransform(Transform());
    batch.clearViewTransform();

    // Render all of the Script based "HUD" aka 2D overlays.
    // note: we call them HUD, as opposed to 2D, only because there are some cases of 3D HUD overlays, like the
    // cameral controls for the edit.js
    qApp->getOverlays().renderHUD(renderArgs);
}

void ApplicationOverlay::renderRearViewToFbo(RenderArgs* renderArgs) {
}

void ApplicationOverlay::renderRearView(RenderArgs* renderArgs) {
    if (!qApp->isHMDMode() && Menu::getInstance()->isOptionChecked(MenuOption::MiniMirror) &&
        !Menu::getInstance()->isOptionChecked(MenuOption::FullscreenMirror)) {
        gpu::Batch& batch = *renderArgs->_batch;

        auto geometryCache = DependencyManager::get<GeometryCache>();

        auto framebuffer = DependencyManager::get<FramebufferCache>();
        auto selfieTexture = framebuffer->getSelfieFramebuffer()->getRenderBuffer(0);

        int width = renderArgs->_viewport.z;
        int height = renderArgs->_viewport.w;
        mat4 legacyProjection = glm::ortho<float>(0, width, height, 0, ORTHO_NEAR_CLIP, ORTHO_FAR_CLIP);
        batch.setProjectionTransform(legacyProjection);
        batch.setModelTransform(Transform());
        batch.clearViewTransform();
        
        float screenRatio = ((float)qApp->getDevicePixelRatio());
        float renderRatio = ((float)qApp->getRenderResolutionScale());
        
        auto viewport = qApp->getMirrorViewRect();
        glm::vec2 bottomLeft(viewport.left(), viewport.top() + viewport.height());
        glm::vec2 topRight(viewport.left() + viewport.width(), viewport.top());
        bottomLeft *= screenRatio;
        topRight *= screenRatio;
        glm::vec2 texCoordMinCorner(0.0f, 0.0f);
        glm::vec2 texCoordMaxCorner(viewport.width() * renderRatio / float(selfieTexture->getWidth()), viewport.height() * renderRatio / float(selfieTexture->getHeight()));

        batch.setResourceTexture(0, selfieTexture);
        float alpha = DependencyManager::get<OffscreenUi>()->getDesktop()->property("unpinnedAlpha").toFloat();
        geometryCache->renderQuad(batch, bottomLeft, topRight, texCoordMinCorner, texCoordMaxCorner, glm::vec4(1.0f, 1.0f, 1.0f, alpha));

        batch.setResourceTexture(0, renderArgs->_whiteTexture);
    }
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
        gpu::Batch& batch = *renderArgs->_batch;
        auto geometryCache = DependencyManager::get<GeometryCache>();
        geometryCache->useSimpleDrawPipeline(batch);
        batch.setProjectionTransform(mat4());
        batch.setModelTransform(Transform());
        batch.clearViewTransform();
        batch.setResourceTexture(0, DependencyManager::get<TextureCache>()->getWhiteTexture());
        // FIXME: THe line width of CONNECTION_STATUS_BORDER_LINE_WIDTH is not supported anymore, we ll need a workaround

        // TODO animate the disconnect border for some excitement while not connected?
        //double usecs = usecTimestampNow();
        //double secs = usecs / 1000000.0;
        //float scaleAmount = 1.0f + (0.01f * sin(secs * 5.0f));
        //batch.setModelTransform(glm::scale(mat4(), vec3(scaleAmount)));

        geometryCache->renderVertices(batch, gpu::LINE_STRIP, _domainStatusBorder);
    }
}

static const auto COLOR_FORMAT = gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA);
static const auto DEFAULT_SAMPLER = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR);
static const auto DEPTH_FORMAT = gpu::Element(gpu::SCALAR, gpu::FLOAT, gpu::DEPTH);

void ApplicationOverlay::buildFramebufferObject() {
    PROFILE_RANGE(__FUNCTION__);

    auto uiSize = qApp->getUiSize();
    if (!_overlayFramebuffer || uiSize != _overlayFramebuffer->getSize()) {
        _overlayFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    }

    auto width = uiSize.x;
    auto height = uiSize.y;
    if (!_overlayFramebuffer->getDepthStencilBuffer()) {
        auto overlayDepthTexture = gpu::TexturePointer(gpu::Texture::create2D(DEPTH_FORMAT, width, height, DEFAULT_SAMPLER));
        _overlayFramebuffer->setDepthStencilBuffer(overlayDepthTexture, DEPTH_FORMAT);
    }

    if (!_overlayFramebuffer->getRenderBuffer(0)) {
        const gpu::Sampler OVERLAY_SAMPLER(gpu::Sampler::FILTER_MIN_MAG_LINEAR, gpu::Sampler::WRAP_CLAMP);
        auto colorBuffer = gpu::TexturePointer(gpu::Texture::create2D(COLOR_FORMAT, width, height, OVERLAY_SAMPLER));
        _overlayFramebuffer->setRenderBuffer(0, colorBuffer);
    }
}

gpu::TexturePointer ApplicationOverlay::getOverlayTexture() {
    if (!_overlayFramebuffer) {
        return gpu::TexturePointer();
    }
    return _overlayFramebuffer->getRenderBuffer(0);
}