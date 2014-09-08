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
#include <PerfStat.h>

#include "Application.h"
#include "ApplicationOverlay.h"
#include "devices/OculusManager.h"

#include "ui/Stats.h"

// Used to fade the UI
const float FADE_SPEED = 0.08f;
// Used to animate the magnification windows
const float MAG_SPEED = 0.08f;

const quint64 MSECS_TO_USECS = 1000ULL;

// Fast helper functions
inline float max(float a, float b) {
    return (a > b) ? a : b;
}

inline float min(float a, float b) {
    return (a < b) ? a : b;
}

ApplicationOverlay::ApplicationOverlay() : 
    _framebufferObject(NULL),
    _textureFov(DEFAULT_OCULUS_UI_ANGULAR_SIZE * RADIANS_PER_DEGREE),
    _alpha(1.0f),
    _oculusuiRadius(1.0f),
    _crosshairTexture(0) {

    memset(_reticleActive, 0, sizeof(_reticleActive));
    memset(_magActive, 0, sizeof(_reticleActive));
    memset(_magSizeMult, 0, sizeof(_magSizeMult));
}

ApplicationOverlay::~ApplicationOverlay() {
    if (_framebufferObject != NULL) {
        delete _framebufferObject;
    }
}

const float WHITE_TEXT[] = { 0.93f, 0.93f, 0.93f };
const float RETICLE_COLOR[] = { 0.0f, 198.0f / 255.0f, 244.0f / 255.0f };

const float CONNECTION_STATUS_BORDER_COLOR[] = { 1.0f, 0.0f, 0.0f };
const float CONNECTION_STATUS_BORDER_LINE_WIDTH = 4.0f;

// Renders the overlays either to a texture or to the screen
void ApplicationOverlay::renderOverlay(bool renderToTexture) {

    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), "ApplicationOverlay::displayOverlay()");

    _textureFov = Menu::getInstance()->getOculusUIAngularSize() * RADIANS_PER_DEGREE;

    Application* application = Application::getInstance();

    Overlays& overlays = application->getOverlays();
    GLCanvas* glWidget = application->getGLWidget();
    MyAvatar* myAvatar = application->getAvatar();

    //Handle fading and deactivation/activation of UI
    if (Menu::getInstance()->isOptionChecked(MenuOption::UserInterface)) {
        _alpha += FADE_SPEED;
        if (_alpha > 1.0f) {
            _alpha = 1.0f;
        }
    } else {
        _alpha -= FADE_SPEED;
        if (_alpha <= 0.0f) {
            _alpha = 0.0f;
        }
    }

    if (renderToTexture) {
        getFramebufferObject()->bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    // Render 2D overlay
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();

    glLoadIdentity();
    gluOrtho2D(0, glWidget->width(), glWidget->height(), 0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    renderAudioMeter();

    if (Menu::getInstance()->isOptionChecked(MenuOption::HeadMouse)) {
        myAvatar->renderHeadMouse(glWidget->width(), glWidget->height());
    }

    renderStatsAndLogs();

    // give external parties a change to hook in
    emit application->renderingOverlay();

    overlays.render2D();

    renderPointers();

    renderDomainConnectionStatusBorder();

    glPopMatrix();


    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);

    if (renderToTexture) {
        getFramebufferObject()->release();
    }
}

// Draws the FBO texture for the screen
void ApplicationOverlay::displayOverlayTexture() {

    if (_alpha == 0.0f) {
        return;
    }

    Application* application = Application::getInstance();
    GLCanvas* glWidget = application->getGLWidget();


    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, getFramebufferObject()->texture());

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();

    glLoadIdentity();
    gluOrtho2D(0, glWidget->getDeviceWidth(), glWidget->getDeviceHeight(), 0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);

    glBegin(GL_QUADS);
    glColor4f(1.0f, 1.0f, 1.0f, _alpha);
    glTexCoord2f(0, 0); glVertex2i(0, glWidget->getDeviceHeight());
    glTexCoord2f(1, 0); glVertex2i(glWidget->getDeviceWidth(), glWidget->getDeviceHeight());
    glTexCoord2f(1, 1); glVertex2i(glWidget->getDeviceWidth(), 0);
    glTexCoord2f(0, 1); glVertex2i(0, 0);
    glEnd();
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
}

void ApplicationOverlay::computeOculusPickRay(float x, float y, glm::vec3& direction) const {
    MyAvatar* myAvatar = Application::getInstance()->getAvatar();

    //invert y direction
    y = 1.0 - y;

    //Get position on hemisphere UI
    x = sin((x - 0.5f) * _textureFov);
    y = sin((y - 0.5f) * _textureFov);

    float dist = sqrt(x * x + y * y);
    float z = -sqrt(1.0f - dist * dist);

    glm::vec3 relativePosition = myAvatar->getHead()->getEyePosition() +
        glm::normalize(myAvatar->getOrientation() * glm::vec3(x, y, z));

    //Rotate the UI pick ray by the avatar orientation
    direction = glm::normalize(relativePosition - Application::getInstance()->getCamera()->getPosition());
}

// Calculates the click location on the screen by taking into account any
// opened magnification windows.
void ApplicationOverlay::getClickLocation(int &x, int &y) const {
    int dx;
    int dy;
    const float xRange = MAGNIFY_WIDTH * MAGNIFY_MULT / 2.0f;
    const float yRange = MAGNIFY_HEIGHT * MAGNIFY_MULT / 2.0f;

    //Loop through all magnification windows
    for (int i = 0; i < NUMBER_OF_MAGNIFIERS; i++) {
        if (_magActive[i]) {
            dx = x - _magX[i];
            dy = y - _magY[i];
            //Check to see if they clicked inside a mag window
            if (abs(dx) <= xRange && abs(dy) <= yRange) {
                //Move the click to the actual UI location by inverting the magnification
                x = dx / MAGNIFY_MULT + _magX[i];
                y = dy / MAGNIFY_MULT + _magY[i];
                return;
            }
        }
    }
}

