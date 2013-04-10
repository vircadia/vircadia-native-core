
#include <algorithm>
#include "InterfaceConfig.h"
#include "MenuRow.h"
#include "MenuColumn.h"
#include "Menu.h"
#include "Util.h"



MenuColumn::MenuColumn()
{
}
MenuColumn::MenuColumn(char * columnName)
{
    int length = std::min(MAX_COLUMN_NAME - 1,(int) strlen(columnName));
    strncpy(this->columnName, columnName, length);
    this->columnName[length] = '\0';
    columnWidth = 0;
    leftPosition = 0;
    xLeftMouseOver = 0;
    xRightMouseOver = 0;
    yTopMouseOver = 0;
    yBottomMouseOver = 0;
}

MenuColumn::~MenuColumn()
{
}

void MenuColumn::mouseClickRow(int iRowIndex) {
     rows[iRowIndex].call();
}

bool MenuColumn::mouseClick(int x, int y, int xLeft, int menuHeight, int lineHeight) {
    int xRight = xLeft + 200;
    int yTop = menuHeight;
    int yBottom = menuHeight;
    int columnWidth;
    bool bRet = false;
    for (unsigned int i = 0; i < rows.size(); ++i) {
        columnWidth = rows[i].getWidth();
        yTop = yBottom + lineHeight;
        if (x > xLeft && x < xRight && y > yBottom && y < yTop) {
            mouseClickRow(i);
            bRet = true;
            break;
        }
        yBottom = yTop;
    }
    return bRet;
}

void MenuColumn::setMouseOver(int xLeft, int xRight, int yTop, int yBottom) {
   xLeftMouseOver = xLeft;
   xRightMouseOver = xRight;
   yTopMouseOver = yTop;
   yBottomMouseOver = yBottom;
}

bool MenuColumn::mouseOver(int x, int y, int xLeft, int menuHeight, int lineHeight) {
    int xRight = xLeft + 100;
    int yTop = menuHeight;
    int yBottom = menuHeight;
    int columnWidth;
    bool bRet = false;
    for (unsigned int i = 0; i < rows.size(); ++i) {
        columnWidth = rows[i].getWidth();
        yTop = yBottom + lineHeight ;
        if (x > xLeft && x < xRight && y > yBottom && y < yTop) {
            setMouseOver(xLeft, xRight, yBottom, yTop);
            bRet = true;
            break;
        }
        yBottom = yTop;
    }
    if (!bRet) {
        setMouseOver(0, 0, 0, 0);
    }
    return bRet;
}

char * MenuColumn::getName() {
    return this->columnName;
}

int MenuColumn::getWidth(float scale, int mono, int leftPosition){
    if (columnWidth == 0) {
        columnWidth = widthText( scale, mono, this->columnName);
        this->leftPosition = leftPosition;
    }  
    return columnWidth;
}

int MenuColumn::getWidth(){
    return columnWidth;
}

int MenuColumn::getLeftPosition(){
    return leftPosition;
}

int  MenuColumn::addRow(char * rowName, PFNRowCallback callback){
    MenuRow  * pRow;
    pRow = new MenuRow(rowName, callback);
    rows.push_back(*pRow);
    delete pRow;
    return 0;

}

void MenuColumn::render(int yOffset, int menuHeight, int lineHeight) {
    int iRow = rows.size();
    if (iRow > 0) {
        glColor3f(0.9,0.9,0.9);
        glBegin(GL_QUADS); {
            glVertex2f(leftPosition,  yOffset + menuHeight);
            glVertex2f(leftPosition+100, yOffset + menuHeight);
            glVertex2f(leftPosition+100,  yOffset + menuHeight + iRow*lineHeight);
            glVertex2f(leftPosition ,  yOffset + menuHeight + iRow* lineHeight);
        }
        glEnd();
    }
    float scale = 0.10;
    int mono = 0;
    int y = menuHeight + lineHeight / 2 ;
    char * rowName;
    int columnWidth;
    for (unsigned int i = 0; i < rows.size(); ++i)
    {
        rowName = rows[i].getName();
        columnWidth = rows[i].getWidth(scale, mono, 0);
        drawtext(leftPosition + SPACE_BEFORE_ROW_NAME, y+5 + yOffset, scale, 0, 1.0, mono, rowName, 0, 0, 0);
        y += lineHeight;
    }
    renderMouseOver(yOffset);
}

void MenuColumn::renderMouseOver(int yOffset) {
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
