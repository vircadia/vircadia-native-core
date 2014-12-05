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

const float WHITE_TEXT[] = { 0.93f, 0.93f, 0.93f };
const float RETICLE_COLOR[] = { 0.0f, 198.0f / 255.0f, 244.0f / 255.0f };


const float CONNECTION_STATUS_BORDER_COLOR[] = { 1.0f, 0.0f, 0.0f };
const float CONNECTION_STATUS_BORDER_LINE_WIDTH = 4.0f;

static const float MOUSE_PITCH_RANGE = 1.0f * PI;
static const float MOUSE_YAW_RANGE = 0.5f * TWO_PI;


// Return a point's coordinates on a sphere from pitch and yaw
glm::vec3 getPoint(float yaw, float pitch) {
    return glm::vec3(glm::cos(-pitch) * (-glm::sin(yaw)),
                     glm::sin(-pitch),
                     glm::cos(-pitch) * (-glm::cos(yaw)));
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

void renderReticule(glm::quat orientation, float alpha) {
    const float reticleSize = TWO_PI / 80.0f;
    
    glm::vec3 topLeft = getPoint(reticleSize / 2.0f, -reticleSize / 2.0f);
    glm::vec3 topRight = getPoint(-reticleSize / 2.0f, -reticleSize / 2.0f);
    glm::vec3 bottomLeft = getPoint(reticleSize / 2.0f, reticleSize / 2.0f);
    glm::vec3 bottomRight = getPoint(-reticleSize / 2.0f, reticleSize / 2.0f);
    
    glPushMatrix(); {
        glm::vec3 axis = glm::axis(orientation);
        glRotatef(glm::degrees(glm::angle(orientation)), axis.x, axis.y, axis.z);
        
        glBegin(GL_QUADS); {
            glColor4f(RETICLE_COLOR[0], RETICLE_COLOR[1], RETICLE_COLOR[2], alpha);
            
            glTexCoord2f(0.0f, 0.0f); glVertex3f(topLeft.x, topLeft.y, topLeft.z);
            glTexCoord2f(1.0f, 0.0f); glVertex3f(bottomLeft.x, bottomLeft.y, bottomLeft.z);
            glTexCoord2f(1.0f, 1.0f); glVertex3f(bottomRight.x, bottomRight.y, bottomRight.z);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(topRight.x, topRight.y, topRight.z);
        } glEnd();
    } glPopMatrix();
}

ApplicationOverlay::ApplicationOverlay() :
    _textureFov(glm::radians(DEFAULT_OCULUS_UI_ANGULAR_SIZE)),
    _textureAspectRatio(1.0f),
    _alpha(1.0f),
    _oculusUIRadius(1.0f),
    _crosshairTexture(0) {

    memset(_reticleActive, 0, sizeof(_reticleActive));
    memset(_magActive, 0, sizeof(_reticleActive));
    memset(_magSizeMult, 0, sizeof(_magSizeMult));
}

ApplicationOverlay::~ApplicationOverlay() {
}

// Renders the overlays either to a texture or to the screen
void ApplicationOverlay::renderOverlay(bool renderToTexture) {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), "ApplicationOverlay::displayOverlay()");
    Application* application = Application::getInstance();
    Overlays& overlays = application->getOverlays();
    GLCanvas* glWidget = application->getGLWidget();
    MyAvatar* myAvatar = application->getAvatar();
    
    _textureFov = glm::radians(Menu::getInstance()->getOculusUIAngularSize());
    _textureAspectRatio = (float)application->getGLWidget()->getDeviceWidth() / (float)application->getGLWidget()->getDeviceHeight();

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

    // Render 2D overlay
    glMatrixMode(GL_PROJECTION);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (renderToTexture) {
        _overlays.buildFramebufferObject();
        _overlays.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    
    glPushMatrix(); {
        glLoadIdentity();
        gluOrtho2D(0, glWidget->width(), glWidget->height(), 0);
        
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
    } glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);

    if (renderToTexture) {
        _overlays.release();
    }
}