//Checks if the given ray intersects the sphere at the origin. result will store a multiplier that should
//be multiplied by dir and added to origin to get the location of the collision
bool raySphereIntersect(const glm::vec3 &dir, const glm::vec3 &origin, float r, float* result)
{
    //Source: http://wiki.cgsociety.org/index.php/Ray_Sphere_Intersection

    //Compute A, B and C coefficients
    float a = glm::dot(dir, dir);
    float b = 2 * glm::dot(dir, origin);
    float c = glm::dot(origin, origin) - (r * r);

    //Find discriminant
    float disc = b * b - 4 * a * c;

    // if discriminant is negative there are no real roots, so return 
    // false as ray misses sphere
    if (disc < 0) {
        return false;
    }

    // compute q as described above
    float distSqrt = sqrtf(disc);
    float q;
    if (b < 0) {
        q = (-b - distSqrt) / 2.0;
    } else {
        q = (-b + distSqrt) / 2.0;
    }

    // compute t0 and t1
    float t0 = q / a;
    float t1 = c / q;

    // make sure t0 is smaller than t1
    if (t0 > t1) {
        // if t0 is bigger than t1 swap them around
        float temp = t0;
        t0 = t1;
        t1 = temp;
    }

    // if t1 is less than zero, the object is in the ray's negative direction
    // and consequently the ray misses the sphere
    if (t1 < 0) {
        return false;
    }

    // if t0 is less than zero, the intersection point is at t1
    if (t0 < 0) {
        *result = t1;
        return true;
    } else { // else the intersection point is at t0
        *result = t0;
        return true;
    }
}

//Caculate the click location using one of the sixense controllers. Scale is not applied
QPoint ApplicationOverlay::getPalmClickLocation(const PalmData *palm) const {

    Application* application = Application::getInstance();
    GLCanvas* glWidget = application->getGLWidget();
    MyAvatar* myAvatar = application->getAvatar();

    glm::vec3 tip = myAvatar->getLaserPointerTipPosition(palm);
    glm::vec3 eyePos = myAvatar->getHead()->getEyePosition();
    glm::quat orientation = glm::inverse(myAvatar->getOrientation());
    glm::vec3 dir = orientation * glm::normalize(application->getCamera()->getPosition() - tip); //direction of ray goes towards camera
    glm::vec3 tipPos = orientation * (tip - eyePos);

    QPoint rv;

    if (OculusManager::isConnected()) {
        float t;

        //We back the ray up by dir to ensure that it will not start inside the UI.
        glm::vec3 adjustedPos = tipPos - dir;
        //Find intersection of crosshair ray. 
        if (raySphereIntersect(dir, adjustedPos, _oculusuiRadius * myAvatar->getScale(), &t)){
            glm::vec3 collisionPos = adjustedPos + dir * t;
            //Normalize it in case its not a radius of 1
            collisionPos = glm::normalize(collisionPos);
            //If we hit the back hemisphere, mark it as not a collision
            if (collisionPos.z > 0) {
                rv.setX(INT_MAX);
                rv.setY(INT_MAX);
            } else {

                float u = asin(collisionPos.x) / (_textureFov)+0.5f;
                float v = 1.0 - (asin(collisionPos.y) / (_textureFov)+0.5f);

                rv.setX(u * glWidget->width());
                rv.setY(v * glWidget->height());
            }
        } else {
            //if they did not click on the overlay, just set the coords to INT_MAX
            rv.setX(INT_MAX);
            rv.setY(INT_MAX);
        }
    } else {
        glm::dmat4 projection;
        application->getProjectionMatrix(&projection);

        glm::vec4 clipSpacePos = glm::vec4(projection * glm::dvec4(tipPos, 1.0));
        glm::vec3 ndcSpacePos;
        if (clipSpacePos.w != 0) {
            ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;
        }

        rv.setX(((ndcSpacePos.x + 1.0) / 2.0) * glWidget->width());
        rv.setY((1.0 - ((ndcSpacePos.y + 1.0) / 2.0)) * glWidget->height());
    }
    return rv;
}

//Finds the collision point of a world space ray
bool ApplicationOverlay::calculateRayUICollisionPoint(const glm::vec3& position, const glm::vec3& direction, glm::vec3& result) const {
    Application* application = Application::getInstance();
    MyAvatar* myAvatar = application->getAvatar();
    
    glm::quat orientation = myAvatar->getOrientation();

    glm::vec3 relativePosition = orientation * (position - myAvatar->getHead()->getEyePosition());
    glm::vec3 relativeDirection = orientation * direction;

    float t;
    if (raySphereIntersect(relativeDirection, relativePosition, _oculusuiRadius * myAvatar->getScale(), &t)){
        result = position + direction * t;
        return true;
    }

    return false;
}

// Draws the FBO texture for Oculus rift.
void ApplicationOverlay::displayOverlayTextureOculus(Camera& whichCamera) {

    if (_alpha == 0.0f) {
        return;
    }

    Application* application = Application::getInstance();

    MyAvatar* myAvatar = application->getAvatar();

    glActiveTexture(GL_TEXTURE0);
   
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, getFramebufferObject()->texture());

    glMatrixMode(GL_MODELVIEW);

    glDepthMask(GL_TRUE);
    
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.01f);

    //Update and draw the magnifiers

    glPushMatrix();
    const glm::quat& orientation = myAvatar->getOrientation();
    const glm::vec3& position = myAvatar->getHead()->getEyePosition();

    glm::mat4 rotation = glm::toMat4(orientation);

    glTranslatef(position.x, position.y, position.z);
    glMultMatrixf(&rotation[0][0]);
    for (int i = 0; i < NUMBER_OF_MAGNIFIERS; i++) {

        if (_magActive[i]) {
            _magSizeMult[i] += MAG_SPEED;
            if (_magSizeMult[i] > 1.0f) {
                _magSizeMult[i] = 1.0f;
            }
        } else {
            _magSizeMult[i] -= MAG_SPEED;
            if (_magSizeMult[i] < 0.0f) {
                _magSizeMult[i] = 0.0f;
            }
        }

        if (_magSizeMult[i] > 0.0f) {
            //Render magnifier, but dont show border for mouse magnifier
            renderMagnifier(_magX[i], _magY[i], _magSizeMult[i], i != MOUSE);
        }
    }
    glPopMatrix();

    glDepthMask(GL_FALSE);   
    glDisable(GL_ALPHA_TEST);

    glColor4f(1.0f, 1.0f, 1.0f, _alpha);

    renderTexturedHemisphere();

    renderPointersOculus(whichCamera.getPosition());

    glDepthMask(GL_TRUE);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);
    glEnable(GL_LIGHTING);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

}

