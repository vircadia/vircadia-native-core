
#include <algorithm>
#include "InterfaceConfig.h"
#include "MenuRow.h"
#include "MenuColumn.h"
#include "Menu.h"
#include "Util.h"


int lineHeight = 30;
int menuHeight = 30;
int yOffset = 8;    // under windows we have 8 vertical pixels offset. In 2D an object with y=8, the object is displayed at y=0
                    // change the value in the other platforms (if required).


Menu::Menu(){
    iCurrentColumn = -1;
    xLeftMouseOver = 0;
    xRightMouseOver = 0;
    yTopMouseOver = 0;
    yBottomMouseOver = 0;
}


Menu::~Menu(){
    columns.clear();
}

void Menu::mouseClickColumn(int iColumnIndex) {
    if (iCurrentColumn == iColumnIndex) {
        iCurrentColumn = -1;
    } else {
        iCurrentColumn = iColumnIndex;
    }
}

void Menu::setMouseOver(int xLeft, int xRight, int yTop, int yBottom) {
   xLeftMouseOver = xLeft;
   xRightMouseOver = xRight;
   yTopMouseOver = yTop;
   yBottomMouseOver = yBottom;
}

void Menu::renderMouseOver() {
    if (xLeftMouseOver != 0 || yTopMouseOver != 0 || xRightMouseOver != 0 ||& yBottomMouseOver != 0){
        glColor4f(0,0,0,0.1);
        glBegin(GL_QUADS); {
            glVertex2f(xLeftMouseOver,  yOffset + yTopMouseOver);
            glVertex2f(xRightMouseOver, yOffset + yTopMouseOver);
            glVertex2f(xRightMouseOver,  yOffset + yBottomMouseOver);
            glVertex2f(xLeftMouseOver ,  yOffset + yBottomMouseOver);
        }
        glEnd();
    }
}

bool Menu::mouseClick(int x, int y) {
    int xLeft = 0.5 * SPACE_BETWEEN_COLUMNS;
    int xRight;
    int columnWidth;
    bool bRet = false;
    for (unsigned int i = 0; i < columns.size(); ++i)
    {
        columnWidth = columns[i].getWidth();
        xRight = xLeft + columnWidth + 1.5 * SPACE_BETWEEN_COLUMNS;
        if (x > xLeft && x < xRight && y > 0 && y < menuHeight){
            mouseClickColumn(i);
            bRet = true;
            break;
        }
        if (iCurrentColumn == i) {
             bRet = columns[i].mouseClick(x, y, xLeft, menuHeight, lineHeight);
             if (bRet) {
                 iCurrentColumn = -1;
            }
        }
        xLeft = xRight;
    }
    return bRet;
}

bool Menu::mouseOver(int x, int y) {
    int xLeft = 0.5 * SPACE_BETWEEN_COLUMNS;
    int xRight;
    int columnWidth;
    bool bRet = false;
    for (unsigned int i = 0; i < columns.size(); ++i)
    {
        columnWidth = columns[i].getWidth();
        xRight = xLeft + columnWidth +  SPACE_BETWEEN_COLUMNS;
        if (x > xLeft && x < xRight && y > 0 && y < menuHeight){
               printf("Mouse Over: %d %d", x, y);

            setMouseOver(xLeft, xRight, 0, menuHeight);
            bRet = true;
            if (iCurrentColumn >= 0) {
                columns[iCurrentColumn].setMouseOver(0, 0, 0, 0);
                iCurrentColumn = i;
            }
            break;
        } else if (iCurrentColumn == i) {
             columns[i].mouseOver(x, y, xLeft, menuHeight, lineHeight);
        }
        xLeft = xRight;
    }
    if (!bRet) {
        setMouseOver(0, 0, 0, 0);
    }
    return bRet;
}

void Menu::render(int screenwidth, int screenheight) {
    float scale = 0.10;
    int mono = 0;
    glColor3f(0.9,0.9,0.9);
    int width = screenwidth;
    int height = screenheight;
    glBegin(GL_QUADS); {
        glVertex2f(0, yOffset);
        glVertex2f(width, yOffset);
        glVertex2f( width, menuHeight + yOffset);
        glVertex2f(0 , menuHeight + yOffset);
    }
    glEnd();
    int x = SPACE_BETWEEN_COLUMNS;
    char * columnName;
    int columnWidth;
    for (unsigned int i = 0; i < columns.size(); ++i)
    {
        columnName = columns[i].getName();
        columnWidth = columns[i].getWidth(scale, mono, x- 0.5 * SPACE_BETWEEN_COLUMNS);
        drawtext(x,18 + yOffset, scale, 0, 1.0, mono, columnName, 0, 0, 0);
        x += columnWidth + SPACE_BETWEEN_COLUMNS;
        if (iCurrentColumn == i) {
            columns[i].render(yOffset, menuHeight, lineHeight);
        }
    }
    renderMouseOver();
 }


MenuColumn* Menu::addColumn(char *columnName) {
    MenuColumn  * pColumn;
    pColumn = new MenuColumn(columnName);
    columns.push_back(*pColumn);
    delete pColumn;
    return &columns[columns.size()-1];
}