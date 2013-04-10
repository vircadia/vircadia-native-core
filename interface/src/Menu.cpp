
#include <algorithm>

#include "InterfaceConfig.h"
#include "Util.h"

#include "MenuRow.h"
#include "MenuColumn.h"
#include "Menu.h"


const int lineHeight = 30;
const int menuHeight = 30;
const int yOffset = 8;      // under windows we have 8 vertical pixels offset. In 2D an object with y=8, the object is displayed at y=0
                            // change the value in the other platforms (if required).


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

void Menu::mouseClickColumn(int iColumnIndex) {
    if (currentColumn == iColumnIndex) {
        currentColumn = -1;
    } else {
        currentColumn = iColumnIndex;
    }
}

void Menu::setMouseOver(int leftPosition, int rightPosition, int yTop, int yBottom) {
   leftMouseOver = leftPosition;
   rightMouseOver = rightPosition;
   topMouseOver = yTop;
   bottomMouseOver = yBottom;
}

void Menu::renderMouseOver() {
    if (leftMouseOver != 0 || topMouseOver != 0 || rightMouseOver != 0 ||& bottomMouseOver != 0) {
        glColor4f(0, 0, 0, 0.1);
        glBegin(GL_QUADS); {
            glVertex2f(leftMouseOver,  yOffset + topMouseOver);
            glVertex2f(rightMouseOver, yOffset + topMouseOver);
            glVertex2f(rightMouseOver,  yOffset + bottomMouseOver);
            glVertex2f(leftMouseOver,  yOffset + bottomMouseOver);
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
        if (x > leftPosition && x < rightPosition && y > 0 && y < menuHeight) {
            mouseClickColumn(i);
            menuFound = true;
            break;
        } else if (currentColumn == i) {
             menuFound = columns[i].mouseClick(x, y, leftPosition, menuHeight, lineHeight);
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
        if (x > leftPosition && x < rightPosition && y > 0 && y < menuHeight) {
            setMouseOver(leftPosition, rightPosition, 0, menuHeight);
            overMenu = true;
            if (currentColumn >= 0) {
                columns[currentColumn].setMouseOver(0, 0, 0, 0);
                currentColumn = i;
            }
            break;
        } else if (currentColumn == i) {
             columns[i].mouseOver(x, y, leftPosition, menuHeight, lineHeight);
        }
        leftPosition = rightPosition;
    }
    if (!overMenu) {
        setMouseOver(0, 0, 0, 0);
    }
    return overMenu;
}

void Menu::render(int screenWidth, int screenHeight) {
    float scale = 0.10;
    int mono = 0;
    glColor3f(0.9, 0.9, 0.9);
    int width = screenWidth;
    int height = screenHeight;
    glBegin(GL_QUADS); {
        glVertex2f(0, yOffset);
        glVertex2f(width, yOffset);
        glVertex2f(width, menuHeight + yOffset);
        glVertex2f(0 , menuHeight + yOffset);
    }
    glEnd();
    int xPosition = SPACE_BETWEEN_COLUMNS;
    char* columnName;
    int columnWidth;
    for (unsigned int i = 0; i < columns.size(); ++i) {
        columnName = columns[i].getName();
        columnWidth = columns[i].getWidth(scale, mono, xPosition - 0.5 * SPACE_BETWEEN_COLUMNS);
        drawtext(xPosition, 18 + yOffset, scale, 0, 1.0, mono, columnName, 0, 0, 0);
        xPosition += columnWidth + SPACE_BETWEEN_COLUMNS;
        if (currentColumn == i) {
            columns[i].render(yOffset, menuHeight, lineHeight);
        }
    }
    renderMouseOver();
 }


MenuColumn* Menu::addColumn(char *columnName) {
    MenuColumn* pColumn;
    pColumn = new MenuColumn(columnName);
    columns.push_back(*pColumn);
    delete pColumn;
    return &columns[columns.size() - 1];
}