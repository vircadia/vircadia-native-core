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

#include <AudioClient.h>
#include <GLCanvas.h>
#include <PathUtils.h>
#include <GeometryCache.h>
#include <gpu/GLBackend.h>

#include "Application.h"
#include "AudioToolBox.h"

// Mute icon configration
const int MUTE_ICON_SIZE = 24;

AudioToolBox::AudioToolBox() :
    _iconPulseTimeReference(usecTimestampNow())
{
}

bool AudioToolBox::mousePressEvent(int x, int y) {
    if (_iconBounds.contains(x, y)) {
        DependencyManager::get<AudioClient>()->toggleMute();
        return true;
    }
    return false;
}

void AudioToolBox::render(int x, int y, int padding, bool boxed) {
    glEnable(GL_TEXTURE_2D);
    
    if (!_micTexture) {
        _micTexture = DependencyManager::get<TextureCache>()->getImageTexture(PathUtils::resourcesPath() + "images/mic.svg");
    }
    if (!_muteTexture) {
        _muteTexture = DependencyManager::get<TextureCache>()->getImageTexture(PathUtils::resourcesPath() + "images/mic-mute.svg");
    }
    if (_boxTexture) {
        _boxTexture = DependencyManager::get<TextureCache>()->getImageTexture(PathUtils::resourcesPath() + "images/audio-box.svg");
    }
    
    auto audioIO = DependencyManager::get<AudioClient>();
    
    if (boxed) {
        bool isClipping = ((audioIO->getTimeSinceLastClip() > 0.0f) && (audioIO->getTimeSinceLastClip() < 1.0f));
        const int BOX_LEFT_PADDING = 5;
        const int BOX_TOP_PADDING = 10;
        const int BOX_WIDTH = 266;
        const int BOX_HEIGHT = 44;
        
        QRect boxBounds = QRect(x - BOX_LEFT_PADDING, y - BOX_TOP_PADDING, BOX_WIDTH, BOX_HEIGHT);
        glm::vec4 quadColor;
       
        if (isClipping) {
            quadColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        } else {
            quadColor = glm::vec4(0.41f, 0.41f, 0.41f, 1.0f);
        }
        glm::vec2 topLeft(boxBounds.left(), boxBounds.top());
        glm::vec2 bottomRight(boxBounds.right(), boxBounds.bottom());
        static const glm::vec2 texCoordTopLeft(1,1);
        static const glm::vec2 texCoordBottomRight(0, 0);
        glBindTexture(GL_TEXTURE_2D, gpu::GLBackend::getTextureID(_boxTexture));
        DependencyManager::get<GeometryCache>()->renderQuad(topLeft, bottomRight, texCoordTopLeft, texCoordBottomRight, quadColor);
    }
    
    float iconColor = 1.0f;
    
    _iconBounds = QRect(x + padding, y, MUTE_ICON_SIZE, MUTE_ICON_SIZE);
    if (!audioIO->isMuted()) {
        glBindTexture(GL_TEXTURE_2D, gpu::GLBackend::getTextureID(_micTexture));
        iconColor = 1.0f;
    } else {
        glBindTexture(GL_TEXTURE_2D, gpu::GLBackend::getTextureID(_muteTexture));
        
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
    
    if (_boxQuadID == GeometryCache::UNKNOWN_ID) {
        _boxQuadID = DependencyManager::get<GeometryCache>()->allocateID();
    }

    DependencyManager::get<GeometryCache>()->renderQuad(topLeft, bottomRight, texCoordTopLeft, texCoordBottomRight, quadColor, _boxQuadID);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}