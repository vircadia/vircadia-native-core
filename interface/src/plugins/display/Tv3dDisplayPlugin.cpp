//
//  Tv3dDisplayPlugin.cpp
//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Tv3dDisplayPlugin.h"
#include <GlWindow.h>

const QString Tv3dDisplayPlugin::NAME("Tv3dDisplayPlugin");

const QString & Tv3dDisplayPlugin::getName() {
    return NAME;
}

void Tv3dDisplayPlugin::display(
    GLuint sceneTexture, const glm::uvec2& sceneSize,
    GLuint overlayTexture, const glm::uvec2& overlaySize) {
    makeCurrent();
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);

    if (sceneTexture) {
        glBindTexture(GL_TEXTURE_2D, sceneTexture);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex2f(-1, -1);
        glTexCoord2f(1, 0);
        glVertex2f(+1, -1);
        glTexCoord2f(1, 1);
        glVertex2f(+1, +1);
        glTexCoord2f(0, 1);
        glVertex2f(-1, +1);
        glEnd();
    }

    if (overlayTexture) {
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);
        glBindTexture(GL_TEXTURE_2D, overlayTexture);

        QSize size = getDeviceSize();
        QRect r(QPoint(0, 0), QSize(size.width() / 2, size.height()));
        for (int i = 0; i < 2; ++i) {
            glViewport(r.x(), r.y(), r.width(), r.height());
            glScissor(r.x(), r.y(), r.width(), r.height());

            glBegin(GL_QUADS);
            glTexCoord2f(0, 0);
            glVertex2f(-1, -1);
            glTexCoord2f(1, 0);
            glVertex2f(+1, -1);
            glTexCoord2f(1, 1);
            glVertex2f(+1, +1);
            glTexCoord2f(0, 1);
            glVertex2f(-1, +1);
            glEnd();

            r.moveLeft(r.width());
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_BLEND);
    }
    glDisable(GL_TEXTURE_2D);
    Q_ASSERT(!glGetError());

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    glFinish();
}

/*
void Tv3dDisplayPlugin::activate() {
    GlWindowDisplayPlugin::activate();
    _window->setFlags(Qt::FramelessWindowHint);
    _window->setPosition(100, 100);
    _window->resize(512, 512);
    _window->setVisible(true);
    _window->show();
}

void Tv3dDisplayPlugin::deactivate() {
    GlWindowDisplayPlugin::deactivate();
}
*/

/*
glm::ivec2 LegacyDisplayPlugin::getCanvasSize() const {
    return toGlm(_window->size());
}

bool LegacyDisplayPlugin::hasFocus() const {
    return _window->hasFocus();
}

PickRay LegacyDisplayPlugin::computePickRay(const glm::vec2 & pos) const {
    return PickRay();
}

bool isMouseOnScreen() {
    return false;
}

void LegacyDisplayPlugin::preDisplay() {
    SimpleDisplayPlugin::preDisplay();
    auto size = toGlm(_window->size());
    glViewport(0, 0, size.x, size.y);
}

bool LegacyDisplayPlugin::isThrottled() const {
    return _window->isThrottleRendering();


    */

#if 0

int TV3DManager::_screenWidth = 1;
int TV3DManager::_screenHeight = 1;
double TV3DManager::_aspect = 1.0;
eyeFrustum TV3DManager::_leftEye;
eyeFrustum TV3DManager::_rightEye;
eyeFrustum* TV3DManager::_activeEye = NULL;


bool TV3DManager::isConnected() {
    return Menu::getInstance()->isOptionChecked(MenuOption::Enable3DTVMode);
}

void TV3DManager::connect() {
    auto deviceSize = qApp->getDeviceSize();
    configureCamera(*(qApp->getCamera()), deviceSize.width(), deviceSize.height());
}


