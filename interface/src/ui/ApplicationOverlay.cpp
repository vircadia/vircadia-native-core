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

#include "ui/Stats.h"

ApplicationOverlay::ApplicationOverlay() : 
    _framebufferObject(NULL),
    _oculusAngle(65.0f * RADIANS_PER_DEGREE),
    _distance(0.5f),
    _uiType(HEMISPHERE) {

}

ApplicationOverlay::~ApplicationOverlay() {
    if (_framebufferObject != NULL) {
        delete _framebufferObject;
    }
}

const float WHITE_TEXT[] = { 0.93f, 0.93f, 0.93f };

void renderControllerPointer() {
    Application* application = Application::getInstance();
    QGLWidget* glWidget = application->getGLWidget();
    MyAvatar* myAvatar = application->getAvatar();

    const HandData* handData = Application::getInstance()->getAvatar()->getHandData();
    int numberOfPalms = handData->getNumPalms();

 
    int palmIndex;
    /*if (Menu::getInstance()->isOptionChecked(MenuOption::SixenseLeftHanded)) {
        palmIndex = 2;
    } else {
        palmIndex = 3;
    }*/
    for (palmIndex = 2; palmIndex < 4; palmIndex++) {
        const PalmData* palmData = NULL;

        if (palmIndex >= handData->getPalms().size()) {
            return;
        }

        if (handData->getPalms()[palmIndex].isActive()) {
            palmData = &handData->getPalms()[palmIndex];
        } else {
            return;
        }

        // Get directon relative to avatar orientation
        glm::vec3 direction = glm::inverse(myAvatar->getOrientation()) * palmData->getFingerDirection();

        // Get the angles, scaled between 0-1
        float xAngle = (atan2(direction.z, direction.x) + M_PI_2) + 0.5f;
        float yAngle = 1.0f - ((atan2(direction.z, direction.y) + M_PI_2) + 0.5f);

        float cursorRange = glWidget->width();

        int mouseX = cursorRange * xAngle;
        int mouseY = cursorRange * yAngle;

        if (mouseX < 0) {
            //mouseX = 0;
            continue;
        } else if (mouseX > glWidget->width()) {
            //mouseX = glWidget->width();
            continue;
        }
        if (mouseY < 0) {
            //mouseY = 0;
            continue;
        } else if (mouseY > glWidget->width()) {
            //mouseY = glWidget->width();
            continue;
        }

        const float pointerWidth = 40;
        const float pointerHeight = 40;
        const float crossPad = 16;

        mouseX -= pointerWidth / 2.0f;
        mouseY += pointerHeight / 2.0f;

        glBegin(GL_QUADS);

        glColor3f(0.0f, 0.0f, 1.0f);

        //Horizontal crosshair
        glVertex2i(mouseX, mouseY - crossPad);
        glVertex2i(mouseX + pointerWidth, mouseY - crossPad);
        glVertex2i(mouseX + pointerWidth, mouseY - pointerHeight + crossPad);
        glVertex2i(mouseX, mouseY - pointerHeight + crossPad);

        //Vertical crosshair
        glVertex2i(mouseX + crossPad, mouseY);
        glVertex2i(mouseX + pointerWidth - crossPad, mouseY);
        glVertex2i(mouseX + pointerWidth - crossPad, mouseY - pointerHeight);
        glVertex2i(mouseX + crossPad, mouseY - pointerHeight);

        glEnd();
    }
}

