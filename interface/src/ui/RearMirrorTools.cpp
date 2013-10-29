//
//  RearMirrorTools.cpp
//  interface
//
//  Created by stojce on 23.10.2013.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include "RearMirrorTools.h"
#include <SharedUtil.h>
#include <QMouseEvent>

const int ICON_SIZE = 16;
const int ICON_PADDING = 5;

RearMirrorTools::RearMirrorTools(QGLWidget* parent, QRect& bounds) : _parent(parent), _bounds(bounds), _windowed(false), _fullScreen(false) {
    switchToResourcesParentIfRequired();
    _closeTextureId = _parent->bindTexture(QImage("./resources/images/close.png"));
    _resetTextureId = _parent->bindTexture(QImage("./resources/images/reset.png"));
};

void RearMirrorTools::render(bool fullScreen) {
    if (fullScreen) {
        _fullScreen = true;
        displayIcon(_parent->geometry(), ICON_PADDING, ICON_PADDING, _closeTextureId);
    } else {
        // render rear view tools if mouse is in the bounds
        QPoint mousePosition = _parent->mapFromGlobal(QCursor::pos());
        _windowed = _bounds.contains(mousePosition.x(), mousePosition.y());
        if (_windowed) {
            displayIcon(_bounds, _bounds.left() + ICON_PADDING, _bounds.top() + ICON_PADDING, _closeTextureId);
            displayIcon(_bounds, _bounds.width() - ICON_SIZE - ICON_PADDING, _bounds.top() + ICON_PADDING, _resetTextureId);
        }
    }
}

bool RearMirrorTools::mousePressEvent(int x, int y) {
    if (_windowed) {
        QRect closeIconRect = QRect(_bounds.left() + ICON_PADDING, _bounds.top() + ICON_PADDING, ICON_SIZE, ICON_SIZE);
        if (closeIconRect.contains(x, y)) {
            _windowed = false;
            emit closeView();
            return true;
        }
        
        QRect resetIconRect = QRect(_bounds.width() - ICON_SIZE - ICON_PADDING, _bounds.top() + ICON_PADDING, ICON_SIZE, ICON_SIZE);
        if (resetIconRect.contains(x, y)) {
            emit resetView();
            return true;
        }
        
        if (_bounds.contains(x, y)) {
            _windowed = false;
            emit restoreView();
            return true;
        }
    }
    
    if (_fullScreen) {
        QRect shrinkIconRect = QRect(ICON_PADDING, ICON_PADDING, ICON_SIZE, ICON_SIZE);
        if (shrinkIconRect.contains(x, y)) {
            _fullScreen = false;
            emit shrinkView();
            return true;
        }
    }
    return false;
}

void RearMirrorTools::displayIcon(QRect bounds, int left, int top, GLuint textureId) {

    int twp =  ICON_SIZE + top;
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    gluOrtho2D(bounds.left(), bounds.right(), bounds.bottom(), bounds.top());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    
    glColor3f(1, 1, 1);
    
    glBindTexture(GL_TEXTURE_2D, textureId);
    glBegin(GL_QUADS);
    {
        glTexCoord2f(0, 0);
        glVertex2f(left, top);
        
        glTexCoord2f(1, 0);
        glVertex2f(ICON_SIZE + left, top);
        
        glTexCoord2f(1, 1);
        glVertex2f(ICON_SIZE + left, twp);
        
        glTexCoord2f(0, 1);
        glVertex2f(left, twp);
    }
    glEnd();
    glPopMatrix();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}