// Draws the FBO texture for 3DTV.
void ApplicationOverlay::displayOverlayTexture3DTV(Camera& whichCamera, float aspectRatio, float fov) {

    if (_alpha == 0.0f) {
        return;
    }

    Application* application = Application::getInstance();

    MyAvatar* myAvatar = application->getAvatar();
    const glm::vec3& viewMatrixTranslation = application->getViewMatrixTranslation();

    glActiveTexture(GL_TEXTURE0);

    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);
    glBindTexture(GL_TEXTURE_2D, getFramebufferObject()->texture());
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_MODELVIEW);

    glPushMatrix();
    glLoadIdentity();
    // Transform to world space
    glm::quat rotation = whichCamera.getRotation();
    glm::vec3 axis2 = glm::axis(rotation);
    glRotatef(-glm::degrees(glm::angle(rotation)), axis2.x, axis2.y, axis2.z);
    glTranslatef(viewMatrixTranslation.x, viewMatrixTranslation.y, viewMatrixTranslation.z);

    // Translate to the front of the camera
    glm::vec3 pos = whichCamera.getPosition();
    glm::quat rot = myAvatar->getOrientation();
    glm::vec3 axis = glm::axis(rot);

    glTranslatef(pos.x, pos.y, pos.z);
    glRotatef(glm::degrees(glm::angle(rot)), axis.x, axis.y, axis.z);

    glColor4f(1.0f, 1.0f, 1.0f, _alpha);

    //Render
    const GLfloat distance = 1.0f;

    const GLfloat halfQuadHeight = distance * tan(fov);
    const GLfloat halfQuadWidth = halfQuadHeight * aspectRatio;
    const GLfloat quadWidth = halfQuadWidth * 2.0f;
    const GLfloat quadHeight = halfQuadHeight * 2.0f;

    GLfloat x = -halfQuadWidth;
    GLfloat y = -halfQuadHeight;
    glDisable(GL_DEPTH_TEST);

    glBegin(GL_QUADS);

    glTexCoord2f(0.0f, 1.0f); glVertex3f(x, y + quadHeight, -distance);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x + quadWidth, y + quadHeight, -distance);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x + quadWidth, y, -distance);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x, y, -distance);

    glEnd();

    if (_crosshairTexture == 0) {
        _crosshairTexture = Application::getInstance()->getGLWidget()->bindTexture(QImage(Application::resourcesPath() + "images/sixense-reticle.png"));
    }

    //draw the mouse pointer
    glBindTexture(GL_TEXTURE_2D, _crosshairTexture);

    const float reticleSize = 40.0f / application->getGLWidget()->width() * quadWidth;
    x -= reticleSize / 2.0f;
    y += reticleSize / 2.0f;
    const float mouseX = (application->getMouseX() / (float)application->getGLWidget()->width()) * quadWidth;
    const float mouseY = (1.0 - (application->getMouseY() / (float)application->getGLWidget()->height())) * quadHeight;

    glBegin(GL_QUADS);

    glColor3f(RETICLE_COLOR[0], RETICLE_COLOR[1], RETICLE_COLOR[2]);

    glTexCoord2d(0.0f, 0.0f); glVertex3f(x + mouseX, y + mouseY, -distance);
    glTexCoord2d(1.0f, 0.0f); glVertex3f(x + mouseX + reticleSize, y + mouseY, -distance);
    glTexCoord2d(1.0f, 1.0f); glVertex3f(x + mouseX + reticleSize, y + mouseY - reticleSize, -distance);
    glTexCoord2d(0.0f, 1.0f); glVertex3f(x + mouseX, y + mouseY - reticleSize, -distance);

    glEnd();

    glEnable(GL_DEPTH_TEST);

    glPopMatrix();

    glDepthMask(GL_TRUE);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);
    glEnable(GL_LIGHTING);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

//Renders optional pointers
void ApplicationOverlay::renderPointers() {
    Application* application = Application::getInstance();

    //lazily load crosshair texture
    if (_crosshairTexture == 0) {
        _crosshairTexture = Application::getInstance()->getGLWidget()->bindTexture(QImage(Application::resourcesPath() + "images/sixense-reticle.png"));
    }
    glEnable(GL_TEXTURE_2D);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _crosshairTexture);
    
    if (OculusManager::isConnected() && !application->getLastMouseMoveWasSimulated()) {
        //If we are in oculus, render reticle later
        _reticleActive[MOUSE] = true;
        _magActive[MOUSE] = true;
        _mouseX[MOUSE] = application->getMouseX();
        _mouseY[MOUSE] = application->getMouseY();
        _magX[MOUSE] = _mouseX[MOUSE];
        _magY[MOUSE] = _mouseY[MOUSE];
        _reticleActive[LEFT_CONTROLLER] = false;
        _reticleActive[RIGHT_CONTROLLER] = false;
        
    } else if (application->getLastMouseMoveWasSimulated() && Menu::getInstance()->isOptionChecked(MenuOption::SixenseMouseInput)) {
        //only render controller pointer if we aren't already rendering a mouse pointer
        _reticleActive[MOUSE] = false;
        _magActive[MOUSE] = false;
        renderControllerPointers();
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

}

