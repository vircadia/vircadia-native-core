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

class Tool {
public:
    Tool(QAction *action, GLuint texture, int x, int y);

    bool isActive();
    virtual void render(int width, int height);

protected:
    QAction* _action;
    GLuint _texture;

    // position in the SVG grid
    double _x;
    double _y;
};

#endif /* defined(__interface__Tool__) */
