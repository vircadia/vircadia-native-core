//
//  RearMirrorTools.cpp
//  interface
//
//  Created by stojce on 23.10.2013.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include "RearMirrorTools.h"
#include <SharedUtil.h>
#include <QMouseEvent>

const int MIRROR_ICON_SIZE = 16;
const int MIRROR_ICON_PADDING = 5;

RearMirrorTools::RearMirrorTools(QGLWidget* parent, QRect& bounds) : _parent(parent), _bounds(bounds), _visible(false) {
    switchToResourcesParentIfRequired();
    _closeTextureId = _parent->bindTexture(QImage("./resources/images/close.png"));
    _restoreTextureId = _parent->bindTexture(QImage("./resources/images/close.png"));
};

void RearMirrorTools::render() {
    // render rear view tools if mouse is in the bounds
    QPoint mousePosition = _parent->mapFromGlobal(QCursor::pos());
    _visible = _bounds.contains(mousePosition.x(), mousePosition.y());
    if (_visible) {
        displayTools();
    }
}

bool RearMirrorTools::mousePressEvent(int x, int y) {
    QRect closeIconRect = QRect(MIRROR_ICON_PADDING + _bounds.left(), MIRROR_ICON_PADDING + _bounds.top(), MIRROR_ICON_SIZE, MIRROR_ICON_SIZE);
    QRect restoreIconRect = QRect(_bounds.width() - _bounds.left() - MIRROR_ICON_PADDING, MIRROR_ICON_PADDING + _bounds.top(), MIRROR_ICON_SIZE, MIRROR_ICON_SIZE);
    
    if (closeIconRect.contains(x, y)) {
        emit closeView();
        return true;
    }
    
    if (restoreIconRect.contains(x, y)) {
        emit restoreView();
        return true;
    }
    
    return false;
}

void RearMirrorTools::displayTools() {
    int closeLeft = MIRROR_ICON_PADDING + _bounds.left();
    int resoreLeft = _bounds.width() - _bounds.left() - MIRROR_ICON_PADDING;
    
    int iconTop = MIRROR_ICON_PADDING + _bounds.top();
    int twp =  MIRROR_ICON_SIZE + iconTop;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(_bounds.left(), _bounds.right(), _bounds.bottom(), _bounds.top());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    glColor3f(1, 1, 1);

    glBindTexture(GL_TEXTURE_2D, _closeTextureId);
    glBegin(GL_QUADS);
    {
        glTexCoord2f(1, 1);
        glVertex2f(closeLeft, iconTop);

        glTexCoord2f(0, 1);
        glVertex2f(MIRROR_ICON_SIZE + closeLeft, iconTop);

        glTexCoord2f(0, 0);
        glVertex2f(MIRROR_ICON_SIZE + closeLeft, twp);

        glTexCoord2f(1, 0);
        glVertex2f(closeLeft, twp);
    }
    glEnd();

    glBindTexture(GL_TEXTURE_2D, _restoreTextureId);
    glBegin(GL_QUADS);
    {
        glTexCoord2f(1, 1);
        glVertex2f(resoreLeft, iconTop);
        
        glTexCoord2f(0, 1);
        glVertex2f(MIRROR_ICON_SIZE + resoreLeft, iconTop);
        
        glTexCoord2f(0, 0);
        glVertex2f(MIRROR_ICON_SIZE + resoreLeft, twp);
        
        glTexCoord2f(1, 0);
        glVertex2f(resoreLeft, twp);
    }
    glEnd();
    
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
}
