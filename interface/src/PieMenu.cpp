//
//  PieMenu.cpp
//  hifi
//
//  Created by Clement Brisset on 7/18/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "PieMenu.h"

#include <cmath>

#include <QAction>
#include <QSvgRenderer>
#include <QPainter>
#include <QGLWidget>
#include <SharedUtil.h>

PieMenu::PieMenu() :
    _radiusIntern(30),
    _radiusExtern(70),
    _magnification(1.2f),
    _isDisplayed(false) {
}

void PieMenu::init(const char *fileName, int screenWidth, int screenHeight) {
    // Load SVG
    switchToResourcesParentIfRequired();
    QSvgRenderer renderer((QString) QString(fileName));

    // Prepare a QImage with desired characteritisc
    QImage image(2 * _radiusExtern, 2 * _radiusExtern, QImage::Format_ARGB32);
    image.fill(0x0);

    // Get QPainter that paints to the image
    QPainter painter(&image);
    renderer.render(&painter);

    //get the OpenGL-friendly image
    _textureImage = QGLWidget::convertToGLFormat(image);

    glGenTextures(1, &_textureID);
    glBindTexture(GL_TEXTURE_2D, _textureID);

    //generate the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                  _textureImage.width(),
                  _textureImage.height(),
                  0, GL_RGBA, GL_UNSIGNED_BYTE,
                  _textureImage.bits());

    //texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void PieMenu::addAction(QAction* action){
    _actions.push_back(action);
}

void PieMenu::render() {
    if (_actions.size() == 0) {
        return;
    }

    float start = M_PI / 2.0f;
    float end   = start + 2.0f * M_PI;
    float step  = 2.0f * M_PI / 100.0f;
    float distance  = sqrt((_mouseX - _x) * (_mouseX - _x) +
                           (_mouseY - _y) * (_mouseY - _y));

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _textureID);

    glColor3f(1.0f, 1.0f, 1.0f);

    if (_radiusIntern < distance) {
        float angle = atan2((_mouseY - _y), (_mouseX - _x)) - start;
        angle = (0.0f < angle) ? angle : angle + 2.0f * M_PI;

        _selectedAction = floor(angle / (2.0f * M_PI / _actions.size()));

        start = start + _selectedAction * 2.0f * M_PI / _actions.size();
        end   = start + 2.0f * M_PI / _actions.size();
        glBegin(GL_TRIANGLE_FAN);
        glTexCoord2f(0.5f, 0.5f);
        glVertex2f(_x, _y);
        for (float i = start; i < end; i += step) {
            glTexCoord2f(0.5f + 0.5f * cos(i), 0.5f - 0.5f * sin(i));
            glVertex2f(_x + _magnification * _radiusExtern * cos(i),
                       _y + _magnification * _radiusExtern * sin(i));
        }
        glTexCoord2f(0.5f + 0.5f * cos(end), 0.5f +  - 0.5f * sin(end));
        glVertex2f(_x + _magnification * _radiusExtern * cos(end),
                   _y + _magnification * _radiusExtern * sin(end));
        glEnd();
    } else {
        _selectedAction = -1;

        glBegin(GL_QUADS);
        glTexCoord2f(1, 1);
        glVertex2f(_x + _radiusExtern, _y - _radiusExtern);

        glTexCoord2f(1, 0);
        glVertex2f(_x + _radiusExtern, _y + _radiusExtern);

        glTexCoord2f(0, 0);
        glVertex2f(_x - _radiusExtern, _y + _radiusExtern);

        glTexCoord2f(0, 1);
        glVertex2f(_x - _radiusExtern, _y - _radiusExtern);
        glEnd();
    }
    glDisable(GL_TEXTURE_2D);
}

void PieMenu::resize(int screenWidth, int screenHeight) {
}

void PieMenu::mouseMoveEvent(int x, int y) {
    _mouseX = x;
    _mouseY = y;
}

void PieMenu::mousePressEvent(int x, int y) {
    _x = _mouseX = x;
    _y = _mouseY = y;
    _selectedAction = -1;
    _isDisplayed = true;
}

void PieMenu::mouseReleaseEvent(int x, int y) {
    if (0 <= _selectedAction && _selectedAction < _actions.size() && _actions[_selectedAction]) {
        _actions[_selectedAction]->trigger();
    }

    _isDisplayed = false;
}
