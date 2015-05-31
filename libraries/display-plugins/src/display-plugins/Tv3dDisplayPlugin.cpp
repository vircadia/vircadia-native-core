//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Tv3dDisplayPlugin.h"

#include <QApplication>
#include <QDesktopWidget>

#include <GlWindow.h>
#include <ViewFrustum.h>
#include <MatrixStack.h>

#include <gpu/GLBackend.h>

const QString Tv3dDisplayPlugin::NAME("Tv3dDisplayPlugin");

const QString & Tv3dDisplayPlugin::getName() {
    return NAME;
}

Tv3dDisplayPlugin::Tv3dDisplayPlugin() {
}

bool Tv3dDisplayPlugin::isSupported() const {
    // FIXME this should attempt to do a scan for supported 3D output
    return true;
}

void Tv3dDisplayPlugin::customizeWindow(PluginContainer * container) {
    _window->setFlags(Qt::FramelessWindowHint);
    auto desktop = QApplication::desktop();
    QRect primaryGeometry = desktop->screenGeometry();
    for (int i = 0; i < desktop->screenCount(); ++i) {
        QRect geometry = desktop->screen(i)->geometry();
        if (geometry.topLeft() == primaryGeometry.topLeft()) {
            continue;
        }
        float aspect = (float)geometry.width() / (float)geometry.height();
        if (aspect < 1.0f) {
            continue;
        }
        _window->setGeometry(geometry);
        break;
    }
    _window->setCursor(Qt::BlankCursor);
    _window->show();
}

// FIXME make this into a setting that can be adjusted
const float DEFAULT_IPD = 0.064f;
const float HALF_DEFAULT_IPD = DEFAULT_IPD / 2.0f;

glm::mat4 Tv3dDisplayPlugin::getProjection(Eye eye, const glm::mat4& baseProjection) const {
    // Refer to http://www.nvidia.com/content/gtc-2010/pdfs/2010_gtc2010.pdf on creating 
    // stereo projection matrices.  Do NOT use "toe-in", use translation.

    float nearZ = DEFAULT_NEAR_CLIP; // near clipping plane
    float screenZ = 0.25f; // screen projection plane
    // FIXME verify this is the right calculation
    float frustumshift = HALF_DEFAULT_IPD * nearZ / screenZ;
    if (eye == Right) {
        frustumshift = -frustumshift;
    }
    return glm::translate(baseProjection, vec3(frustumshift, 0, 0));
}

glm::mat4 Tv3dDisplayPlugin::getModelview(Eye eye, const glm::mat4& baseModelview) const {
    float modelviewShift = HALF_DEFAULT_IPD;
    if (eye == Left) {
        modelviewShift = -modelviewShift;
    }
    return baseModelview * glm::translate(mat4(), vec3(modelviewShift, 0, 0));
}

template <typename F>
void sbs_for_each_eye(const uvec2& size, F f) {
    QRect r(QPoint(0, 0), QSize(size.x / 2, size.y));
    for_each_eye([&](Eye eye) {
        oglplus::Context::Viewport(r.x(), r.y(), r.width(), r.height());
        f(eye);
    }, [&] {
        r.moveLeft(r.width());
    });
}

void Tv3dDisplayPlugin::display(
    GLuint sceneTexture, const glm::uvec2& sceneSize,
    GLuint overlayTexture, const glm::uvec2& overlaySize) {
    makeCurrent();
    GLenum err = glGetError();
    uvec2 size = toGlm(getDeviceSize());
    err = glGetError();
    using namespace oglplus;
    err = glGetError();
    Context::Viewport(size.x, size.y);
    err = glGetError();
    Context::Clear().ColorBuffer().DepthBuffer();

    Mat4Uniform(*program, "ModelView").Set(mat4());
    Mat4Uniform(*program, "Projection").Set(mat4());

    glBindTexture(GL_TEXTURE_2D, sceneTexture);

    plane->Draw();

    // FIXME the 
    const float screenAspect = aspect(size);
    const GLfloat distance = 1.0f;
    const GLfloat halfQuadHeight = distance * tan(DEFAULT_FIELD_OF_VIEW_DEGREES);
    const GLfloat halfQuadWidth = halfQuadHeight * screenAspect;
    const GLfloat quadWidth = halfQuadWidth * 2.0f;
    const GLfloat quadHeight = halfQuadHeight * 2.0f;

    vec3 quadSize(quadWidth, quadHeight, 1.0f);
    quadSize = vec3(1.0f) / quadSize;

    using namespace oglplus;
    Context::Enable(Capability::Blend);
    glBindTexture(GL_TEXTURE_2D, overlayTexture);
    
    mat4 pr = glm::perspective(glm::radians(DEFAULT_FIELD_OF_VIEW_DEGREES), screenAspect, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP);
    Mat4Uniform(*program, "Projection").Set(pr);

    // Position the camera relative to the overlay texture
    MatrixStack mv;
    mv.translate(vec3(0, 0, -distance)).scale(vec3(0.7f, 0.7f / screenAspect, 1.0f)); // .scale(vec3(quadWidth, quadHeight, 1.0));
    sbs_for_each_eye(size, [&](Eye eye) {
        mv.withPush([&] {
            // translate 
            mv.top() = getModelview(eye, mv.top());
            Mat4Uniform(*program, "ModelView").Set(mv.top());
            plane->Draw();
        });
    });

    glBindTexture(GL_TEXTURE_2D, gpu::GLBackend::getTextureID(crosshairTexture));
    glm::vec2 canvasSize = getCanvasSize();
    glm::vec2 mouse = toGlm(_window->mapFromGlobal(QCursor::pos()));
    mouse /= canvasSize;
    mouse *= 2.0f;
    mouse -= 1.0f;
    mouse.y *= -1.0f;
    sbs_for_each_eye(size, [&](Eye eye) {
        mv.withPush([&] {
            // translate 
            mv.top() = getModelview(eye, mv.top());
            mv.translate(mouse);
            //mv.scale(0.05f);
            mv.scale(vec3(0.025f, 0.05f, 1.0f));
            Mat4Uniform(*program, "ModelView").Set(mv.top());
            plane->Draw();
        });
    });
    Context::Disable(Capability::Blend);
}


void Tv3dDisplayPlugin::activate(PluginContainer * container) {
    OpenGlDisplayPlugin::activate(container);
    // FIXME Add menu items
}
