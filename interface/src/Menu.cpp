//
//  Menu.cpp
//  hifi
//
//  Created by Dominque Vincent on 4/10/13.
//
//

#include <algorithm>

#include "InterfaceConfig.h"
#include "Util.h"

#include "MenuRow.h"
#include "MenuColumn.h"
#include "Menu.h"


const int LINE_HEIGHT = 30;
const int MENU_HEIGHT = 30;

#ifdef _WIN32
const int MENU_Y_OFFSET = 8;      // under windows we have 8 vertical pixels offset.
                                  // In 2D an object with y=8, the object is displayed at y=0
                                  // change the value in the other platforms (if required).
#else
const int MENU_Y_OFFSET = 0;
#endif

Menu::Menu() {
    currentColumn = -1;
    leftMouseOver = 0;
    rightMouseOver = 0;
    topMouseOver = 0;
    bottomMouseOver = 0;
}


Menu::~Menu() {
    columns.clear();
}

void Menu::mouseClickColumn(int columnIndex) {
    if (currentColumn == columnIndex) {
        currentColumn = -1;
    } else {
        currentColumn = columnIndex;
    }
}

void Menu::setMouseOver(int leftPosition, int rightPosition, int top, int bottom) {
   leftMouseOver = leftPosition;
   rightMouseOver = rightPosition;
   topMouseOver = top;
   bottomMouseOver = bottom;
}

void Menu::renderMouseOver() {
    if (leftMouseOver != 0 || topMouseOver != 0 || rightMouseOver != 0 ||& bottomMouseOver != 0) {
        glColor4f(0, 0, 0, 0.1);
        glBegin(GL_QUADS); {
            glVertex2f(leftMouseOver,  MENU_Y_OFFSET + topMouseOver);
            glVertex2f(rightMouseOver, MENU_Y_OFFSET + topMouseOver);
            glVertex2f(rightMouseOver,  MENU_Y_OFFSET + bottomMouseOver);
            glVertex2f(leftMouseOver,  MENU_Y_OFFSET + bottomMouseOver);
        }
        glEnd();
    }
}

bool Menu::mouseClick(int x, int y) {
    int leftPosition = 0.5 * SPACE_BETWEEN_COLUMNS;
    int rightPosition = 0;
    int columnWidth = 0;
    bool menuFound = false;
    for (unsigned int i = 0; i < columns.size(); ++i) {
        columnWidth = columns[i].getWidth();
        rightPosition = leftPosition + columnWidth + 1.5 * SPACE_BETWEEN_COLUMNS;
        if (x > leftPosition && x < rightPosition && y > 0 && y < MENU_HEIGHT) {
            mouseClickColumn(i);
            menuFound = true;
            break;
        } else if (currentColumn == i) {
             menuFound = columns[i].mouseClick(x, y, leftPosition, MENU_HEIGHT, LINE_HEIGHT);
             if (menuFound) {
                 currentColumn = -1;
            }
        }
        leftPosition = rightPosition;
    }
    return menuFound;
}

bool Menu::mouseOver(int x, int y) {
    int leftPosition = 0.5 * SPACE_BETWEEN_COLUMNS;
    int rightPosition;
    int columnWidth;
    bool overMenu = false;
    for (unsigned int i = 0; i < columns.size(); ++i) {
        columnWidth = columns[i].getWidth();
        rightPosition = leftPosition + columnWidth +  SPACE_BETWEEN_COLUMNS;
        if (x > leftPosition && x < rightPosition && y > 0 && y < MENU_HEIGHT) {
            setMouseOver(leftPosition, rightPosition, 0, MENU_HEIGHT);
            overMenu = true;
            if (currentColumn >= 0) {
                columns[currentColumn].setMouseOver(0, 0, 0, 0);
                currentColumn = i;
            }
            break;
        } else if (currentColumn == i) {
             columns[i].mouseOver(x, y, leftPosition, MENU_HEIGHT, LINE_HEIGHT);
        }
        leftPosition = rightPosition;
    }
    if (!overMenu) {
        setMouseOver(0, 0, 0, 0);
    }
    return overMenu;
}

const float MENU_COLOR[3] = {0.75, 0.75, 0.75};

void Menu::render(int screenWidth, int screenHeight) {
    float scale = 0.10;
    int mono = 0;
    glColor3fv(MENU_COLOR);
    int width = screenWidth;
    glEnable(GL_LINE_SMOOTH);
    glBegin(GL_QUADS); {
        glVertex2f(0, MENU_Y_OFFSET);
        glVertex2f(width, MENU_Y_OFFSET);
        glVertex2f(width, MENU_HEIGHT + MENU_Y_OFFSET);
        glVertex2f(0 , MENU_HEIGHT + MENU_Y_OFFSET);
    }
    glEnd();
    int xPosition = SPACE_BETWEEN_COLUMNS;
    char* columnName;
    int columnWidth;
    for (unsigned int i = 0; i < columns.size(); ++i) {
        columnName = columns[i].getName();
        columnWidth = columns[i].getWidth(scale, mono, xPosition - 0.5 * SPACE_BETWEEN_COLUMNS);
        drawtext(xPosition, 18 + MENU_Y_OFFSET, scale, 0, 1.0, mono, columnName, 0, 0, 0);
        xPosition += columnWidth + SPACE_BETWEEN_COLUMNS;
        if (currentColumn == i) {
            columns[i].render(MENU_Y_OFFSET, MENU_HEIGHT, LINE_HEIGHT);
        }
    }
    renderMouseOver();
 }


MenuColumn* Menu::addColumn(const char *columnName) {
    MenuColumn* pColumn;
    pColumn = new MenuColumn(columnName);
    columns.push_back(*pColumn);
    delete pColumn;
    return &columns[columns.size() - 1];
}