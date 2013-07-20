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
    // position of the menu
    int _x;
    int _y;
    int _radiusIntern;
    int _radiusExtern;

    int _mouseX;
    int _mouseY;

    int _selectedAction;

    bool _isDisplayed;

    std::vector<QAction*> _actions;
};

#endif /* defined(__hifi__PieMenu__) */
