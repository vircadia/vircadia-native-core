//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SideBySideStereoDisplayPlugin.h"

#include <QApplication>
#include <QDesktopWidget>

#include <GlWindow.h>
#include <ViewFrustum.h>
#include <MatrixStack.h>

#include <gpu/GLBackend.h>

const QString SideBySideStereoDisplayPlugin::NAME("SBS Stereo Display");

const QString & SideBySideStereoDisplayPlugin::getName() {
    return NAME;
}

SideBySideStereoDisplayPlugin::SideBySideStereoDisplayPlugin() {
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

void SideBySideStereoDisplayPlugin::display(
    GLuint sceneTexture, const glm::uvec2& sceneSize,
    GLuint overlayTexture, const glm::uvec2& overlaySize) {
    makeCurrent();
    Q_ASSERT(0 == glGetError());
    uvec2 size = toGlm(getDeviceSize());
    using namespace oglplus;
    Context::Viewport(size.x, size.y);
    Context::Clear().ColorBuffer();
    
    _program->Bind();
    Mat4Uniform(*_program, "ModelView").Set(mat4());
    Mat4Uniform(*_program, "Projection").Set(mat4());

    glBindTexture(GL_TEXTURE_2D, sceneTexture);

    _plane->Use();
    _plane->Draw();

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
    Mat4Uniform(*_program, "Projection").Set(pr);

    // Position the camera relative to the overlay texture
    MatrixStack mv;
    mv.translate(vec3(0, 0, -distance)).scale(vec3(0.7f, 0.7f / screenAspect, 1.0f)); // .scale(vec3(quadWidth, quadHeight, 1.0));
    sbs_for_each_eye(size, [&](Eye eye) {
        mv.withPush([&] {
            // translate 
            mv.top() = getModelview(eye, mv.top());
            Mat4Uniform(*_program, "ModelView").Set(mv.top());
            _plane->Draw();
        });
    });

    glBindTexture(GL_TEXTURE_2D, gpu::GLBackend::getTextureID(_crosshairTexture));
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
            Mat4Uniform(*_program, "ModelView").Set(mv.top());
            _plane->Draw();
        });
    });
    Context::Disable(Capability::Blend);
}