// Draws the FBO texture for the screen
void ApplicationOverlay::displayOverlayTexture() {
    if (_alpha == 0.0f) {
        return;
    }
    GLCanvas* glWidget = Application::getInstance()->getGLWidget();

    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    _overlays.bindTexture();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); {
        glLoadIdentity();
        gluOrtho2D(0, glWidget->getDeviceWidth(), glWidget->getDeviceHeight(), 0);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        
        glBegin(GL_QUADS); {
            glColor4f(1.0f, 1.0f, 1.0f, _alpha);
            glTexCoord2f(0, 0); glVertex2i(0, glWidget->getDeviceHeight());
            glTexCoord2f(1, 0); glVertex2i(glWidget->getDeviceWidth(), glWidget->getDeviceHeight());
            glTexCoord2f(1, 1); glVertex2i(glWidget->getDeviceWidth(), 0);
            glTexCoord2f(0, 1); glVertex2i(0, 0);
        } glEnd();
    } glPopMatrix();
    
    glDisable(GL_TEXTURE_2D);
}

// Draws the FBO texture for Oculus rift.
void ApplicationOverlay::displayOverlayTextureOculus(Camera& whichCamera) {
    if (_alpha == 0.0f) {
        return;
    }
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    _overlays.bindTexture();
    
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_LIGHTING);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.01f);
    
    
    //Update and draw the magnifiers
    MyAvatar* myAvatar = Application::getInstance()->getAvatar();
    const glm::quat& orientation = myAvatar->getOrientation();
    const glm::vec3& position = myAvatar->getDefaultEyePosition();
    const float scale = myAvatar->getScale() * _oculusUIRadius;
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); {
        glTranslatef(position.x, position.y, position.z);
        glm::mat4 rotation = glm::toMat4(orientation);
        glMultMatrixf(&rotation[0][0]);
        glScalef(scale, scale, scale);
        for (int i = 0; i < NUMBER_OF_RETICULES; i++) {
            
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
                glm::vec2 projection = screenToOverlay(_reticulePosition[i]);
                
                renderMagnifier(projection, _magSizeMult[i], i != MOUSE);
            }
        }
        
        glDepthMask(GL_FALSE);
        glDisable(GL_ALPHA_TEST);
        
        glColor4f(1.0f, 1.0f, 1.0f, _alpha);
        
        static float textureFOV = 0.0f, textureAspectRatio = 1.0f;
        if (textureFOV != _textureFov ||
            textureAspectRatio != _textureAspectRatio) {
            textureFOV = _textureFov;
            textureAspectRatio = _textureAspectRatio;
            
            _overlays.buildVBO(_textureFov, _textureAspectRatio, 80, 80);
        }
        _overlays.render();
        renderPointersOculus(myAvatar->getDefaultEyePosition());
        
        glDepthMask(GL_TRUE);
        _overlays.releaseTexture();
        glDisable(GL_TEXTURE_2D);
        
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);
        glEnable(GL_LIGHTING);
    } glPopMatrix();
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
    _overlays.bindTexture();
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

void ApplicationOverlay::computeOculusPickRay(float x, float y, glm::vec3& origin, glm::vec3& direction) const {
    const float pitch = (0.5f - y) * MOUSE_PITCH_RANGE;
    const float yaw = (0.5f - x) * MOUSE_YAW_RANGE;
    const glm::quat orientation(glm::vec3(pitch, yaw, 0.0f));
    const glm::vec3 localDirection = orientation * IDENTITY_FRONT;

    //Rotate the UI pick ray by the avatar orientation
    const MyAvatar* myAvatar = Application::getInstance()->getAvatar();
    origin = myAvatar->getDefaultEyePosition();
    direction = myAvatar->getOrientation() * localDirection;
}

