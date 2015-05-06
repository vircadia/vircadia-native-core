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

#include "gpu/GLBackend.h"
#include "Application.h"
#include "CameraToolBox.h"
#include "FaceTracker.h"


CameraToolBox::CameraToolBox() :
    _iconPulseTimeReference(usecTimestampNow()),
    _doubleClickTimer(NULL)
{
}

CameraToolBox::~CameraToolBox() {
    if (_doubleClickTimer) {
        _doubleClickTimer->stop();
        delete _doubleClickTimer;
    }
}

bool CameraToolBox::mousePressEvent(int x, int y) {
    if (_iconBounds.contains(x, y)) {
        if (!_doubleClickTimer) {
            // Toggle mute after waiting to check that it's not a double-click.
            const int DOUBLE_CLICK_WAIT = 200;  // ms
            _doubleClickTimer = new QTimer(this);
            connect(_doubleClickTimer, SIGNAL(timeout()), this, SLOT(toggleMute()));
            _doubleClickTimer->setSingleShot(true);
            _doubleClickTimer->setInterval(DOUBLE_CLICK_WAIT);
            _doubleClickTimer->start();
        }
        return true;
    }
    return false;
}

bool CameraToolBox::mouseDoublePressEvent(int x, int y) {
    if (_iconBounds.contains(x, y)) {
        if (_doubleClickTimer) {
            _doubleClickTimer->stop();
            delete _doubleClickTimer;
            _doubleClickTimer = NULL;
        }
        Application::getInstance()->resetSensors();
        return true;
    }
    return false;
}

void CameraToolBox::toggleMute() {
    delete _doubleClickTimer;
    _doubleClickTimer = NULL;

    FaceTracker* faceTracker = Application::getInstance()->getSelectedFaceTracker();
    if (faceTracker) {
        faceTracker->toggleMute();
    }
}

void CameraToolBox::render(int x, int y, bool boxed) {
    glEnable(GL_TEXTURE_2D);
    
    if (!_enabledTexture) {
        _enabledTexture = DependencyManager::get<TextureCache>()->getImageTexture(PathUtils::resourcesPath() + "images/face.svg");
    }
    if (!_mutedTexture) {
        _mutedTexture = DependencyManager::get<TextureCache>()->getImageTexture(PathUtils::resourcesPath() + "images/face-mute.svg");
    }
    
    const int MUTE_ICON_SIZE = 24;
    _iconBounds = QRect(x, y, MUTE_ICON_SIZE, MUTE_ICON_SIZE);
    float iconColor = 1.0f;
    if (!Menu::getInstance()->isOptionChecked(MenuOption::MuteFaceTracking)) {
        glBindTexture(GL_TEXTURE_2D, gpu::GLBackend::getTextureID(_enabledTexture));
    } else {
        glBindTexture(GL_TEXTURE_2D, gpu::GLBackend::getTextureID(_mutedTexture));
        
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