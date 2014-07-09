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

// Renders the overlays either to a texture or to the screen
void ApplicationOverlay::renderOverlay(bool renderToTexture) {

    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), "ApplicationOverlay::displayOverlay()");

    _textureFov = Menu::getInstance()->getOculusUIAngularSize() * RADIANS_PER_DEGREE;

    Application* application = Application::getInstance();

    Overlays& overlays = application->getOverlays();
    QGLWidget* glWidget = application->getGLWidget();
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
    QGLWidget* glWidget = application->getGLWidget();

    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, getFramebufferObject()->texture());

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();

    glLoadIdentity();
    gluOrtho2D(0, glWidget->width(), glWidget->height(), 0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);

    glBegin(GL_QUADS);
    glColor4f(1.0f, 1.0f, 1.0f, _alpha);
    glTexCoord2f(0, 0); glVertex2i(0, glWidget->height());
    glTexCoord2f(1, 0); glVertex2i(glWidget->width(), glWidget->height());
    glTexCoord2f(1, 1); glVertex2i(glWidget->width(), 0);
    glTexCoord2f(0, 1); glVertex2i(0, 0);
    glEnd();
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
}

void ApplicationOverlay::computeOculusPickRay(float x, float y, glm::vec3& direction) const {
    glm::quat rot = Application::getInstance()->getAvatar()->getOrientation();

    //invert y direction
    y = 1.0 - y;

    //Get position on hemisphere UI
    x = sin((x - 0.5f) * _textureFov);
    y = sin((y - 0.5f) * _textureFov);

    float dist = sqrt(x * x + y * y);
    float z = -sqrt(1.0f - dist * dist);

    //Rotate the UI pick ray by the avatar orientation
    direction = glm::normalize(rot * glm::vec3(x, y, z));
}