void ApplicationOverlay::renderControllerPointers() {
    Application* application = Application::getInstance();
    GLCanvas* glWidget = application->getGLWidget();
    MyAvatar* myAvatar = application->getAvatar();

    //Static variables used for storing controller state
    static quint64 pressedTime[NUMBER_OF_MAGNIFIERS] = { 0ULL, 0ULL, 0ULL };
    static bool isPressed[NUMBER_OF_MAGNIFIERS] = { false, false, false };
    static bool stateWhenPressed[NUMBER_OF_MAGNIFIERS] = { false, false, false };
    static bool triggerPressed[NUMBER_OF_MAGNIFIERS] = { false, false, false };
    static bool bumperPressed[NUMBER_OF_MAGNIFIERS] = { false, false, false };

    const HandData* handData = Application::getInstance()->getAvatar()->getHandData();

    for (unsigned int palmIndex = 2; palmIndex < 4; palmIndex++) {
        const int index = palmIndex - 1;

        const PalmData* palmData = NULL;

        if (palmIndex >= handData->getPalms().size()) {
            return;
        }

        if (handData->getPalms()[palmIndex].isActive()) {
            palmData = &handData->getPalms()[palmIndex];
        } else {
            continue;
        }

        int controllerButtons = palmData->getControllerButtons();

        //Check for if we should toggle or drag the magnification window
        if (controllerButtons & BUTTON_3) {
            if (isPressed[index] == false) {
                //We are now dragging the window
                isPressed[index] = true;
                //set the pressed time in us
                pressedTime[index] = usecTimestampNow();
                stateWhenPressed[index] = _magActive[index];
            }
        } else if (isPressed[index]) {
            isPressed[index] = false;
            //If the button was only pressed for < 250 ms
            //then disable it.

            const int MAX_BUTTON_PRESS_TIME = 250 * MSECS_TO_USECS;
            if (usecTimestampNow() < pressedTime[index] + MAX_BUTTON_PRESS_TIME) {
                _magActive[index] = !stateWhenPressed[index];
            }
        }

        //Check for UI active toggle
        if (palmData->getTrigger() == 1.0f) {
            if (!triggerPressed[index]) {
                if (bumperPressed[index]) {
                    Menu::getInstance()->setIsOptionChecked(MenuOption::UserInterface,
                                                            !Menu::getInstance()->isOptionChecked(MenuOption::UserInterface));
                }
                triggerPressed[index] = true;
            }
        } else {
            triggerPressed[index] = false;
        }
        if ((controllerButtons & BUTTON_FWD)) {
            if (!bumperPressed[index]) {
                if (triggerPressed[index]) {
                    Menu::getInstance()->setIsOptionChecked(MenuOption::UserInterface,
                                                            !Menu::getInstance()->isOptionChecked(MenuOption::UserInterface));
                }
                bumperPressed[index] = true;
            }
        } else {
            bumperPressed[index] = false;
        }

        //if we have the oculus, we should make the cursor smaller since it will be
        //magnified
        if (OculusManager::isConnected()) {

            QPoint point = getPalmClickLocation(palmData);

            _mouseX[index] = point.x();
            _mouseY[index] = point.y();

            //When button 2 is pressed we drag the mag window
            if (isPressed[index]) {
                _magActive[index] = true;
                _magX[index] = point.x();
                _magY[index] = point.y();
            }

            // If oculus is enabled, we draw the crosshairs later
            continue;
        }

        int mouseX, mouseY;
        if (Menu::getInstance()->isOptionChecked(MenuOption::SixenseLasers)) {
            QPoint res = getPalmClickLocation(palmData);
            mouseX = res.x();
            mouseY = res.y();
        } else {
            // Get directon relative to avatar orientation
            glm::vec3 direction = glm::inverse(myAvatar->getOrientation()) * palmData->getFingerDirection();

            // Get the angles, scaled between (-0.5,0.5)
            float xAngle = (atan2(direction.z, direction.x) + M_PI_2);
            float yAngle = 0.5f - ((atan2(direction.z, direction.y) + M_PI_2));

            // Get the pixel range over which the xAngle and yAngle are scaled
            float cursorRange = glWidget->width() * application->getSixenseManager()->getCursorPixelRangeMult();

            mouseX = (glWidget->width() / 2.0f + cursorRange * xAngle);
            mouseY = (glWidget->height() / 2.0f + cursorRange * yAngle);
        }

        //If the cursor is out of the screen then don't render it
        if (mouseX < 0 || mouseX >= glWidget->width() || mouseY < 0 || mouseY >= glWidget->height()) {
            _reticleActive[index] = false;
            continue;
        }
        _reticleActive[index] = true;


        const float reticleSize = 40.0f;

        mouseX -= reticleSize / 2.0f;
        mouseY += reticleSize / 2.0f;

        glBegin(GL_QUADS);

        glColor3f(RETICLE_COLOR[0], RETICLE_COLOR[1], RETICLE_COLOR[2]);

        glTexCoord2d(0.0f, 0.0f); glVertex2i(mouseX, mouseY);
        glTexCoord2d(1.0f, 0.0f); glVertex2i(mouseX + reticleSize, mouseY);
        glTexCoord2d(1.0f, 1.0f); glVertex2i(mouseX + reticleSize, mouseY - reticleSize);
        glTexCoord2d(0.0f, 1.0f); glVertex2i(mouseX, mouseY - reticleSize);

        glEnd();
    }
}