//Caculate the click location using one of the sixense controllers. Scale is not applied
QPoint ApplicationOverlay::getPalmClickLocation(const PalmData *palm) const {
    Application* application = Application::getInstance();
    GLCanvas* glWidget = application->getGLWidget();
    MyAvatar* myAvatar = application->getAvatar();

    glm::vec3 tip = myAvatar->getLaserPointerTipPosition(palm);
    glm::vec3 eyePos = myAvatar->getHead()->getEyePosition();
    glm::quat invOrientation = glm::inverse(myAvatar->getOrientation());
    //direction of ray goes towards camera
    glm::vec3 dir = invOrientation * glm::normalize(application->getCamera()->getPosition() - tip);
    glm::vec3 tipPos = invOrientation * (tip - eyePos);

    QPoint rv;

    if (OculusManager::isConnected()) {
        float t;

        //We back the ray up by dir to ensure that it will not start inside the UI.
        glm::vec3 adjustedPos = tipPos - dir;
        //Find intersection of crosshair ray. 
        if (raySphereIntersect(dir, adjustedPos, _oculusUIRadius * myAvatar->getScale(), &t)){
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

    glm::vec3 relativePosition = orientation * (position - myAvatar->getDefaultEyePosition());
    glm::vec3 relativeDirection = orientation * direction;

    float t;
    if (raySphereIntersect(relativeDirection, relativePosition, _oculusUIRadius * myAvatar->getScale(), &t)){
        result = position + direction * t;
        return true;
    }

    return false;
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
        _reticulePosition[MOUSE] = glm::vec2(application->getTrueMouseX(), application->getTrueMouseY());
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
    static quint64 pressedTime[NUMBER_OF_RETICULES] = { 0ULL, 0ULL, 0ULL };
    static bool isPressed[NUMBER_OF_RETICULES] = { false, false, false };
    static bool stateWhenPressed[NUMBER_OF_RETICULES] = { false, false, false };

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

        //if we have the oculus, we should make the cursor smaller since it will be
        //magnified
        if (OculusManager::isConnected()) {

            QPoint point = getPalmClickLocation(palmData);

            _reticulePosition[index] = glm::vec2(point.x(), point.y());

            //When button 2 is pressed we drag the mag window
            if (isPressed[index]) {
                _magActive[index] = true;
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
            float cursorRange = glWidget->width() * SixenseManager::getInstance().getCursorPixelRangeMult();

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
    glBindTexture(GL_TEXTURE_2D, _crosshairTexture);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_MODELVIEW);
    
    //Controller Pointers
    MyAvatar* myAvatar = Application::getInstance()->getAvatar();
    for (int i = 0; i < (int)myAvatar->getHand()->getNumPalms(); i++) {

//        PalmData& palm = myAvatar->getHand()->getPalms()[i];
//        if (palm.isActive()) {
//            glm::vec3 tip = myAvatar->getLaserPointerTipPosition(&palm);
//            glm::vec3 tipPos = (tip - eyePos);
//                
//            float length = glm::length(eyePos - tip);
//            float size = 0.03f * length;
//
//            glm::vec3 up = glm::vec3(0.0, 1.0, 0.0) * size;
//            glm::vec3 right = glm::vec3(1.0, 0.0, 0.0) * size;
//
//            cursorVerts[0] = -right + up;
//            cursorVerts[1] = right + up;
//            cursorVerts[2] = right - up;
//            cursorVerts[3] = -right - up;
//
//            glPushMatrix();
//
//            // objToCamProj is the vector in world coordinates from the 
//            // local origin to the camera projected in the XZ plane
//            glm::vec3 cursorToCameraXZ(-tipPos.x, 0, -tipPos.z);
//            cursorToCameraXZ = glm::normalize(cursorToCameraXZ);
//
//            //Translate the cursor to the tip of the oculus ray
//            glTranslatef(tip.x, tip.y, tip.z);
//
//            glm::vec3 direction(0, 0, 1);
//            // easy fix to determine wether the angle is negative or positive
//            // for positive angles upAux will be a vector pointing in the 
//            // positive y direction, otherwise upAux will point downwards
//            // effectively reversing the rotation.
//            glm::vec3 upAux = glm::cross(direction, cursorToCameraXZ);
//
//            // compute the angle
//            float angleCosine = glm::dot(direction, cursorToCameraXZ);
//
//            //Rotate in XZ direction
//            glRotatef(acos(angleCosine) * DEGREES_PER_RADIAN, upAux[0], upAux[1], upAux[2]);
//
//            glm::vec3 cursorToCamera = glm::normalize(-tipPos);
//
//            // Compute the angle between cursorToCameraXZ and cursorToCamera, 
//            angleCosine = glm::dot(cursorToCameraXZ, cursorToCamera);
//
//            //Rotate in Y direction
//            if (cursorToCamera.y < 0) {
//                glRotatef(acos(angleCosine) * DEGREES_PER_RADIAN, 1, 0, 0);
//            } else {
//                glRotatef(acos(angleCosine) * DEGREES_PER_RADIAN, -1, 0, 0);
//            }
//
//            glBegin(GL_QUADS);
//
//            glColor4f(RETICLE_COLOR[0], RETICLE_COLOR[1], RETICLE_COLOR[2], _alpha);
//
//            glTexCoord2f(0.0f, 0.0f); glVertex3f(cursorVerts[0].x, cursorVerts[0].y, cursorVerts[0].z);
//            glTexCoord2f(1.0f, 0.0f); glVertex3f(cursorVerts[1].x, cursorVerts[1].y, cursorVerts[1].z);
//            glTexCoord2f(1.0f, 1.0f); glVertex3f(cursorVerts[2].x, cursorVerts[2].y, cursorVerts[2].z);
//            glTexCoord2f(0.0f, 1.0f); glVertex3f(cursorVerts[3].x, cursorVerts[3].y, cursorVerts[3].z);
//
//            glEnd();
//
//            glPopMatrix();
//        } 
    }

    //Mouse Pointer
    if (_reticleActive[MOUSE]) {
        glm::vec2 projection = screenToSpherical(_reticulePosition[MOUSE]);
        glm::quat orientation(glm::vec3(-projection.y, projection.x, 0.0f));
        renderReticule(orientation, _alpha);
    }

    glEnable(GL_DEPTH_TEST);
}

//Renders a small magnification of the currently bound texture at the coordinates
void ApplicationOverlay::renderMagnifier(glm::vec2 magPos, float sizeMult, bool showBorder) const {
    Application* application = Application::getInstance();
    GLCanvas* glWidget = application->getGLWidget();
    
    const int widgetWidth = glWidget->width();
    const int widgetHeight = glWidget->height();
    
    const float halfWidth = (MAGNIFY_WIDTH / _textureAspectRatio) * sizeMult / 2.0f;
    const float halfHeight = MAGNIFY_HEIGHT * sizeMult / 2.0f;
    // Magnification Texture Coordinates
    const float magnifyULeft = (magPos.x - halfWidth) / (float)widgetWidth;
    const float magnifyURight = (magPos.x + halfWidth) / (float)widgetWidth;
    const float magnifyVTop = 1.0f - (magPos.y - halfHeight) / (float)widgetHeight;
    const float magnifyVBottom = 1.0f - (magPos.y + halfHeight) / (float)widgetHeight;
    
    const float newHalfWidth = halfWidth * MAGNIFY_MULT;
    const float newHalfHeight = halfHeight * MAGNIFY_MULT;
    //Get yaw / pitch value for the corners
    const glm::vec2 topLeftYawPitch = overlayToSpherical(glm::vec2(magPos.x - newHalfWidth,
                                                                   magPos.y - newHalfHeight));
    const glm::vec2 bottomRightYawPitch = overlayToSpherical(glm::vec2(magPos.x + newHalfWidth,
                                                                       magPos.y + newHalfHeight));
    
    const glm::vec3 bottomLeft = getPoint(topLeftYawPitch.x, bottomRightYawPitch.y);
    const glm::vec3 bottomRight = getPoint(bottomRightYawPitch.x, bottomRightYawPitch.y);
    const glm::vec3 topLeft = getPoint(topLeftYawPitch.x, topLeftYawPitch.y);
    const glm::vec3 topRight = getPoint(bottomRightYawPitch.x, topLeftYawPitch.y);
    
    glPushMatrix(); {
        if (showBorder) {
            glDisable(GL_TEXTURE_2D);
            glLineWidth(1.0f);
            //Outer Line
            glBegin(GL_LINE_STRIP); {
                glColor4f(1.0f, 0.0f, 0.0f, _alpha);
                
                glVertex3f(topLeft.x, topLeft.y, topLeft.z);
                glVertex3f(bottomLeft.x, bottomLeft.y, bottomLeft.z);
                glVertex3f(bottomRight.x, bottomRight.y, bottomRight.z);
                glVertex3f(topRight.x, topRight.y, topRight.z);
                glVertex3f(topLeft.x, topLeft.y, topLeft.z);
            } glEnd();
            
            glEnable(GL_TEXTURE_2D);
        }
        glColor4f(1.0f, 1.0f, 1.0f, _alpha);
        
        glBegin(GL_QUADS); {
            glTexCoord2f(magnifyULeft, magnifyVBottom); glVertex3f(bottomLeft.x, bottomLeft.y, bottomLeft.z);
            glTexCoord2f(magnifyURight, magnifyVBottom); glVertex3f(bottomRight.x, bottomRight.y, bottomRight.z);
            glTexCoord2f(magnifyURight, magnifyVTop); glVertex3f(topRight.x, topRight.y, topRight.z);
            glTexCoord2f(magnifyULeft, magnifyVTop); glVertex3f(topLeft.x, topLeft.y, topLeft.z);
        } glEnd();
    } glPopMatrix();
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
    bool smallMirrorVisible = Menu::getInstance()->isOptionChecked(MenuOption::Mirror) && !OculusManager::isConnected();
    bool boxed = smallMirrorVisible &&
        !Menu::getInstance()->isOptionChecked(MenuOption::FullscreenMirror);
    if (boxed) {
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
    const float LOG2 = log(2.0f);
    const float METER_LOUDNESS_SCALE = 2.8f / 5.0f;
    const float LOG2_LOUDNESS_FLOOR = 11.0f;
    float audioLevel = 0.0f;
    float loudness = audio->getLastInputLoudness() + 1.0f;

    _trailingAudioLoudness = AUDIO_METER_AVERAGING * _trailingAudioLoudness + (1.0f - AUDIO_METER_AVERAGING) * loudness;
    float log2loudness = log(_trailingAudioLoudness) / LOG2;

    if (log2loudness <= LOG2_LOUDNESS_FLOOR) {
        audioLevel = (log2loudness / LOG2_LOUDNESS_FLOOR) * METER_LOUDNESS_SCALE * AUDIO_METER_SCALE_WIDTH;
    } else {
        audioLevel = (log2loudness - (LOG2_LOUDNESS_FLOOR - 1.0f)) * METER_LOUDNESS_SCALE * AUDIO_METER_SCALE_WIDTH;
    }
    if (audioLevel > AUDIO_METER_SCALE_WIDTH) {
        audioLevel = AUDIO_METER_SCALE_WIDTH;
    }
    bool isClipping = ((audio->getTimeSinceLastClip() > 0.0f) && (audio->getTimeSinceLastClip() < CLIPPING_INDICATOR_TIME));

    if ((audio->getTimeSinceLastClip() > 0.0f) && (audio->getTimeSinceLastClip() < CLIPPING_INDICATOR_TIME)) {
        const float MAX_MAGNITUDE = 0.7f;
        float magnitude = MAX_MAGNITUDE * (1 - audio->getTimeSinceLastClip() / CLIPPING_INDICATOR_TIME);
        renderCollisionOverlay(glWidget->width(), glWidget->height(), magnitude, 1.0f);
    }

    audio->renderToolBox(MIRROR_VIEW_LEFT_PADDING + AUDIO_METER_GAP, audioMeterY, boxed);

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



ApplicationOverlay::TexturedHemisphere::TexturedHemisphere() :
    _vertices(0),
    _indices(0),
    _framebufferObject(NULL),
    _vbo(0, 0) {
}

ApplicationOverlay::TexturedHemisphere::~TexturedHemisphere() {
    cleanupVBO();
    if (_framebufferObject != NULL) {
        delete _framebufferObject;
    }
}

void ApplicationOverlay::TexturedHemisphere::bind() {
    _framebufferObject->bind();
}

void ApplicationOverlay::TexturedHemisphere::release() {
    _framebufferObject->release();
}

void ApplicationOverlay::TexturedHemisphere::bindTexture() {
    glBindTexture(GL_TEXTURE_2D, _framebufferObject->texture());
}

void ApplicationOverlay::TexturedHemisphere::releaseTexture() {
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ApplicationOverlay::TexturedHemisphere::buildVBO(const float fov,
                                                      const float aspectRatio,
                                                      const int slices,
                                                      const int stacks) {
    if (fov >= PI) {
        qDebug() << "TexturedHemisphere::buildVBO(): FOV greater or equal than Pi will create issues";
    }
    // Cleanup old VBO if necessary
    cleanupVBO();
    
    //UV mapping source: http://www.mvps.org/directx/articles/spheremap.htm
    
    // Compute number of vertices needed
    _vertices = slices * stacks;
    
    // Compute vertices positions and texture UV coordinate
    TextureVertex* vertexData = new TextureVertex[_vertices];
    TextureVertex* vertexPtr = &vertexData[0];
    for (int i = 0; i < stacks; i++) {
        float stacksRatio = (float)i / (float)(stacks - 1); // First stack is 0.0f, last stack is 1.0f
        // abs(theta) <= fov / 2.0f
        float pitch = -fov * (stacksRatio - 0.5f);
        
        for (int j = 0; j < slices; j++) {
            float slicesRatio = (float)j / (float)(slices - 1); // First slice is 0.0f, last slice is 1.0f
            // abs(phi) <= fov * aspectRatio / 2.0f
            float yaw = -fov * aspectRatio * (slicesRatio - 0.5f);
            
            vertexPtr->position = getPoint(yaw, pitch);
            vertexPtr->uv.x = slicesRatio;
            vertexPtr->uv.y = stacksRatio;
            vertexPtr++;
        }
    }
    // Create and write to buffer
    glGenBuffers(1, &_vbo.first);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo.first);
    static const int BYTES_PER_VERTEX = sizeof(TextureVertex);
    glBufferData(GL_ARRAY_BUFFER, _vertices * BYTES_PER_VERTEX, vertexData, GL_STATIC_DRAW);
    delete[] vertexData;
    
    
    // Compute number of indices needed
    static const int VERTEX_PER_TRANGLE = 3;
    static const int TRIANGLE_PER_RECTANGLE = 2;
    int numberOfRectangles = (slices - 1) * (stacks - 1);
    _indices = numberOfRectangles * TRIANGLE_PER_RECTANGLE * VERTEX_PER_TRANGLE;
    
    // Compute indices order
    GLushort* indexData = new GLushort[_indices];
    GLushort* indexPtr = indexData;
    for (int i = 0; i < stacks - 1; i++) {
        for (int j = 0; j < slices - 1; j++) {
            GLushort bottomLeftIndex = i * slices + j;
            GLushort bottomRightIndex = bottomLeftIndex + 1;
            GLushort topLeftIndex = bottomLeftIndex + slices;
            GLushort topRightIndex = topLeftIndex + 1;
            
            *(indexPtr++) = topLeftIndex;
            *(indexPtr++) = bottomLeftIndex;
            *(indexPtr++) = topRightIndex;
            
            *(indexPtr++) = topRightIndex;
            *(indexPtr++) = bottomLeftIndex;
            *(indexPtr++) = bottomRightIndex;
        }
    }
    // Create and write to buffer
    glGenBuffers(1, &_vbo.second);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo.second);
    static const int BYTES_PER_INDEX = sizeof(GLushort);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indices * BYTES_PER_INDEX, indexData, GL_STATIC_DRAW);
    delete[] indexData;
}

void ApplicationOverlay::TexturedHemisphere::cleanupVBO() {
    if (_vbo.first != 0) {
        glDeleteBuffers(1, &_vbo.first);
        _vbo.first = 0;
    }
    if (_vbo.second != 0) {
        glDeleteBuffers(1, &_vbo.second);
        _vbo.second = 0;
    }
}

void ApplicationOverlay::TexturedHemisphere::buildFramebufferObject() {
    QSize size = Application::getInstance()->getGLWidget()->getDeviceSize();
    if (_framebufferObject != NULL && size == _framebufferObject->size()) {
        // Already build
        return;
    }
    
    if (_framebufferObject != NULL) {
        delete _framebufferObject;
    }
    
    _framebufferObject = new QOpenGLFramebufferObject(size);
    bindTexture();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    GLfloat borderColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    releaseTexture();
}

//Renders a hemisphere with texture coordinates.
void ApplicationOverlay::TexturedHemisphere::render() {
    if (_framebufferObject == NULL || _vbo.first == 0 || _vbo.second == 0) {
        qDebug() << "TexturedHemisphere::render(): Incorrect initialisation";
        return;
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, _vbo.first);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo.second);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    static const int STRIDE = sizeof(TextureVertex);
    static const void* VERTEX_POINTER = 0;
    static const void* TEX_COORD_POINTER = (void*)sizeof(glm::vec3);
    glVertexPointer(3, GL_FLOAT, STRIDE, VERTEX_POINTER);
    glTexCoordPointer(2, GL_FLOAT, STRIDE, TEX_COORD_POINTER);
    
    glDrawRangeElements(GL_TRIANGLES, 0, _vertices - 1, _indices, GL_UNSIGNED_SHORT, 0);
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


glm::vec2 ApplicationOverlay::screenToSpherical(glm::vec2 screenPos) const {
    QSize screenSize = Application::getInstance()->getGLWidget()->getDeviceSize();
    float yaw = -(screenPos.x / screenSize.width() - 0.5f) * MOUSE_YAW_RANGE;
    float pitch = (screenPos.y / screenSize.height() - 0.5f) * MOUSE_PITCH_RANGE;
    
    return glm::vec2(yaw, pitch);
}

glm::vec2 ApplicationOverlay::sphericalToScreen(glm::vec2 sphericalPos) const {
    QSize screenSize = Application::getInstance()->getGLWidget()->getDeviceSize();
    float x = (-sphericalPos.x / MOUSE_YAW_RANGE + 0.5f) * screenSize.width();
    float y = (sphericalPos.y / MOUSE_PITCH_RANGE + 0.5f) * screenSize.height();
    
    return glm::vec2(x, y);
}

glm::vec2 ApplicationOverlay::sphericalToOverlay(glm::vec2 sphericalPos) const {
    QSize screenSize = Application::getInstance()->getGLWidget()->getDeviceSize();
    float x = (-sphericalPos.x / (_textureFov * _textureAspectRatio) + 0.5f) * screenSize.width();
    float y = (sphericalPos.y / _textureFov + 0.5f) * screenSize.height();
    
    return glm::vec2(x, y);
}

glm::vec2 ApplicationOverlay::overlayToSpherical(glm::vec2 overlayPos) const {
    QSize screenSize = Application::getInstance()->getGLWidget()->getDeviceSize();
    float yaw = -(overlayPos.x / screenSize.width() - 0.5f) * _textureFov * _textureAspectRatio;
    float pitch = (overlayPos.y / screenSize.height() - 0.5f) * _textureFov;
    
    return glm::vec2(yaw, pitch);
}

glm::vec2 ApplicationOverlay::screenToOverlay(glm::vec2 screenPos) const {
    return sphericalToOverlay(screenToSpherical(screenPos));
}

glm::vec2 ApplicationOverlay::overlayToScreen(glm::vec2 overlayPos) const {
    return sphericalToScreen(overlayToSpherical(overlayPos));
}
