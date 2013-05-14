//
//  ChatEntry.cpp
//  interface
//
//  Created by Andrzej Kapolka on 4/24/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <QKeyEvent>

#include "ChatEntry.h"
#include "InterfaceConfig.h"
#include "Util.h"

using namespace std;

const int MAX_CONTENT_LENGTH = 140;

ChatEntry::ChatEntry() : _cursorPos(0) {
}

void ChatEntry::clear() {
    _contents.clear();
    _cursorPos = 0;
}

bool ChatEntry::keyPressEvent(QKeyEvent* event) {
    event->accept();
    switch (event->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            return false;
        
        case Qt::Key_Escape:
            clear();
            return false;
        
        case Qt::Key_Backspace:
            if (_cursorPos != 0) {
                _contents.erase(_cursorPos - 1, 1);
                _cursorPos--;
            }
            return true;
        
        case Qt::Key_Delete:
            if (_cursorPos < _contents.size()) {
                _contents.erase(_cursorPos, 1);
            }
            return true;
        
        case Qt::Key_Left:
            if (_cursorPos != 0) {
                _cursorPos--;
            }
            return true;
            
        case Qt::Key_Right:
            if (_cursorPos != _contents.size()) {
                _cursorPos++;
            }
            return true;
            
        default:
            QString text = event->text();
            if (text.isEmpty()) {
                event->ignore();
                return true;
            }
            if (_contents.size() != MAX_CONTENT_LENGTH) {
                _contents.insert(_cursorPos, 1, text.at(0).toAscii());
                _cursorPos++;
            }
            return true;
    }
}

void ChatEntry::render(int screenWidth, int screenHeight) {
    drawtext(20, screenHeight - 150, 0.10, 0, 1.0, 0, _contents.c_str(), 1, 1, 1);
    
    float width = 0;
    for (string::iterator it = _contents.begin(), end = it + _cursorPos; it != end; it++) {
        width += widthChar(0.10, 0, *it);
    }
    glDisable(GL_LINE_SMOOTH);
    glBegin(GL_LINE_STRIP);
    glVertex2f(20 + width, screenHeight - 165);
    glVertex2f(20 + width, screenHeight - 150);
    glEnd();
    glEnable(GL_LINE_SMOOTH);
}