void ApplicationOverlay::renderPointersOculus(const glm::vec3& eyePos) {

    Application* application = Application::getInstance();
    GLCanvas* glWidget = application->getGLWidget();
    glm::vec3 cursorVerts[4];

    const int widgetWidth = glWidget->width();
    const int widgetHeight = glWidget->height();
  
    const float reticleSize = 50.0f;

    glBindTexture(GL_TEXTURE_2D, _crosshairTexture);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_MODELVIEW);
    MyAvatar* myAvatar = application->getAvatar();

    //Controller Pointers
    for (int i = 0; i < (int)myAvatar->getHand()->getNumPalms(); i++) {

        PalmData& palm = myAvatar->getHand()->getPalms()[i];
        if (palm.isActive()) {
            glm::vec3 tip = myAvatar->getLaserPointerTipPosition(&palm);
            glm::vec3 tipPos = (tip - eyePos);
                
            float length = glm::length(eyePos - tip);
            float size = 0.03f * length;

            glm::vec3 up = glm::vec3(0.0, 1.0, 0.0) * size;
            glm::vec3 right = glm::vec3(1.0, 0.0, 0.0) * size;

            cursorVerts[0] = -right + up;
            cursorVerts[1] = right + up;
            cursorVerts[2] = right - up;
            cursorVerts[3] = -right - up;

            glPushMatrix();

            // objToCamProj is the vector in world coordinates from the 
            // local origin to the camera projected in the XZ plane
            glm::vec3 cursorToCameraXZ(-tipPos.x, 0, -tipPos.z);
            cursorToCameraXZ = glm::normalize(cursorToCameraXZ);

            //Translate the cursor to the tip of the oculus ray
            glTranslatef(tip.x, tip.y, tip.z);

            glm::vec3 direction(0, 0, 1);
            // easy fix to determine wether the angle is negative or positive
            // for positive angles upAux will be a vector pointing in the 
            // positive y direction, otherwise upAux will point downwards
            // effectively reversing the rotation.
            glm::vec3 upAux = glm::cross(direction, cursorToCameraXZ);

            // compute the angle
            float angleCosine = glm::dot(direction, cursorToCameraXZ);

            //Rotate in XZ direction
            glRotatef(acos(angleCosine) * DEGREES_PER_RADIAN, upAux[0], upAux[1], upAux[2]);

            glm::vec3 cursorToCamera = glm::normalize(-tipPos);

            // Compute the angle between cursorToCameraXZ and cursorToCamera, 
            angleCosine = glm::dot(cursorToCameraXZ, cursorToCamera);

            //Rotate in Y direction
            if (cursorToCamera.y < 0) {
                glRotatef(acos(angleCosine) * DEGREES_PER_RADIAN, 1, 0, 0);
            } else {
                glRotatef(acos(angleCosine) * DEGREES_PER_RADIAN, -1, 0, 0);
            }

            glBegin(GL_QUADS);

            glColor4f(RETICLE_COLOR[0], RETICLE_COLOR[1], RETICLE_COLOR[2], _alpha);

            glTexCoord2f(0.0f, 0.0f); glVertex3f(cursorVerts[0].x, cursorVerts[0].y, cursorVerts[0].z);
            glTexCoord2f(1.0f, 0.0f); glVertex3f(cursorVerts[1].x, cursorVerts[1].y, cursorVerts[1].z);
            glTexCoord2f(1.0f, 1.0f); glVertex3f(cursorVerts[2].x, cursorVerts[2].y, cursorVerts[2].z);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(cursorVerts[3].x, cursorVerts[3].y, cursorVerts[3].z);

            glEnd();

            glPopMatrix();
        } 
    }

    //Mouse Pointer
    if (_reticleActive[MOUSE]) {

        float mouseX = (float)_mouseX[MOUSE];
        float mouseY = (float)_mouseY[MOUSE];
        mouseX -= reticleSize / 2;
        mouseY += reticleSize / 2;

        //Get new UV coordinates from our magnification window
        float newULeft = mouseX / widgetWidth;
        float newURight = (mouseX + reticleSize) / widgetWidth;
        float newVBottom = 1.0 - mouseY / widgetHeight;
        float newVTop = 1.0 - (mouseY - reticleSize) / widgetHeight;

        // Project our position onto the hemisphere using the UV coordinates
        float lX = sin((newULeft - 0.5f) * _textureFov);
        float rX = sin((newURight - 0.5f) * _textureFov);
        float bY = sin((newVBottom - 0.5f) * _textureFov);
        float tY = sin((newVTop - 0.5f) * _textureFov);

        float dist;
        //Bottom Left
        dist = sqrt(lX * lX + bY * bY);
        float blZ = sqrt(1.0f - dist * dist);
        //Top Left
        dist = sqrt(lX * lX + tY * tY);
        float tlZ = sqrt(1.0f - dist * dist);
        //Bottom Right
        dist = sqrt(rX * rX + bY * bY);
        float brZ = sqrt(1.0f - dist * dist);
        //Top Right
        dist = sqrt(rX * rX + tY * tY);
        float trZ = sqrt(1.0f - dist * dist);

        glBegin(GL_QUADS);

        glColor4f(RETICLE_COLOR[0], RETICLE_COLOR[1], RETICLE_COLOR[2], _alpha);

        const glm::quat& orientation = myAvatar->getOrientation();
        cursorVerts[0] = orientation * glm::vec3(lX, tY, -tlZ) + eyePos;
        cursorVerts[1] = orientation * glm::vec3(rX, tY, -trZ) + eyePos;
        cursorVerts[2] = orientation * glm::vec3(rX, bY, -brZ) + eyePos;
        cursorVerts[3] = orientation * glm::vec3(lX, bY, -blZ) + eyePos;

        glTexCoord2f(0.0f, 0.0f); glVertex3f(cursorVerts[0].x, cursorVerts[0].y, cursorVerts[0].z);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(cursorVerts[1].x, cursorVerts[1].y, cursorVerts[1].z);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(cursorVerts[2].x, cursorVerts[2].y, cursorVerts[2].z);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(cursorVerts[3].x, cursorVerts[3].y, cursorVerts[3].z);

        glEnd();
    }
        

    glEnable(GL_DEPTH_TEST);
}

