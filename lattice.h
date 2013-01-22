//
//  lattice.h
//  interface
//
//  Created by Philip on 1/19/13.
//  Copyright (c) 2013 Rosedale Lab. All rights reserved.
//

#ifndef __interface__lattice__
#define __interface__lattice__

#include "glm/glm.hpp"
#include "util.h"
#include "world.h"
#include <GLUT/glut.h>
#include <iostream>

class Lattice {
public:
    Lattice(int width, int height);
    void simulate(float deltaTime);
    void render(int screenWidth, int screenHeight);
    void mouseClick(float x, float y);
    void mouseOver(float x, float y);
private:
    int lastindex;
    int width, height;
    int tilegap;
    struct Tile {
        float x,y;
        float color[3];
        float brightness;
        float excited;
        float inhibited;
        int type;
    } *tiles;
};

#endif /* defined(__interface__lattice__) */
