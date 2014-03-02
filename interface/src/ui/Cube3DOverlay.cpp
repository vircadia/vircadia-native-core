//
//  Cube3DOverlay.cpp
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <SharedUtil.h>

#include "Cube3DOverlay.h"

Cube3DOverlay::Cube3DOverlay() {
}

Cube3DOverlay::~Cube3DOverlay() {
}

void Cube3DOverlay::render() {
    if (!_visible) {
        return; // do nothing if we're not visible
    }

    const float MAX_COLOR = 255;
    glColor4f(_color.red / MAX_COLOR, _color.green / MAX_COLOR, _color.blue / MAX_COLOR, _alpha);


    glDisable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(_position.x + _size * 0.5f,
                 _position.y + _size * 0.5f,
                 _position.z + _size * 0.5f);
    glLineWidth(_lineWidth);
    if (_isSolid) {
        glutSolidCube(_size);
    } else {
        glutWireCube(_size);
    }
    glPopMatrix();

}
