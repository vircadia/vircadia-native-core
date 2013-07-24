//
//  PieMenu.h
//  hifi
//
//  Created by Clement Brisset on 7/18/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__PieMenu__
#define __hifi__PieMenu__

#include <vector>

#include "InterfaceConfig.h"
#include "Util.h"

#include <QImage>

class QAction;

class PieMenu {
public:
    PieMenu();

    void init(const char* fileName, int screenWidth, int screenHeight);
    void addAction(QAction* action);
    void render();
    void resize(int screenWidth, int screenHeight);

    bool isDisplayed() const {return _isDisplayed;}

    void mouseMoveEvent   (int x, int y);
    void mousePressEvent  (int x, int y);
    void mouseReleaseEvent(int x, int y);

private:
    QImage _textureImage;
    GLuint _textureID;

    // position of the menu
    int   _x;
    int   _y;
    int   _radiusIntern;
    int   _radiusExtern;
    float _magnification;

    int _mouseX;
    int _mouseY;

    int _selectedAction;

    bool _isDisplayed;

    std::vector<QAction*> _actions;
};

#endif /* defined(__hifi__PieMenu__) */