//Renders a small magnification of the currently bound texture at the coordinates
void ApplicationOverlay::renderMagnifier(int mouseX, int mouseY, float sizeMult, bool showBorder) const
{
    Application* application = Application::getInstance();
    GLCanvas* glWidget = application->getGLWidget();

    const int widgetWidth = glWidget->width();
    const int widgetHeight = glWidget->height();

    const float magnifyWidth = MAGNIFY_WIDTH * sizeMult;
    const float magnifyHeight = MAGNIFY_HEIGHT * sizeMult;

    mouseX -= magnifyWidth / 2;
    mouseY -= magnifyHeight / 2;

    float newWidth = magnifyWidth * MAGNIFY_MULT;
    float newHeight = magnifyHeight * MAGNIFY_MULT;

    // Magnification Texture Coordinates
    float magnifyULeft = mouseX / (float)widgetWidth;
    float magnifyURight = (mouseX + magnifyWidth) / (float)widgetWidth;
    float magnifyVBottom = 1.0f - mouseY / (float)widgetHeight;
    float magnifyVTop = 1.0f - (mouseY + magnifyHeight) / (float)widgetHeight;

    // Coordinates of magnification overlay
    float newMouseX = (mouseX + magnifyWidth / 2) - newWidth / 2.0f;
    float newMouseY = (mouseY + magnifyHeight / 2) + newHeight / 2.0f;

    // Get position on hemisphere using angle

    //Get new UV coordinates from our magnification window
    float newULeft = newMouseX / widgetWidth;
    float newURight = (newMouseX + newWidth) / widgetWidth;
    float newVBottom = 1.0 - newMouseY / widgetHeight;
    float newVTop = 1.0 - (newMouseY - newHeight) / widgetHeight;

    // Project our position onto the hemisphere using the UV coordinates
    float radius = _oculusuiRadius * application->getAvatar()->getScale();
    float radius2 = radius * radius;

    float lX = radius * sin((newULeft - 0.5f) * _textureFov);
    float rX = radius * sin((newURight - 0.5f) * _textureFov);
    float bY = radius * sin((newVBottom - 0.5f) * _textureFov);
    float tY = radius * sin((newVTop - 0.5f) * _textureFov);

    float blZ, tlZ, brZ, trZ;

    float dist;
    float discriminant;

    //Bottom Left
    dist = sqrt(lX * lX + bY * bY);
    discriminant = radius2 - dist * dist;
    if (discriminant > 0) {
        blZ = sqrt(discriminant);
    } else {
        blZ = 0;
    }
    //Top Left
    dist = sqrt(lX * lX + tY * tY);
    discriminant = radius2 - dist * dist;
    if (discriminant > 0) {
        tlZ = sqrt(discriminant);
    } else {
        tlZ = 0;
    }
    //Bottom Right
    dist = sqrt(rX * rX + bY * bY);
    discriminant = radius2 - dist * dist;
    if (discriminant > 0) {
        brZ = sqrt(discriminant);
    } else {
        brZ = 0;
    }
    //Top Right
    dist = sqrt(rX * rX + tY * tY);
    discriminant = radius2 - dist * dist;
    if (discriminant > 0) {
        trZ = sqrt(discriminant);
    } else {
        trZ = 0;
    }
    
    if (showBorder) {
        glDisable(GL_TEXTURE_2D);
        glLineWidth(1.0f);
        //Outer Line
        glBegin(GL_LINE_STRIP);
        glColor4f(1.0f, 0.0f, 0.0f, _alpha);

        glVertex3f(lX, tY, -tlZ);
        glVertex3f(rX, tY, -trZ);
        glVertex3f(rX, bY, -brZ);
        glVertex3f(lX, bY, -blZ);
        glVertex3f(lX, tY, -tlZ);

        glEnd();
        glEnable(GL_TEXTURE_2D);
    }
    glColor4f(1.0f, 1.0f, 1.0f, _alpha);

    glBegin(GL_QUADS);

    glTexCoord2f(magnifyULeft, magnifyVBottom); glVertex3f(lX, tY, -tlZ);
    glTexCoord2f(magnifyURight, magnifyVBottom); glVertex3f(rX, tY, -trZ);
    glTexCoord2f(magnifyURight, magnifyVTop); glVertex3f(rX, bY, -brZ);
    glTexCoord2f(magnifyULeft, magnifyVTop); glVertex3f(lX, bY, -blZ);

    glEnd();

}