// The basic strategy of this stereoscopic rendering is explained here:
//    http://www.orthostereo.com/geometryopengl.html
void TV3DManager::setFrustum(const Camera& whichCamera) {
    const double DTR = 0.0174532925; // degree to radians
    const double IOD = 0.05; //intraocular distance
    double fovy = DEFAULT_FIELD_OF_VIEW_DEGREES; // field of view in y-axis
    double nearZ = DEFAULT_NEAR_CLIP; // near clipping plane
    double screenZ = 0.25f; // screen projection plane

    double top = nearZ * tan(DTR * fovy / 2.0); //sets top of frustum based on fovy and near clipping plane
    double right = _aspect * top; // sets right of frustum based on aspect ratio
    double frustumshift = (IOD / 2) * nearZ / screenZ;

    _leftEye.top = top;
    _leftEye.bottom = -top;
    _leftEye.left = -right + frustumshift;
    _leftEye.right = right + frustumshift;
    _leftEye.modelTranslation = IOD / 2;

    _rightEye.top = top;
    _rightEye.bottom = -top;
    _rightEye.left = -right - frustumshift;
    _rightEye.right = right - frustumshift;
    _rightEye.modelTranslation = -IOD / 2;
}

void TV3DManager::configureCamera(Camera& whichCamera_, int screenWidth, int screenHeight) {
    const Camera& whichCamera = whichCamera_;

    if (screenHeight == 0) {
        screenHeight = 1; // prevent divide by 0
    }
    _screenWidth = screenWidth;
    _screenHeight = screenHeight;
    _aspect = (double)_screenWidth / (double)_screenHeight;
    setFrustum(whichCamera);

    glViewport(0, 0, _screenWidth, _screenHeight); // sets drawing viewport
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void TV3DManager::display(Camera& whichCamera) {
    double nearZ = DEFAULT_NEAR_CLIP; // near clipping plane
    double farZ = DEFAULT_FAR_CLIP; // far clipping plane

    // left eye portal
    int portalX = 0;
    int portalY = 0;
    QSize deviceSize = qApp->getDeviceSize() *
        qApp->getRenderResolutionScale();
    int portalW = deviceSize.width() / 2;
    int portalH = deviceSize.height();


    DependencyManager::get<GlowEffect>()->prepare();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Camera eyeCamera;
    eyeCamera.setRotation(whichCamera.getRotation());
    eyeCamera.setPosition(whichCamera.getPosition());

    glEnable(GL_SCISSOR_TEST);
    glPushMatrix();
    forEachEye([&](eyeFrustum& eye) {
        _activeEye = &eye;
        glViewport(portalX, portalY, portalW, portalH);
        glScissor(portalX, portalY, portalW, portalH);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity(); // reset projection matrix
        glFrustum(eye.left, eye.right, eye.bottom, eye.top, nearZ, farZ); // set left view frustum
        GLfloat p[4][4];
        // Really?
        glGetFloatv(GL_PROJECTION_MATRIX, &(p[0][0]));
        float cotangent = p[1][1];
        GLfloat fov = atan(1.0f / cotangent);
        glTranslatef(eye.modelTranslation, 0.0, 0.0); // translate to cancel parallax

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        qApp->displaySide(eyeCamera, false, RenderArgs::MONO);
#if 0
        qApp->getApplicationOverlay().displayOverlayTextureStereo(whichCamera, _aspect, fov);
#endif
        _activeEye = NULL;
    }, [&] {
        // render right side view
        portalX = deviceSize.width() / 2;
    });
    glPopMatrix();
    glDisable(GL_SCISSOR_TEST);

    auto finalFbo = DependencyManager::get<GlowEffect>()->render();
    auto fboSize = finalFbo->getSize();
    // Get the ACTUAL device size for the BLIT
    deviceSize = qApp->getDeviceSize();

    glBindFramebuffer(GL_READ_FRAMEBUFFER, gpu::GLBackend::getFramebufferID(finalFbo));
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, fboSize.x, fboSize.y,
        0, 0, deviceSize.width(), deviceSize.height(),
        GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // reset the viewport to how we started
    glViewport(0, 0, deviceSize.width(), deviceSize.height());
}

void TV3DManager::overrideOffAxisFrustum(float& left, float& right, float& bottom, float& top, float& nearVal,
    float& farVal, glm::vec4& nearClipPlane, glm::vec4& farClipPlane) {
    if (_activeEye) {
        left = _activeEye->left;
        right = _activeEye->right;
        bottom = _activeEye->bottom;
        top = _activeEye->top;
    }
}

#endif
