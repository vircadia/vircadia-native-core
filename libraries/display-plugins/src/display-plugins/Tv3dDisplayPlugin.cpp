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
#include <PathUtils.h>


#include "../OglplusHelpers.h"

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

static ProgramPtr program;
static ShapeWrapperPtr plane;

void Tv3dDisplayPlugin::customizeWindow() {
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
}

void Tv3dDisplayPlugin::customizeContext() {
    using namespace oglplus;
    Context::BlendFunc(BlendFunction::SrcAlpha, BlendFunction::OneMinusSrcAlpha);
    Context::Disable(Capability::Blend);
    Context::Disable(Capability::DepthTest);
    Context::Disable(Capability::CullFace);
    program = loadDefaultShader();
    plane = loadPlane(program);
    Context::ClearColor(0, 0, 0, 1);
    //        _crosshairTexture = DependencyManager::get<TextureCache>()->
    //            getImageTexture(PathUtils::resourcesPath() + "images/sixense-reticle.png");
}


const float DEFAULT_IPD = 0.064f;
const float HALF_DEFAULT_IPD = DEFAULT_IPD / 2.0f;

glm::mat4 Tv3dDisplayPlugin::getProjection(Eye eye, const glm::mat4& baseProjection) const {
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



void Tv3dDisplayPlugin::display(
    GLuint sceneTexture, const glm::uvec2& sceneSize,
    GLuint overlayTexture, const glm::uvec2& overlaySize) {

    QSize size = getDeviceSize();
    using namespace oglplus;
    Context::Viewport(size.width(), size.height());
    Context::Clear().ColorBuffer().DepthBuffer();

    Mat4Uniform(*program, "ModelView").Set(mat4());
    Mat4Uniform(*program, "Projection").Set(mat4());

    glBindTexture(GL_TEXTURE_2D, sceneTexture);

    plane->Draw();

    const float overlayAspect = aspect(toGlm(size));
    const GLfloat distance = 1.0f;
    const GLfloat halfQuadHeight = distance * tan(DEFAULT_FIELD_OF_VIEW_DEGREES);
    const GLfloat halfQuadWidth = halfQuadHeight * (float)size.width() / (float)size.height();
    const GLfloat quadWidth = halfQuadWidth * 2.0f;
    const GLfloat quadHeight = halfQuadHeight * 2.0f;

    vec3 quadSize(quadWidth, quadHeight, 1.0f);
    quadSize = vec3(1.0f) / quadSize;

    using namespace oglplus;
    Context::Enable(Capability::Blend);
    glBindTexture(GL_TEXTURE_2D, overlayTexture);
    
    mat4 pr = glm::perspective(glm::radians(DEFAULT_FIELD_OF_VIEW_DEGREES), aspect(toGlm(size)), DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP);
    Mat4Uniform(*program, "Projection").Set(pr);

    MatrixStack mv;
    mv.translate(vec3(0, 0, -distance)).scale(vec3(0.7f, 0.7f / overlayAspect, 1.0f)); // .scale(vec3(quadWidth, quadHeight, 1.0));

    QRect r(QPoint(0, 0), QSize(size.width() / 2, size.height()));
    for (int i = 0; i < 2; ++i) {
        Context::Viewport(r.x(), r.y(), r.width(), r.height());
        Mat4Uniform(*program, "ModelView").Set(mv.top());
        plane->Draw();
        r.moveLeft(r.width());
    }

#if 0
    glBindTexture(GL_TEXTURE_2D, gpu::GLBackend::getTextureID(_crosshairTexture));
    glm::vec2 canvasSize = qApp->getCanvasSize();
    glm::vec2 mouse = qApp->getMouse();
    mouse /= canvasSize;
    mouse *= 2.0f;
    mouse -= 1.0f;
    mouse.y *= -1.0f;
    mv.translate(mouse);
    mv.scale(0.1f);
    Mat4Uniform(*program, "ModelView").Set(mv.top());
    r = QRect(QPoint(0, 0), QSize(size.width() / 2, size.height()));
    for (int i = 0; i < 2; ++i) {
        Context::Viewport(r.x(), r.y(), r.width(), r.height());
        plane->Draw();
        r.moveLeft(r.width());
    }
#endif
    Context::Disable(Capability::Blend);
}


void Tv3dDisplayPlugin::activate(PluginContainer * container) {
    GlWindowDisplayPlugin::activate(container);
    _window->show();
}

void Tv3dDisplayPlugin::deactivate() {
    makeCurrent();
    if (plane) {
        plane.reset();
        program.reset();
//        _crosshairTexture.reset();
    }
    doneCurrent();
    GlWindowDisplayPlugin::deactivate();
}