void ApplicationOverlay::renderAudioMeter() {

    Application* application = Application::getInstance();

    GLCanvas* glWidget = application->getGLWidget();
    Audio* audio = application->getAudio();

    //  Display a single screen-size quad to create an alpha blended 'collision' flash
    if (audio->getCollisionFlashesScreen()) {
        float collisionSoundMagnitude = audio->getCollisionSoundMagnitude();
        const float VISIBLE_COLLISION_SOUND_MAGNITUDE = 0.5f;
        if (collisionSoundMagnitude > VISIBLE_COLLISION_SOUND_MAGNITUDE) {
            renderCollisionOverlay(glWidget->width(), glWidget->height(),
                audio->getCollisionSoundMagnitude());
        }
    }

    //  Audio VU Meter and Mute Icon
    const int MUTE_ICON_SIZE = 24;
    const int AUDIO_METER_INSET = 2;
    const int MUTE_ICON_PADDING = 10;
    const int AUDIO_METER_WIDTH = MIRROR_VIEW_WIDTH - MUTE_ICON_SIZE - AUDIO_METER_INSET - MUTE_ICON_PADDING;
    const int AUDIO_METER_SCALE_WIDTH = AUDIO_METER_WIDTH - 2 * AUDIO_METER_INSET;
    const int AUDIO_METER_HEIGHT = 8;
    const int AUDIO_METER_GAP = 5;
    const int AUDIO_METER_X = MIRROR_VIEW_LEFT_PADDING + MUTE_ICON_SIZE + AUDIO_METER_INSET + AUDIO_METER_GAP;

    int audioMeterY;
    if (Menu::getInstance()->isOptionChecked(MenuOption::Mirror)) {
        audioMeterY = MIRROR_VIEW_HEIGHT + AUDIO_METER_GAP + MUTE_ICON_PADDING;
    } else {
        audioMeterY = AUDIO_METER_GAP + MUTE_ICON_PADDING;
    }

    const float AUDIO_METER_BLUE[] = { 0.0, 0.0, 1.0 };
    const float AUDIO_METER_GREEN[] = { 0.0, 1.0, 0.0 };
    const float AUDIO_METER_RED[] = { 1.0, 0.0, 0.0 };
    const float AUDIO_GREEN_START = 0.25 * AUDIO_METER_SCALE_WIDTH;
    const float AUDIO_RED_START = 0.80 * AUDIO_METER_SCALE_WIDTH;
    const float CLIPPING_INDICATOR_TIME = 1.0f;
    const float AUDIO_METER_AVERAGING = 0.5;
    const float LOG2 = log(2.f);
    const float METER_LOUDNESS_SCALE = 2.8f / 5.f;
    const float LOG2_LOUDNESS_FLOOR = 11.f;
    float audioLevel = 0.f;
    float loudness = audio->getLastInputLoudness() + 1.f;

    _trailingAudioLoudness = AUDIO_METER_AVERAGING * _trailingAudioLoudness + (1.f - AUDIO_METER_AVERAGING) * loudness;
    float log2loudness = log(_trailingAudioLoudness) / LOG2;

    if (log2loudness <= LOG2_LOUDNESS_FLOOR) {
        audioLevel = (log2loudness / LOG2_LOUDNESS_FLOOR) * METER_LOUDNESS_SCALE * AUDIO_METER_SCALE_WIDTH;
    } else {
        audioLevel = (log2loudness - (LOG2_LOUDNESS_FLOOR - 1.f)) * METER_LOUDNESS_SCALE * AUDIO_METER_SCALE_WIDTH;
    }
    if (audioLevel > AUDIO_METER_SCALE_WIDTH) {
        audioLevel = AUDIO_METER_SCALE_WIDTH;
    }
    bool isClipping = ((audio->getTimeSinceLastClip() > 0.f) && (audio->getTimeSinceLastClip() < CLIPPING_INDICATOR_TIME));

    if ((audio->getTimeSinceLastClip() > 0.f) && (audio->getTimeSinceLastClip() < CLIPPING_INDICATOR_TIME)) {
        const float MAX_MAGNITUDE = 0.7f;
        float magnitude = MAX_MAGNITUDE * (1 - audio->getTimeSinceLastClip() / CLIPPING_INDICATOR_TIME);
        renderCollisionOverlay(glWidget->width(), glWidget->height(), magnitude, 1.0f);
    }

    audio->renderToolBox(MIRROR_VIEW_LEFT_PADDING + AUDIO_METER_GAP,
                         audioMeterY,
                         Menu::getInstance()->isOptionChecked(MenuOption::Mirror));

    audio->renderScope(glWidget->width(), glWidget->height());

    audio->renderStats(WHITE_TEXT, glWidget->width(), glWidget->height());

    glBegin(GL_QUADS);
    if (isClipping) {
        glColor3f(1, 0, 0);
    } else {
        glColor3f(0.475f, 0.475f, 0.475f);
    }

    audioMeterY += AUDIO_METER_HEIGHT;

    glColor3f(0, 0, 0);
    //  Draw audio meter background Quad
    glVertex2i(AUDIO_METER_X, audioMeterY);
    glVertex2i(AUDIO_METER_X + AUDIO_METER_WIDTH, audioMeterY);
    glVertex2i(AUDIO_METER_X + AUDIO_METER_WIDTH, audioMeterY + AUDIO_METER_HEIGHT);
    glVertex2i(AUDIO_METER_X, audioMeterY + AUDIO_METER_HEIGHT);

    if (audioLevel > AUDIO_RED_START) {
        if (!isClipping) {
            glColor3fv(AUDIO_METER_RED);
        } else {
            glColor3f(1, 1, 1);
        }
        // Draw Red Quad
        glVertex2i(AUDIO_METER_X + AUDIO_METER_INSET + AUDIO_RED_START, audioMeterY + AUDIO_METER_INSET);
        glVertex2i(AUDIO_METER_X + AUDIO_METER_INSET + audioLevel, audioMeterY + AUDIO_METER_INSET);
        glVertex2i(AUDIO_METER_X + AUDIO_METER_INSET + audioLevel, audioMeterY + AUDIO_METER_HEIGHT - AUDIO_METER_INSET);
        glVertex2i(AUDIO_METER_X + AUDIO_METER_INSET + AUDIO_RED_START, audioMeterY + AUDIO_METER_HEIGHT - AUDIO_METER_INSET);
        audioLevel = AUDIO_RED_START;
    }
    if (audioLevel > AUDIO_GREEN_START) {
        if (!isClipping) {
            glColor3fv(AUDIO_METER_GREEN);
        } else {
            glColor3f(1, 1, 1);
        }
        // Draw Green Quad
        glVertex2i(AUDIO_METER_X + AUDIO_METER_INSET + AUDIO_GREEN_START, audioMeterY + AUDIO_METER_INSET);
        glVertex2i(AUDIO_METER_X + AUDIO_METER_INSET + audioLevel, audioMeterY + AUDIO_METER_INSET);
        glVertex2i(AUDIO_METER_X + AUDIO_METER_INSET + audioLevel, audioMeterY + AUDIO_METER_HEIGHT - AUDIO_METER_INSET);
        glVertex2i(AUDIO_METER_X + AUDIO_METER_INSET + AUDIO_GREEN_START, audioMeterY + AUDIO_METER_HEIGHT - AUDIO_METER_INSET);
        audioLevel = AUDIO_GREEN_START;
    }
    //   Draw Blue Quad
    if (!isClipping) {
        glColor3fv(AUDIO_METER_BLUE);
    } else {
        glColor3f(1, 1, 1);
    }
    // Draw Blue (low level) quad
    glVertex2i(AUDIO_METER_X + AUDIO_METER_INSET, audioMeterY + AUDIO_METER_INSET);
    glVertex2i(AUDIO_METER_X + AUDIO_METER_INSET + audioLevel, audioMeterY + AUDIO_METER_INSET);
    glVertex2i(AUDIO_METER_X + AUDIO_METER_INSET + audioLevel, audioMeterY + AUDIO_METER_HEIGHT - AUDIO_METER_INSET);
    glVertex2i(AUDIO_METER_X + AUDIO_METER_INSET, audioMeterY + AUDIO_METER_HEIGHT - AUDIO_METER_INSET);
    glEnd();
}

void ApplicationOverlay::renderStatsAndLogs() {

    Application* application = Application::getInstance();

    GLCanvas* glWidget = application->getGLWidget();
    const OctreePacketProcessor& octreePacketProcessor = application->getOctreePacketProcessor();
    BandwidthMeter* bandwidthMeter = application->getBandwidthMeter();
    NodeBounds& nodeBoundsDisplay = application->getNodeBoundsDisplay();

    //  Display stats and log text onscreen
    glLineWidth(1.0f);
    glPointSize(1.0f);

    if (Menu::getInstance()->isOptionChecked(MenuOption::Stats)) {
        // let's set horizontal offset to give stats some margin to mirror
        int horizontalOffset = MIRROR_VIEW_WIDTH + MIRROR_VIEW_LEFT_PADDING * 2;
        int voxelPacketsToProcess = octreePacketProcessor.packetsToProcessCount();
        //  Onscreen text about position, servers, etc
        Stats::getInstance()->display(WHITE_TEXT, horizontalOffset, application->getFps(),
            application->getPacketsPerSecond(), application->getBytesPerSecond(), voxelPacketsToProcess);
        //  Bandwidth meter
        if (Menu::getInstance()->isOptionChecked(MenuOption::Bandwidth)) {
            Stats::drawBackground(0x33333399, glWidget->width() - 296, glWidget->height() - 68, 296, 68);
            bandwidthMeter->render(glWidget->width(), glWidget->height());
        }
    }

    //  Show on-screen msec timer
    if (Menu::getInstance()->isOptionChecked(MenuOption::FrameTimer)) {
        char frameTimer[10];
        quint64 mSecsNow = floor(usecTimestampNow() / 1000.0 + 0.5);
        sprintf(frameTimer, "%d\n", (int)(mSecsNow % 1000));
        int timerBottom =
            (Menu::getInstance()->isOptionChecked(MenuOption::Stats) &&
            Menu::getInstance()->isOptionChecked(MenuOption::Bandwidth))
            ? 80 : 20;
        drawText(glWidget->width() - 100, glWidget->height() - timerBottom,
            0.30f, 0.0f, 0, frameTimer, WHITE_TEXT);
    }
    nodeBoundsDisplay.drawOverlay();
}

