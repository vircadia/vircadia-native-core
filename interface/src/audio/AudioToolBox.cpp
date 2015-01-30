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

#include <GLCanvas.h>
#include <PathUtils.h>
#include <GeometryCache.h>

#include "Audio.h"

#include "AudioToolBox.h"

// Mute icon configration
const int MUTE_ICON_SIZE = 24;

AudioToolBox::AudioToolBox() :
    _iconPulseTimeReference(usecTimestampNow())
{
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
    
    auto glCanvas = DependencyManager::get<GLCanvas>();
    if (_micTextureId == 0) {
        _micTextureId =  glCanvas->bindTexture(QImage(PathUtils::resourcesPath() + "images/mic.svg"));
    }
    if (_muteTextureId == 0) {
        _muteTextureId =  glCanvas->bindTexture(QImage(PathUtils::resourcesPath() + "images/mic-mute.svg"));
    }
    if (_boxTextureId == 0) {
        _boxTextureId =  glCanvas->bindTexture(QImage(PathUtils::resourcesPath() + "images/audio-box.svg"));
    }
    
    auto audioIO = DependencyManager::get<Audio>();
    
    if (boxed) {
        bool isClipping = ((audioIO->getTimeSinceLastClip() > 0.0f) && (audioIO->getTimeSinceLastClip() < 1.0f));
        const int BOX_LEFT_PADDING = 5;
        const int BOX_TOP_PADDING = 10;
        const int BOX_WIDTH = 266;
        const int BOX_HEIGHT = 44;
        
        QRect boxBounds = QRect(x - BOX_LEFT_PADDING, y - BOX_TOP_PADDING, BOX_WIDTH, BOX_HEIGHT);
        
        glBindTexture(GL_TEXTURE_2D, _boxTextureId);
        
        glm::vec4 quadColor;
        
        if (isClipping) {
            quadColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        } else {
            quadColor = glm::vec4(0.41f, 0.41f, 0.41f, 1.0f);
        }
        glm::vec2 topLeft(boxBounds.left(), boxBounds.top());
        glm::vec2 bottomRight(boxBounds.right(), boxBounds.bottom());
        glm::vec2 texCoordTopLeft(1,1);
        glm::vec2 texCoordBottomRight(0,0);

        DependencyManager::get<GeometryCache>()->renderQuad(topLeft, bottomRight, texCoordTopLeft, texCoordBottomRight, quadColor);
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
    
    glm::vec4 quadColor(iconColor, iconColor, iconColor, 1.0f);

    glm::vec2 topLeft(_iconBounds.left(), _iconBounds.top());
    glm::vec2 bottomRight(_iconBounds.right(), _iconBounds.bottom());
    glm::vec2 texCoordTopLeft(1,1);
    glm::vec2 texCoordBottomRight(0,0);

    DependencyManager::get<GeometryCache>()->renderQuad(topLeft, bottomRight, texCoordTopLeft, texCoordBottomRight, quadColor);
    
    glDisable(GL_TEXTURE_2D);
}