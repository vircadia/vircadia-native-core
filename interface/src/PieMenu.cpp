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
#include <QGLWidget>

PieMenu::PieMenu() :
    _radiusIntern(30),
    _radiusExtern(70),
    _isDisplayed(false) {

    addAction(NULL);
    addAction(NULL);
    addAction(NULL);
    addAction(NULL);
    addAction(NULL);
}

void PieMenu::init(const char *fileName, int screenWidth, int screenHeight) {

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

    glBegin(GL_QUAD_STRIP);
    glColor3f(1.0f, 1.0f, 1.0f);
    for (float i = start; i < end; i += step) {
        glVertex2f(_x + _radiusIntern * cos(i), _y + _radiusIntern * sin(i));
        glVertex2f(_x + _radiusExtern * cos(i), _y + _radiusExtern * sin(i));
    }
    glVertex2f(_x + _radiusIntern * cos(end), _y + _radiusIntern * sin(end));
    glVertex2f(_x + _radiusExtern * cos(end), _y + _radiusExtern * sin(end));
    glEnd();

    if (_radiusIntern < distance) {
        float angle = atan2((_mouseY - _y), (_mouseX - _x)) - start;
        angle = (0.0f < angle) ? angle : angle + 2.0f * M_PI;

        _selectedAction = floor(angle / (2.0f * M_PI / _actions.size()));

        start = start + _selectedAction * 2.0f * M_PI / _actions.size();
        end   = start + 2.0f * M_PI / _actions.size();
        glBegin(GL_QUAD_STRIP);
        glColor3f(0.0f, 0.0f, 0.0f);
        for (float i = start; i < end; i += step) {
            glVertex2f(_x + _radiusIntern * cos(i), _y + _radiusIntern * sin(i));
            glVertex2f(_x + _radiusExtern * cos(i), _y + _radiusExtern * sin(i));
        }
        glVertex2f(_x + _radiusIntern * cos(end), _y + _radiusIntern * sin(end));
        glVertex2f(_x + _radiusExtern * cos(end), _y + _radiusExtern * sin(end));
        glEnd();
    } else {
        _selectedAction = -1;
    }
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
    _isDisplayed = true;
}

void PieMenu::mouseReleaseEvent(int x, int y) {
    // TODO

    _isDisplayed = false;
}
