//
//  CameraToolBox.cpp
//  interface/src/devices
//
//  Created by David Rowe on 30 Apr 2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InterfaceConfig.h"

#include <GLCanvas.h>
#include <PathUtils.h>

#include "Application.h"
#include "CameraToolBox.h"
#include "FaceTracker.h"


CameraToolBox::CameraToolBox() :
    _iconPulseTimeReference(usecTimestampNow())
{
}

bool CameraToolBox::mousePressEvent(int x, int y) {
    if (_iconBounds.contains(x, y)) {
        FaceTracker* faceTracker = Application::getInstance()->getActiveFaceTracker();
        if (faceTracker) {
            faceTracker->toggleMute();
        }
        return true;
    }
    return false;
}

void CameraToolBox::render(int x, int y, bool boxed) {
    glEnable(GL_TEXTURE_2D);
    
    auto glCanvas = Application::getInstance()->getGLWidget();
    if (_enabledTextureId == 0) {
        _enabledTextureId =  glCanvas->bindTexture(QImage(PathUtils::resourcesPath() + "images/mic.svg"));
    }
    if (_mutedTextureId == 0) {
        _mutedTextureId =  glCanvas->bindTexture(QImage(PathUtils::resourcesPath() + "images/mic-mute.svg"));
    }
    
    const int MUTE_ICON_SIZE = 24;
    _iconBounds = QRect(x, y, MUTE_ICON_SIZE, MUTE_ICON_SIZE);
    float iconColor = 1.0f;
    if (!Menu::getInstance()->isOptionChecked(MenuOption::MuteFaceTracking)) {
        glBindTexture(GL_TEXTURE_2D, _enabledTextureId);
    } else {
        glBindTexture(GL_TEXTURE_2D, _mutedTextureId);
        
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
    
    glDisable(GL_TEXTURE_2D);
}