// Renders the overlays either to a texture or to the screen
void ApplicationOverlay::renderOverlay(bool renderToTexture) {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), "ApplicationOverlay::displayOverlay()");

    Application* application = Application::getInstance();

    Overlays& overlays = application->getOverlays();
    QGLWidget* glWidget = application->getGLWidget();
    MyAvatar* myAvatar = application->getAvatar();
    Audio* audio = application->getAudio();
    const OctreePacketProcessor& octreePacketProcessor = application->getOctreePacketProcessor();
    BandwidthMeter* bandwidthMeter = application->getBandwidthMeter();
    NodeBounds& nodeBoundsDisplay = application->getNodeBoundsDisplay();

    int mouseX = application->getMouseX();
    int mouseY = application->getMouseY();
    bool renderPointer = renderToTexture;

    if (renderToTexture) {
        getFramebufferObject()->bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render 2D overlay:  I/O level bar graphs and text
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();

    glLoadIdentity();
    gluOrtho2D(0, glWidget->width(), glWidget->height(), 0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

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


    if (Menu::getInstance()->isOptionChecked(MenuOption::HeadMouse)) {
        myAvatar->renderHeadMouse(glWidget->width(), glWidget->height());
    }

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

    // give external parties a change to hook in
    emit application->renderingOverlay();

    overlays.render2D();

    // Render a crosshair over the pointer when in Oculus
    if (renderPointer) {
        const float pointerWidth = 10;
        const float pointerHeight = 10;
        const float crossPad = 4;

        mouseX -= pointerWidth / 2.0f;
        mouseY += pointerHeight / 2.0f;

        glBegin(GL_QUADS);
       
        glColor3f(1, 0, 0);
        
        //Horizontal crosshair
        glVertex2i(mouseX, mouseY - crossPad);
        glVertex2i(mouseX + pointerWidth, mouseY - crossPad);
        glVertex2i(mouseX + pointerWidth, mouseY - pointerHeight + crossPad);
        glVertex2i(mouseX, mouseY - pointerHeight + crossPad);

        //Vertical crosshair
        glVertex2i(mouseX + crossPad, mouseY);
        glVertex2i(mouseX + pointerWidth - crossPad, mouseY);
        glVertex2i(mouseX + pointerWidth - crossPad, mouseY - pointerHeight);
        glVertex2i(mouseX + crossPad, mouseY - pointerHeight);
      
        glEnd();
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::SixenseMouseInput)) {
        //only render controller pointer if we aren't already rendering a mouse pointer
        renderControllerPointer();
    }
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
void ApplicationOverlay::displayOverlayTexture(Camera& whichCamera) {

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

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2i(0, glWidget->height());
    glTexCoord2f(1, 0); glVertex2i(glWidget->width(), glWidget->height());
    glTexCoord2f(1, 1); glVertex2i(glWidget->width(), 0);
    glTexCoord2f(0, 1); glVertex2i(0, 0);
    glEnd();

    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
}

const float textureFov = PI / 2.5f;

void ApplicationOverlay::computeOculusPickRay(float x, float y, glm::vec3& direction) const {
    glm::quat rot = Application::getInstance()->getAvatar()->getOrientation();

    //invert y direction
    y = 1.0 - y;

    //Get position on hemisphere UI
    x = sin((x - 0.5f) * textureFov);
    y = sin((y - 0.5f) * textureFov);

    float dist = sqrt(x * x + y * y);
    float z = -sqrt(1.0f - dist * dist);

    //Rotate the UI pick ray by the avatar orientation
    direction = glm::normalize(rot * glm::vec3(x, y, z));
}

// Fast helper functions
inline float max(float a, float b) {
    return (a > b) ? a : b;
}

inline float min(float a, float b) {
    return (a < b) ? a : b;
}

// Draws the FBO texture for Oculus rift. TODO: Draw a curved texture instead of plane.
void ApplicationOverlay::displayOverlayTextureOculus(Camera& whichCamera) {

    Application* application = Application::getInstance();

    QGLWidget* glWidget = application->getGLWidget();
    MyAvatar* myAvatar = application->getAvatar();
    const glm::vec3& viewMatrixTranslation = application->getViewMatrixTranslation();

    int mouseX = application->getMouseX();
    int mouseY = application->getMouseY();
    const int widgetWidth = glWidget->width();
    const int widgetHeight = glWidget->height();
    float magnifyWidth = 80.0f;
    float magnifyHeight = 60.0f;
    const float magnification = 4.0f;

    // Get vertical FoV of the displayed overlay texture
    const float halfVerticalAngle = _oculusAngle / 2.0f;
    const float overlayAspectRatio = glWidget->width() / (float)glWidget->height();
    const float halfOverlayHeight = _distance * tan(halfVerticalAngle);
    const float overlayHeight = halfOverlayHeight * 2.0f;

    // The more vertices, the better the curve
    const int numHorizontalVertices = 20;
    const int numVerticalVertices = 20;
    // U texture coordinate width at each quad
    const float quadTexWidth = 1.0f / (numHorizontalVertices - 1);
    const float quadTexHeight = 1.0f / (numVerticalVertices - 1);

    // Get horizontal angle and angle increment from vertical angle and aspect ratio
    const float horizontalAngle = halfVerticalAngle * 2.0f * overlayAspectRatio;
    const float angleIncrement = horizontalAngle / (numHorizontalVertices - 1);
    const float halfHorizontalAngle = horizontalAngle / 2;

    const float verticalAngleIncrement = _oculusAngle / (numVerticalVertices - 1);

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

    glColor3f(1.0f, 1.0f, 1.0f);

    glDepthMask(GL_TRUE);
    
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.01f);

    //Draw the magnifying glass

    mouseX -= magnifyWidth / 2;
    mouseY -= magnifyHeight / 2;

    //clamp the magnification
    if (mouseX < 0) {
        magnifyWidth += mouseX;
        mouseX = 0;
    } else if (mouseX + magnifyWidth > widgetWidth) {
        magnifyWidth = widgetWidth - mouseX;
    }
    if (mouseY < 0) {
        magnifyHeight += mouseY;
        mouseY = 0;
    } else if (mouseY + magnifyHeight > widgetHeight) {
        magnifyHeight = widgetHeight - mouseY;
    }

    const float halfMagnifyHeight = magnifyHeight / 2.0f;

    float newWidth = magnifyWidth * magnification;
    float newHeight = magnifyHeight * magnification;

    // Magnification Texture Coordinates
    float magnifyULeft = mouseX / (float)widgetWidth;
    float magnifyURight = (mouseX + magnifyWidth) / (float)widgetWidth;
    float magnifyVBottom = 1.0f - mouseY / (float)widgetHeight;
    float magnifyVTop = 1.0f - (mouseY + magnifyHeight) / (float)widgetHeight;

    // Coordinates of magnification overlay
    float newMouseX = (mouseX + magnifyWidth / 2) - newWidth / 2.0f;
    float newMouseY = (mouseY + magnifyHeight / 2) + newHeight / 2.0f;

    // Get angle on the UI
    float leftAngle = (newMouseX / (float)widgetWidth) * horizontalAngle - halfHorizontalAngle;
    float rightAngle = ((newMouseX + newWidth) / (float)widgetWidth) * horizontalAngle - halfHorizontalAngle;

    float bottomAngle = (newMouseY / (float)widgetHeight) * _oculusAngle - halfVerticalAngle;
    float topAngle = ((newMouseY - newHeight) / (float)widgetHeight) * _oculusAngle - halfVerticalAngle;

    float leftX, rightX, leftZ, rightZ, topZ, bottomZ;
    
    // Get position on hemisphere using angle
    if (_uiType == HEMISPHERE) {

        //Get new UV coordinates from our magnification window
        float newULeft = newMouseX / widgetWidth;
        float newURight = (newMouseX + newWidth) / widgetWidth;
        float newVBottom = 1.0 - newMouseY / widgetHeight;
        float newVTop = 1.0 - (newMouseY - newHeight) / widgetHeight;

        // Project our position onto the hemisphere using the UV coordinates
        float lX = sin((newULeft - 0.5f) * textureFov);
        float rX = sin((newURight - 0.5f) * textureFov);
        float bY = sin((newVBottom - 0.5f) * textureFov);
        float tY = sin((newVTop - 0.5f) * textureFov);
        
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

        glTexCoord2f(magnifyULeft, magnifyVBottom); glVertex3f(lX, tY, -tlZ);
        glTexCoord2f(magnifyURight, magnifyVBottom); glVertex3f(rX, tY, -trZ);
        glTexCoord2f(magnifyURight, magnifyVTop); glVertex3f(rX, bY, -brZ);
        glTexCoord2f(magnifyULeft, magnifyVTop); glVertex3f(lX, bY, -blZ);

        glEnd();

    } else {
        leftX = sin(leftAngle) * _distance;
        rightX = sin(rightAngle) * _distance;
        leftZ = -cos(leftAngle) * _distance;
        rightZ = -cos(rightAngle) * _distance;
        if (_uiType == CURVED_SEMICIRCLE) {
            topZ = -cos(topAngle * overlayAspectRatio) * _distance;
            bottomZ = -cos(bottomAngle * overlayAspectRatio) * _distance;
        } else {
            // Dont want to use topZ or bottomZ for SEMICIRCLE
            topZ = -99999;
            bottomZ = -99999;
        }

        float bottomY = (1.0 - newMouseY / (float)widgetHeight) * halfOverlayHeight * 2.0f - halfOverlayHeight;
        float topY = bottomY + (newHeight / widgetHeight) * halfOverlayHeight * 2;

        //TODO: Remove immediate mode in favor of VBO
        glBegin(GL_QUADS);

        glTexCoord2f(magnifyULeft, magnifyVBottom); glVertex3f(leftX, topY, max(topZ, leftZ));
        glTexCoord2f(magnifyURight, magnifyVBottom); glVertex3f(rightX, topY, max(topZ, rightZ));
        glTexCoord2f(magnifyURight, magnifyVTop); glVertex3f(rightX, bottomY, max(bottomZ, rightZ));
        glTexCoord2f(magnifyULeft, magnifyVTop); glVertex3f(leftX, bottomY, max(bottomZ, leftZ));

        glEnd();
    }
    glDepthMask(GL_FALSE);   
    glDisable(GL_ALPHA_TEST);

    //TODO: Remove immediate mode in favor of VBO
    if (_uiType == HEMISPHERE) {
        renderTexturedHemisphere();
    } else{
        glBegin(GL_QUADS);
        // Place the vertices in a semicircle curve around the camera
        for (int i = 0; i < numHorizontalVertices - 1; i++) {
            for (int j = 0; j < numVerticalVertices - 1; j++) {

                // Calculate the X and Z coordinates from the angles and radius from camera
                leftX = sin(angleIncrement * i - halfHorizontalAngle) * _distance;
                rightX = sin(angleIncrement * (i + 1) - halfHorizontalAngle) * _distance;
                leftZ = -cos(angleIncrement * i - halfHorizontalAngle) * _distance;
                rightZ = -cos(angleIncrement * (i + 1) - halfHorizontalAngle) * _distance;
                if (_uiType == 2) {
                    topZ = -cos((verticalAngleIncrement * (j + 1) - halfVerticalAngle) * overlayAspectRatio) * _distance;
                    bottomZ = -cos((verticalAngleIncrement * j - halfVerticalAngle) * overlayAspectRatio) * _distance;
                } else {
                    topZ = -99999;
                    bottomZ = -99999;
                }

                glTexCoord2f(quadTexWidth * i, (j + 1) * quadTexHeight); 
                glVertex3f(leftX, (j + 1) * quadTexHeight * overlayHeight - halfOverlayHeight, max(topZ, leftZ));
                glTexCoord2f(quadTexWidth * (i + 1), (j + 1) * quadTexHeight); 
                glVertex3f(rightX, (j + 1) * quadTexHeight * overlayHeight - halfOverlayHeight, max(topZ, rightZ));
                glTexCoord2f(quadTexWidth * (i + 1), j * quadTexHeight); 
                glVertex3f(rightX, j * quadTexHeight * overlayHeight - halfOverlayHeight, max(bottomZ, rightZ));
                glTexCoord2f(quadTexWidth * i, j * quadTexHeight); 
                glVertex3f(leftX, j * quadTexHeight * overlayHeight - halfOverlayHeight, max(bottomZ, leftZ));
            }
        }

        glEnd();
    }

    glPopMatrix();

    glDepthMask(GL_TRUE);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);
    glEnable(GL_LIGHTING);

}

void ApplicationOverlay::renderTexturedHemisphere() {
    const int slices = 80;
    const int stacks = 80;

    static VerticesIndices vbo(0, 0);
    int vertices = slices * (stacks - 1) + 1;
    int indices = slices * 2 * 3 * (stacks - 2) + slices * 3;
    if (vbo.first == 0) {
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
                vertex->uv.x = asin(vertex->position.x) / (textureFov) + 0.5f;
                vertex->uv.y = asin(vertex->position.y) / (textureFov) + 0.5f;
                vertex++;
            }
        }
        vertex->position.x = 0.0f;
        vertex->position.y = 0.0f;
        vertex->position.z = -1.0f;
        vertex->uv.x = 0.5f;
        vertex->uv.y = 0.5f;
        vertex++;

        glGenBuffers(1, &vbo.first);
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

