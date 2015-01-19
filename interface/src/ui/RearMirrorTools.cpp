//
//  RearMirrorTools.cpp
//  interface/src/ui
//
//  Created by Stojce Slavkovski on 10/23/2013.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InterfaceConfig.h"

#include <QMouseEvent>

#include <PathUtils.h>
#include <Settings.h>
#include <SharedUtil.h>

#include "Application.h"
#include "RearMirrorTools.h"
#include "Util.h"


const char SETTINGS_GROUP_NAME[] = "Rear View Tools";
const char ZOOM_LEVEL_SETTINGS[] = "ZoomLevel";
const int ICON_SIZE = 24;
const int ICON_PADDING = 5;

RearMirrorTools::RearMirrorTools(QGLWidget* parent, QRect& bounds) :
    _parent(parent),
    _bounds(bounds),
    _windowed(false),
    _fullScreen(false)
{
    _closeTextureId = _parent->bindTexture(QImage(PathUtils::resourcesPath() + "images/close.svg"));

    _zoomHeadTextureId = _parent->bindTexture(QImage(PathUtils::resourcesPath() + "images/plus.svg"));
    _zoomBodyTextureId = _parent->bindTexture(QImage(PathUtils::resourcesPath() + "images/minus.svg"));

    _shrinkIconRect = QRect(ICON_PADDING, ICON_PADDING, ICON_SIZE, ICON_SIZE);
    _closeIconRect = QRect(_bounds.left() + ICON_PADDING, _bounds.top() + ICON_PADDING, ICON_SIZE, ICON_SIZE);
    _resetIconRect = QRect(_bounds.width() - ICON_SIZE - ICON_PADDING, _bounds.top() + ICON_PADDING, ICON_SIZE, ICON_SIZE);
    _bodyZoomIconRect = QRect(_bounds.width() - ICON_SIZE - ICON_PADDING, _bounds.bottom() - ICON_PADDING - ICON_SIZE, ICON_SIZE, ICON_SIZE);
    _headZoomIconRect = QRect(_bounds.left() + ICON_PADDING, _bounds.bottom() - ICON_PADDING - ICON_SIZE, ICON_SIZE, ICON_SIZE);
}

void RearMirrorTools::render(bool fullScreen) {
    if (fullScreen) {
        _fullScreen = true;
        displayIcon(_parent->geometry(), _shrinkIconRect, _closeTextureId);
    } else {
        // render rear view tools if mouse is in the bounds
        QPoint mousePosition = _parent->mapFromGlobal(QCursor::pos());
        _windowed = _bounds.contains(mousePosition.x(), mousePosition.y());
        if (_windowed) {
            displayIcon(_bounds, _closeIconRect, _closeTextureId);

            displayIcon(_bounds, _headZoomIconRect, _zoomHeadTextureId, getZoomLevel() == HEAD);
            displayIcon(_bounds, _bodyZoomIconRect, _zoomBodyTextureId, getZoomLevel() == BODY);
        }
    }
}

bool RearMirrorTools::mousePressEvent(int x, int y) {
    if (_windowed) {
        if (_closeIconRect.contains(x, y)) {
            _windowed = false;
            emit closeView();
            return true;
        }
        
        if (_headZoomIconRect.contains(x, y)) {
            setZoomLevel(HEAD);
            return true;
        }
        
        if (_bodyZoomIconRect.contains(x, y)) {
            setZoomLevel(BODY);
            return true;
        }

        if (_bounds.contains(x, y)) {
            _windowed = false;
            emit restoreView();
            return true;
        }
    }
    
    if (_fullScreen) {
        if (_shrinkIconRect.contains(x, y)) {
            _fullScreen = false;
            emit shrinkView();
            return true;
        }
    }
    return false;
}

ZoomLevel RearMirrorTools::getZoomLevel() {
    Settings settings;
    settings.beginGroup(SETTINGS_GROUP_NAME);
    ZoomLevel zoomLevel = (ZoomLevel)settings.value(ZOOM_LEVEL_SETTINGS, HEAD).toInt();
    settings.endGroup();
    
    return zoomLevel;
}

void RearMirrorTools::setZoomLevel(ZoomLevel zoomLevel) {
    Settings settings;
    settings.beginGroup(SETTINGS_GROUP_NAME);
    settings.setValue(ZOOM_LEVEL_SETTINGS, zoomLevel);
    settings.endGroup();
}

void RearMirrorTools::displayIcon(QRect bounds, QRect iconBounds, GLuint textureId, bool selected) {

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    glOrtho(bounds.left(), bounds.right(), bounds.bottom(), bounds.top(), -1.0, 1.0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    if (selected) {
        glColor3f(.5f, .5f, .5f);
    } else {
        glColor3f(1, 1, 1);
    }
    
    glBindTexture(GL_TEXTURE_2D, textureId);
   
    glm::vec2 topLeft(iconBounds.left(), iconBounds.top());
    glm::vec2 bottomRight(iconBounds.right(), iconBounds.bottom());
    glm::vec2 texCoordTopLeft(0.0f, 1.0f);
    glm::vec2 texCoordBottomRight(1.0f, 0.0f);

    DependencyManager::get<GeometryCache>()->renderQuad(topLeft, bottomRight, texCoordTopLeft, texCoordBottomRight);
    
    glPopMatrix();
    
    glMatrixMode(GL_MODELVIEW);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}