// Calculates the click location on the screen by taking into account any
// opened magnification windows.
void ApplicationOverlay::getClickLocation(int &x, int &y) const {
    int dx;
    int dy;
    const float xRange = MAGNIFY_WIDTH * MAGNIFY_MULT / 2.0f;
    const float yRange = MAGNIFY_WIDTH * MAGNIFY_MULT / 2.0f;

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

// Draws the FBO texture for Oculus rift.
void ApplicationOverlay::displayOverlayTextureOculus(Camera& whichCamera) {

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

    glDepthMask(GL_TRUE);
    
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.01f);

    //Update and draw the magnifiers
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

    glDepthMask(GL_FALSE);   
    glDisable(GL_ALPHA_TEST);

    glColor4f(1.0f, 1.0f, 1.0f, _alpha);

    renderTexturedHemisphere();
   
    renderControllerPointersOculus();

    glPopMatrix();

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
 //   fov -= RADIANS_PER_DEGREE * 2.5f; //reduce by 5 degrees so it fits in the view
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
    

    if (OculusManager::isConnected() && application->getLastMouseMoveType() == QEvent::MouseMove) {
        //If we are in oculus, render reticle later
        _reticleActive[MOUSE] = true;
        _magActive[MOUSE] = true;
        _mouseX[MOUSE] = application->getMouseX();
        _mouseY[MOUSE] = application->getMouseY();
        _magX[MOUSE] = _mouseX[MOUSE];
        _magY[MOUSE] = _mouseY[MOUSE];

        _reticleActive[LEFT_CONTROLLER] = false;
        _reticleActive[RIGHT_CONTROLLER] = false;
        
    } else if (application->getLastMouseMoveType() == CONTROLLER_MOVE_EVENT && Menu::getInstance()->isOptionChecked(MenuOption::SixenseMouseInput)) {
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
    QGLWidget* glWidget = application->getGLWidget();
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


        // Get directon relative to avatar orientation
        glm::vec3 direction = glm::inverse(myAvatar->getOrientation()) * palmData->getFingerDirection();

        // Get the angles, scaled between (-0.5,0.5)
        float xAngle = (atan2(direction.z, direction.x) + M_PI_2) ;
        float yAngle = 0.5f - ((atan2(direction.z, direction.y) + M_PI_2));

        // Get the pixel range over which the xAngle and yAngle are scaled
        float cursorRange = glWidget->width() * application->getSixenseManager()->getCursorPixelRangeMult();

        int mouseX = glWidget->width() / 2.0f + cursorRange * xAngle;
        int mouseY = glWidget->height() / 2.0f + cursorRange * yAngle;

        //If the cursor is out of the screen then don't render it
        if (mouseX < 0 || mouseX >= glWidget->width() || mouseY < 0 || mouseY >= glWidget->height()) {
            _reticleActive[index] = false;
            continue;
        }
        _reticleActive[index] = true;
     
        //if we have the oculus, we should make the cursor smaller since it will be
        //magnified
        if (OculusManager::isConnected()) {
          
            _mouseX[index] = mouseX;
            _mouseY[index] = mouseY;

            //When button 2 is pressed we drag the mag window
            if (isPressed[index]) {
                _magActive[index] = true;
                _magX[index] = mouseX;
                _magY[index] = mouseY;
            }

            // If oculus is enabled, we draw the crosshairs later
            continue;
        }

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

void ApplicationOverlay::renderControllerPointersOculus() {
    Application* application = Application::getInstance();
    QGLWidget* glWidget = application->getGLWidget();

    const int widgetWidth = glWidget->width();
    const int widgetHeight = glWidget->height();
  
    const float reticleSize = 50.0f;

    glBindTexture(GL_TEXTURE_2D, _crosshairTexture);
    glDisable(GL_DEPTH_TEST);
    
    for (int i = 0; i < NUMBER_OF_MAGNIFIERS; i++) {

        //Dont render the reticle if its inactive
        if (!_reticleActive[i]) {
            continue;
        }

        float mouseX = (float)_mouseX[i];
        float mouseY = (float)_mouseY[i];
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
            
        glTexCoord2f(0.0f, 0.0f); glVertex3f(lX, tY, -tlZ);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(rX, tY, -trZ);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(rX, bY, -brZ);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(lX, bY, -blZ);

        glEnd();
        
    }
    glEnable(GL_DEPTH_TEST);
}

//Renders a small magnification of the currently bound texture at the coordinates
void ApplicationOverlay::renderMagnifier(int mouseX, int mouseY, float sizeMult, bool showBorder) const
{
    Application* application = Application::getInstance();
    QGLWidget* glWidget = application->getGLWidget();

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
    float lX = sin((newULeft - 0.5f) * _textureFov);
    float rX = sin((newURight - 0.5f) * _textureFov);
    float bY = sin((newVBottom - 0.5f) * _textureFov);
    float tY = sin((newVTop - 0.5f) * _textureFov);

    float blZ, tlZ, brZ, trZ;

    float dist;
    float discriminant;
    //Bottom Left
    dist = sqrt(lX * lX + bY * bY);
    discriminant = 1.0f - dist * dist;
    if (discriminant > 0) {
        blZ = sqrt(discriminant);
    } else {
        blZ = 0;
    }
    //Top Left
    dist = sqrt(lX * lX + tY * tY);
    discriminant = 1.0f - dist * dist;
    if (discriminant > 0) {
        tlZ = sqrt(discriminant);
    } else {
        tlZ = 0;
    }
    //Bottom Right
    dist = sqrt(rX * rX + bY * bY);
    discriminant = 1.0f - dist * dist;
    if (discriminant > 0) {
        brZ = sqrt(discriminant);
    } else {
        brZ = 0;
    }
    //Top Right
    dist = sqrt(rX * rX + tY * tY);
    discriminant = 1.0f - dist * dist;
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

    QGLWidget* glWidget = application->getGLWidget();
    Audio* audio = application->getAudio();

    //  Display a single screen-size quad to create an alpha blended 'collision' flash
    if (audio->getCollisionFlashesScreen()) {
        float collisionSoundMagnitude = audio->getCollisionSoundMagnitude();
        const float VISIBLE_COLLISION_SOUND_MAGNITUDE = 0.5f;
        if (collisionSoundMagnitude > VISIBLE_COLLISION_SOUND_MAGNITUDE) {
            renderCollisionOverlay(glWidget->width(), glWidget->height(), audio->getCollisionSoundMagnitude());
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

    QGLWidget* glWidget = application->getGLWidget();
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
        Stats::getInstance()->display(WHITE_TEXT, horizontalOffset, application->getFps(), application->getPacketsPerSecond(), application->getBytesPerSecond(), voxelPacketsToProcess);
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
        drawText(glWidget->width() - 100, glWidget->height() - timerBottom, 0.30f, 0.0f, 0, frameTimer, WHITE_TEXT);
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
    
    glDrawRangeElements(GL_TRIANGLES, 0, vertices - 1, indices, GL_UNSIGNED_SHORT, 0);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

}

void ApplicationOverlay::resize() {
    if (_framebufferObject != NULL) {
        delete _framebufferObject;
        _framebufferObject = NULL;
    }
    // _framebufferObject is recreated at the correct size the next time it is accessed via getFramebufferObject().
}

QOpenGLFramebufferObject* ApplicationOverlay::getFramebufferObject() {
    if (!_framebufferObject) {
        _framebufferObject = new QOpenGLFramebufferObject(Application::getInstance()->getGLWidget()->size());
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

