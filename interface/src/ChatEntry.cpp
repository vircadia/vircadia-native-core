//
//  ChatEntry.cpp
//  interface
//
//  Created by Andrzej Kapolka on 4/24/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include "InterfaceConfig.h"

#include "ChatEntry.h"
#include "Util.h"

using namespace std;

const int MAX_CONTENT_LENGTH = 140;

void ChatEntry::clear () {
    contents.clear();
    cursorPos = 0;
}

bool ChatEntry::key(unsigned char k) {
    switch (k) {
        case '\r':
            return false;
        
        case '\b':
            if (cursorPos != 0) {
                contents.erase(cursorPos - 1, 1);
                cursorPos--;
            }
            return true;
        
        case 127: // delete
            if (cursorPos < contents.size()) {
                contents.erase(cursorPos, 1);
            }
            return true;
            
        default:
            if (contents.size() != MAX_CONTENT_LENGTH) {
                contents.insert(cursorPos, 1, k);
                cursorPos++;
            }
            return true;
    }
}

void ChatEntry::specialKey(unsigned char k) {
    switch (k) {
        case GLUT_KEY_LEFT:
            if (cursorPos != 0) {
                cursorPos--;
            }
            break;
            
        case GLUT_KEY_RIGHT:
            if (cursorPos != contents.size()) {
                cursorPos++;
            }
            break;
    }
}

void ChatEntry::render(int screenWidth, int screenHeight) {
    drawtext(20, screenHeight - 150, 0.10, 0, 1.0, 0, contents.c_str(), 1, 1, 1);
    
    float width = 0;
    for (string::iterator it = contents.begin(), end = it + cursorPos; it != end; it++) {
        width += glutStrokeWidth(GLUT_STROKE_ROMAN, *it)*0.10;
    }
    glDisable(GL_LINE_SMOOTH);
    glBegin(GL_LINE_STRIP);
    glVertex2f(20 + width, screenHeight - 165);
    glVertex2f(20 + width, screenHeight - 150);
    glEnd();
    glEnable(GL_LINE_SMOOTH);
}
