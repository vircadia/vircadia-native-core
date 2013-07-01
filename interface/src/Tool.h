//
//  Tool.h
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Tool__
#define __interface__Tool__

#include <QAction>

#include "InterfaceConfig.h"
#include "Util.h"

class QAction;

// tool size
static double _width;
static double _height;

class Tool {
public:
    Tool(QAction *action, GLuint texture, int x, int y);

    bool isActive();
    virtual void render(int screenWidth, int screenHeight);

protected:
    QAction* _action;
    GLuint _texture;

    // position in the SVG grid
    double _x;
    double _y;

    // tool size
    double _width;
    double _height;
};

#endif /* defined(__interface__Tool__) */
