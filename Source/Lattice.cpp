//
//  Lattice.cpp
//  interface
//
//  Created by Philip on 1/19/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "Lattice.h"

Lattice::Lattice(int w, int h) {
    width = w;
    height = h;
    tilegap = 2;
    lastindex = -1;
    tiles = new Tile[width*height];
    for (int i = 0; i < (width*height); i++) {
        tiles[i].color[0] = tiles[i].color[1] = tiles[i].color[2] = 0.2 + randFloat()*0.3;
        tiles[i].x = (i % width);
        tiles[i].y = int(i/width);
        tiles[i].brightness = 1.0;
        tiles[i].type = 0;
        tiles[i].excited = tiles[i].inhibited = 0.0;
    }
}

void Lattice::render(int screenWidth, int screenHeight) {
    float tilewidth = screenWidth/width;
    float tileheight = screenHeight/height;
    float tilecolor[3];
    glBegin(GL_QUADS);
    for (int i = 0; i < (width*height); i++) {
        if (tiles[i].type == 0) {
            tilecolor[0] = 0.25; tilecolor[1] = 0.25; tilecolor[2] = 0.25;
        } else if (tiles[i].type == 1) {
            if (tiles[i].inhibited >= 0.1) {
                tilecolor[0] = 0.5; tilecolor[1] = 0.0; tilecolor[2] = 0.0;
            } else {
                tilecolor[0] = 0.2; tilecolor[1] = 0.0; tilecolor[2] = 0.5;
            }
        }
        glColor3f(tilecolor[0]*(1.f+tiles[i].excited), tilecolor[1]*(1.f+tiles[i].excited), tilecolor[2]*(1.f+tiles[i].excited));
        glVertex2f(tiles[i].x*tilewidth, tiles[i].y*tileheight);
        glVertex2f((tiles[i].x + 1)*tilewidth - tilegap, tiles[i].y*tileheight);
        glVertex2f((tiles[i].x + 1)*tilewidth - tilegap, (tiles[i].y + 1)*tileheight - tilegap);
        glVertex2f(tiles[i].x*tilewidth, (tiles[i].y + 1)*tileheight - tilegap);        
    }
    glEnd();
}

void Lattice::mouseClick(float x, float y) {
    //  Update lattice based on mouse location, where x and y are floats between 0 and 1 corresponding to screen location clicked
    //  Change the clicked cell to a different type 
    //printf("X = %3.1f Y = %3.1f\n", x, y);
    int index = int(x*(float)width) + int(y*(float)height)*width;
    if (index != lastindex) {
        tiles[index].type++;
        if (tiles[index].type == 2) tiles[index].type = 0;
        lastindex = index; 
    }
}

void Lattice::mouseOver(float x, float y) {
    //  Update lattice based on mouse location, where x and y are floats between 0 and 1 corresponding to screen location clicked
    //  Excite the hovered cell a bit toward firing 
    //printf("X = %3.1f Y = %3.1f\n", x, y);
    int index = int(x*(float)width) + int(y*(float)height)*width;
    if (tiles[index].type > 0) tiles[index].excited += 0.05;
    //printf("excited = %3.1f, inhibited = %3.1f\n", tiles[index].excited, tiles[index].inhibited);
}

void Lattice::simulate(float deltaTime) {
    int index;
    for (int i = 0; i < (width*height); i++) {
        if (tiles[i].type > 0) {
            if ((tiles[i].excited > 0.5) && (tiles[i].inhibited < 0.1)) {
                tiles[i].excited = 1.0;
                tiles[i].inhibited = 1.0;
                //  Add Energy to neighbors
                for (int j = 0; j < 8; j++) {
                    if (j == 0) index = i - width - 1;
                    else if (j == 1) index = i - width;
                    else if (j == 2) index = i - width + 1;
                    else if (j == 3) index = i - 1;
                    else if (j == 4) index = i + 1;
                    else if (j == 5) index = i + width - 1;
                    else if (j == 6) index = i + width;
                    else index = i + width + 1;
                    if ((index > 0) && (index < width*height)) {
                        if (tiles[index].inhibited < 0.1) tiles[index].excited += 0.5;
                    }
                    
                }
            }
            tiles[i].excited *= 0.98;
            tiles[i].inhibited *= 0.98;
        }
    }
}





