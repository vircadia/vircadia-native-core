//
//  ToolsPalette.h
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__ToolsPalette__
#define __interface__ToolsPalette__

#include "Tool.h"
#include "Swatch.h"

#include <vector>

class ToolsPalette {
public:
    void init(int screenWidth, int screenHeight);
    void addAction(QAction* action, int x, int y);
    void addTool(Tool* tool);
    void render(int screenWidth, int screenHeight);

private:
    QImage _textureImage;
    GLuint _textureID;
    std::vector<Tool*> _tools;

    int _top;
    int _left;

    int _width;
    int _height;
};

#endif /* defined(__interface__ToolsPalette__) */
