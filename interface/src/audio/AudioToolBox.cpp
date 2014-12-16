//
//  AudioToolBox.cpp
//  interface/src/audio
//
//  Created by Stephen Birarda on 2014-12-16.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InterfaceConfig.h"

#include "Application.h"
#include "Audio.h"

#include "AudioToolBox.h"

// Mute icon configration
const int MUTE_ICON_SIZE = 24;

AudioToolBox::AudioToolBox() :
    _iconPulseTimeReference(usecTimestampNow())
{
    Application* app = Application::getInstance();
    _micTextureId = app->getGLWidget()->bindTexture(QImage(Application::resourcesPath() + "images/mic.svg"));
    _muteTextureId = app->getGLWidget()->bindTexture(QImage(Application::resourcesPath() + "images/mic-mute.svg"));
    _boxTextureId = app->getGLWidget()->bindTexture(QImage(Application::resourcesPath() + "images/audio-box.svg"));
}

bool AudioToolBox::mousePressEvent(int x, int y) {
    if (_iconBounds.contains(x, y)) {
        DependencyManager::get<Audio>()->toggleMute();
        return true;
    }
    return false;
}

void AudioToolBox::render(int x, int y, bool boxed) {
    
    glEnable(GL_TEXTURE_2D);
    
    Audio* audioIO = DependencyManager::get<Audio>();
    
    if (boxed) {
        bool isClipping = ((audioIO->getTimeSinceLastClip() > 0.0f) && (audioIO->getTimeSinceLastClip() < 1.0f));
        const int BOX_LEFT_PADDING = 5;
        const int BOX_TOP_PADDING = 10;
        const int BOX_WIDTH = 266;
        const int BOX_HEIGHT = 44;
        
        QRect boxBounds = QRect(x - BOX_LEFT_PADDING, y - BOX_TOP_PADDING, BOX_WIDTH, BOX_HEIGHT);
        
        glBindTexture(GL_TEXTURE_2D, _boxTextureId);
        
        if (isClipping) {
            glColor3f(1.0f, 0.0f, 0.0f);
        } else {
            glColor3f(0.41f, 0.41f, 0.41f);
        }
        glBegin(GL_QUADS);
        
        glTexCoord2f(1, 1);
        glVertex2f(boxBounds.left(), boxBounds.top());
        
        glTexCoord2f(0, 1);
        glVertex2f(boxBounds.right(), boxBounds.top());
        
        glTexCoord2f(0, 0);
        glVertex2f(boxBounds.right(), boxBounds.bottom());
        
        glTexCoord2f(1, 0);
        glVertex2f(boxBounds.left(), boxBounds.bottom());
        
        glEnd();
    }
    
    float iconColor = 1.0f;
    
    _iconBounds = QRect(x, y, MUTE_ICON_SIZE, MUTE_ICON_SIZE);
    if (!audioIO->isMuted()) {
        glBindTexture(GL_TEXTURE_2D, _micTextureId);
        iconColor = 1.0f;
    } else {
        glBindTexture(GL_TEXTURE_2D, _muteTextureId);
        
        // Make muted icon pulsate
        static const float PULSE_MIN = 0.4f;
        static const float PULSE_MAX = 1.0f;
        static const float PULSE_FREQUENCY = 1.0f; // in Hz
        qint64 now = usecTimestampNow();
        if (now - _iconPulseTimeReference > (qint64)USECS_PER_SECOND) {
            // Prevents t from getting too big, which would diminish glm::cos precision
            _iconPulseTimeReference = now - ((now - _iconPulseTimeReference) % USECS_PER_SECOND);
        }
        float t = (float)(now - _iconPulseTimeReference) / (float)USECS_PER_SECOND;
        float pulseFactor = (glm::cos(t * PULSE_FREQUENCY * 2.0f * PI) + 1.0f) / 2.0f;
        iconColor = PULSE_MIN + (PULSE_MAX - PULSE_MIN) * pulseFactor;
    }
    
    glColor3f(iconColor, iconColor, iconColor);
    glBegin(GL_QUADS);
    
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(_iconBounds.left(), _iconBounds.top());
    
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(_iconBounds.right(), _iconBounds.top());
    
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(_iconBounds.right(), _iconBounds.bottom());
    
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(_iconBounds.left(), _iconBounds.bottom());
    
    glEnd();
    
    glDisable(GL_TEXTURE_2D);
}