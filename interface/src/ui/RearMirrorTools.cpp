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
#include <SharedUtil.h>
#include <gpu/GLBackend.h>

#include "Application.h"
#include "RearMirrorTools.h"
#include "Util.h"

const int ICON_SIZE = 24;
const int ICON_PADDING = 5;

const char SETTINGS_GROUP_NAME[] = "Rear View Tools";
const char ZOOM_LEVEL_SETTINGS[] = "ZoomLevel";
Setting::Handle<int> RearMirrorTools::rearViewZoomLevel(QStringList() << SETTINGS_GROUP_NAME << ZOOM_LEVEL_SETTINGS,
                                                        ZoomLevel::HEAD);

RearMirrorTools::RearMirrorTools(QRect& bounds) :
    _bounds(bounds),
    _windowed(false),
    _fullScreen(false)
{
    auto textureCache = DependencyManager::get<TextureCache>();
    _closeTexture = textureCache->getImageTexture(PathUtils::resourcesPath() + "images/close.svg");

    _zoomHeadTexture = textureCache->getImageTexture(PathUtils::resourcesPath() + "images/plus.svg");
    _zoomBodyTexture = textureCache->getImageTexture(PathUtils::resourcesPath() + "images/minus.svg");

    _shrinkIconRect = QRect(ICON_PADDING, ICON_PADDING, ICON_SIZE, ICON_SIZE);
    _closeIconRect = QRect(_bounds.left() + ICON_PADDING, _bounds.top() + ICON_PADDING, ICON_SIZE, ICON_SIZE);
    _resetIconRect = QRect(_bounds.width() - ICON_SIZE - ICON_PADDING, _bounds.top() + ICON_PADDING, ICON_SIZE, ICON_SIZE);
    _bodyZoomIconRect = QRect(_bounds.width() - ICON_SIZE - ICON_PADDING, _bounds.bottom() - ICON_PADDING - ICON_SIZE, ICON_SIZE, ICON_SIZE);
    _headZoomIconRect = QRect(_bounds.left() + ICON_PADDING, _bounds.bottom() - ICON_PADDING - ICON_SIZE, ICON_SIZE, ICON_SIZE);
}

void RearMirrorTools::render(bool fullScreen, const QPoint & mousePosition) {
    if (fullScreen) {
        _fullScreen = true;
        displayIcon(QRect(QPoint(), qApp->getDeviceSize()), _shrinkIconRect, _closeTexture);
    } else {
        // render rear view tools if mouse is in the bounds
        _windowed = _bounds.contains(mousePosition);
        if (_windowed) {
            displayIcon(_bounds, _closeIconRect, _closeTexture);

            ZoomLevel zoomLevel = (ZoomLevel)rearViewZoomLevel.get();
            displayIcon(_bounds, _headZoomIconRect, _zoomHeadTexture, zoomLevel == HEAD);
            displayIcon(_bounds, _bodyZoomIconRect, _zoomBodyTexture, zoomLevel == BODY);
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
            rearViewZoomLevel.set(HEAD);
            return true;
        }
        
        if (_bodyZoomIconRect.contains(x, y)) {
            rearViewZoomLevel.set(BODY);
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

void RearMirrorTools::displayIcon(QRect bounds, QRect iconBounds, const gpu::TexturePointer& texture, bool selected) {

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    glOrtho(bounds.left(), bounds.right(), bounds.bottom(), bounds.top(), -1.0, 1.0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    glm::vec4 quadColor;
    if (selected) {
        quadColor = glm::vec4(.5f, .5f, .5f, 1.0f);
    } else {
        quadColor = glm::vec4(1, 1, 1, 1);
    }

    glBindTexture(GL_TEXTURE_2D, gpu::GLBackend::getTextureID(texture));
   
    glm::vec2 topLeft(iconBounds.left(), iconBounds.top());
    glm::vec2 bottomRight(iconBounds.right(), iconBounds.bottom());
    static const glm::vec2 texCoordTopLeft(0.0f, 1.0f);
    static const glm::vec2 texCoordBottomRight(1.0f, 0.0f);

    DependencyManager::get<GeometryCache>()->renderQuad(topLeft, bottomRight, texCoordTopLeft, texCoordBottomRight, quadColor);
    
    glPopMatrix();
    
    glMatrixMode(GL_MODELVIEW);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}