//Renders a hemisphere with texture coordinates.
void ApplicationOverlay::renderTexturedHemisphere() {
    const int slices = 80;
    const int stacks = 80;

    //UV mapping source: http://www.mvps.org/directx/articles/spheremap.htm
    static VerticesIndices vbo(0, 0);
    int vertices = slices * (stacks - 1) + 1;
    int indices = slices * 2 * 3 * (stacks - 2) + slices * 3;

    static float oldTextureFOV = _textureFov;
    //We only generate the VBO when the _textureFov changes
    if (vbo.first == 0 || oldTextureFOV != _textureFov) {
        oldTextureFOV = _textureFov;
        TextureVertex* vertexData = new TextureVertex[vertices];
        TextureVertex* vertex = vertexData;
        for (int i = 0; i < stacks - 1; i++) {
            float phi = PI_OVER_TWO * (float)i / (float)(stacks - 1);
            float z = -sinf(phi), radius = cosf(phi);

            for (int j = 0; j < slices; j++) {
                float theta = TWO_PI * (float)j / (float)slices;

                vertex->position.x = sinf(theta) * radius;
                vertex->position.y = cosf(theta) * radius;
                vertex->position.z = z;
                vertex->uv.x = asin(vertex->position.x) / (_textureFov) + 0.5f;
                vertex->uv.y = asin(vertex->position.y) / (_textureFov) + 0.5f;
                vertex++;
            }
        }
        vertex->position.x = 0.0f;
        vertex->position.y = 0.0f;
        vertex->position.z = -1.0f;
        vertex->uv.x = 0.5f;
        vertex->uv.y = 0.5f;
        vertex++;

        if (vbo.first == 0){
            glGenBuffers(1, &vbo.first);
        }
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        const int BYTES_PER_VERTEX = sizeof(TextureVertex);
        glBufferData(GL_ARRAY_BUFFER, vertices * BYTES_PER_VERTEX, vertexData, GL_STATIC_DRAW);
        delete[] vertexData;

        GLushort* indexData = new GLushort[indices];
        GLushort* index = indexData;
        for (int i = 0; i < stacks - 2; i++) {
            GLushort bottom = i * slices;
            GLushort top = bottom + slices;
            for (int j = 0; j < slices; j++) {
                int next = (j + 1) % slices;

                *(index++) = bottom + j;
                *(index++) = top + next;
                *(index++) = top + j;

                *(index++) = bottom + j;
                *(index++) = bottom + next;
                *(index++) = top + next;
            }
        }
        GLushort bottom = (stacks - 2) * slices;
        GLushort top = bottom + slices;
        for (int i = 0; i < slices; i++) {
            *(index++) = bottom + i;
            *(index++) = bottom + (i + 1) % slices;
            *(index++) = top;
        }

        glGenBuffers(1, &vbo.second);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
        const int BYTES_PER_INDEX = sizeof(GLushort);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices * BYTES_PER_INDEX, indexData, GL_STATIC_DRAW);
        delete[] indexData;

    } else {
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
    }
 
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(3, GL_FLOAT, sizeof(TextureVertex), (void*)0);
    glTexCoordPointer(2, GL_FLOAT, sizeof(TextureVertex), (void*)12);

    glPushMatrix();
    Application* application = Application::getInstance();
    MyAvatar* myAvatar = application->getAvatar();
    const glm::quat& orientation = myAvatar->getOrientation();
    const glm::vec3& position = myAvatar->getHead()->getEyePosition();

    glm::mat4 rotation = glm::toMat4(orientation);

    glTranslatef(position.x, position.y, position.z);
    glMultMatrixf(&rotation[0][0]);
    
    const float scale = _oculusuiRadius * myAvatar->getScale();
    glScalef(scale, scale, scale);

    glDrawRangeElements(GL_TRIANGLES, 0, vertices - 1, indices, GL_UNSIGNED_SHORT, 0);

    glPopMatrix();

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

}

void ApplicationOverlay::renderDomainConnectionStatusBorder() {
    NodeList* nodeList = NodeList::getInstance();

    if (nodeList && !nodeList->getDomainHandler().isConnected()) {
        GLCanvas* glWidget = Application::getInstance()->getGLWidget();
        int right = glWidget->width();
        int bottom = glWidget->height();

        glColor3f(CONNECTION_STATUS_BORDER_COLOR[0],
                  CONNECTION_STATUS_BORDER_COLOR[1],
                  CONNECTION_STATUS_BORDER_COLOR[2]);
        glLineWidth(CONNECTION_STATUS_BORDER_LINE_WIDTH);

        glBegin(GL_LINE_LOOP);

        glVertex2i(0, 0);
        glVertex2i(0, bottom);
        glVertex2i(right, bottom);
        glVertex2i(right, 0);

        glEnd();
    }
}

QOpenGLFramebufferObject* ApplicationOverlay::getFramebufferObject() {
    QSize size = Application::getInstance()->getGLWidget()->getDeviceSize();
    if (!_framebufferObject || _framebufferObject->size() != size) {

        delete _framebufferObject;

        _framebufferObject = new QOpenGLFramebufferObject(size);
        glBindTexture(GL_TEXTURE_2D, _framebufferObject->texture());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        GLfloat borderColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    return _framebufferObject;
}

