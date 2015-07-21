//
//  TV3DManager.cpp
//  interface/src/devices
//
//  Created by Brad Hefta-Gaub on 12/24/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "gpu/GLBackend.h"
#include "Application.h"

#include "TV3DManager.h"
#include "Menu.h"

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
void TV3DManager::setFrustum(Camera& whichCamera) {
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

void TV3DManager::configureCamera(Camera& whichCamera, int screenWidth, int screenHeight) {
    if (screenHeight == 0) {
        screenHeight = 1; // prevent divide by 0
    }
    _screenWidth = screenWidth;
    _screenHeight = screenHeight;
    _aspect= (double)_screenWidth / (double)_screenHeight;
    setFrustum(whichCamera);

    glViewport (0, 0, _screenWidth, _screenHeight); // sets drawing viewport
}

void TV3DManager::display(RenderArgs* renderArgs, Camera& whichCamera) {

#ifdef THIS_CURRENTLY_BROKEN_WAITING_FOR_DISPLAY_PLUGINS 

    double nearZ = DEFAULT_NEAR_CLIP; // near clipping plane
    double farZ = DEFAULT_FAR_CLIP; // far clipping plane

    // left eye portal
    int portalX = 0;
    int portalY = 0;
    QSize deviceSize = qApp->getDeviceSize() *
        qApp->getRenderResolutionScale();
    int portalW = deviceSize.width() / 2;
    int portalH = deviceSize.height();


    // FIXME - glow effect is removed, 3D TV mode broken until we get display plugins working
    DependencyManager::get<GlowEffect>()->prepare(renderArgs);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Camera eyeCamera;
    eyeCamera.setRotation(whichCamera.getRotation());
    eyeCamera.setPosition(whichCamera.getPosition());

    glEnable(GL_SCISSOR_TEST);
    forEachEye([&](eyeFrustum& eye){
        _activeEye = &eye;
        glViewport(portalX, portalY, portalW, portalH);
        glScissor(portalX, portalY, portalW, portalH);
        renderArgs->_viewport = glm::ivec4(portalX, portalY, portalW, portalH);

        glm::mat4 projection = glm::frustum<float>(eye.left, eye.right, eye.bottom, eye.top, nearZ, farZ);
        projection = glm::translate(projection, vec3(eye.modelTranslation, 0, 0));
        eyeCamera.setProjection(projection);
        renderArgs->_renderSide = RenderArgs::MONO;
        qApp->displaySide(renderArgs, eyeCamera, false);
        qApp->getApplicationCompositor().displayOverlayTexture(renderArgs);
        _activeEye = NULL;
    }, [&]{
        // render right side view
        portalX = deviceSize.width() / 2;
    });
    glDisable(GL_SCISSOR_TEST);

    // FIXME - glow effect is removed, 3D TV mode broken until we get display plugins working
    auto finalFbo = DependencyManager::get<GlowEffect>()->render(renderArgs);
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

#endif 
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
