
#include <algorithm>

#include "InterfaceConfig.h"
#include "Util.h"

#include "MenuRow.h"
#include "MenuColumn.h"
#include "Menu.h"



MenuColumn::MenuColumn() {
}

MenuColumn::MenuColumn(char * columnName) {
    int length = std::min(MAX_COLUMN_NAME - 1,(int) strlen(columnName));
    strncpy(this->columnName, columnName, length);
    this->columnName[length] = '\0';
    columnWidth = 0;
    leftPosition = 0;
    leftMouseOver = 0;
    rightMouseOver = 0;
    topMouseOver = 0;
    bottomMouseOver = 0;
}

MenuColumn::~MenuColumn() {
    rows.clear();
}

void MenuColumn::mouseClickRow(int numberOfRowsIndex) {
     rows[numberOfRowsIndex].call();
}

bool MenuColumn::mouseClick(int x, int y, int leftPosition, int menuHeight, int lineHeight) {
    int rightPosition = leftPosition + 200;
    int topPosition = menuHeight;
    int bottomPosition = menuHeight;
    int columnWidth = 0;
    bool bRet = false;
    for (unsigned int i = 0; i < rows.size(); ++i) {
        columnWidth = rows[i].getWidth();
        topPosition = bottomPosition + lineHeight;
        if (x > leftPosition && x < rightPosition && y > bottomPosition && y < topPosition) {
            mouseClickRow(i);
            bRet = true;
            break;
        }
        bottomPosition = topPosition;
    }
    return bRet;
}

void MenuColumn::setMouseOver(int leftPosition, int rightPosition, int topPosition, int bottomPosition) {
   leftMouseOver = leftPosition;
   rightMouseOver = rightPosition;
   topMouseOver = topPosition;
   bottomMouseOver = bottomPosition;
}

bool MenuColumn::mouseOver(int x, int y, int leftPosition, int menuHeight, int lineHeight) {
    int rightPosition = leftPosition + 100;
    int topPosition = menuHeight;
    int bottomPosition = menuHeight;
    int columnWidth = 0;
    bool bRet = false;
    for (unsigned int i = 0; i < rows.size(); ++i) {
        columnWidth = rows[i].getWidth();
        topPosition = bottomPosition + lineHeight ;
        if (x > leftPosition && x < rightPosition && y > bottomPosition && y < topPosition) {
            setMouseOver(leftPosition, rightPosition, bottomPosition, topPosition);
            bRet = true;
            break;
        }
        bottomPosition = topPosition;
    }
    if (!bRet) {
        setMouseOver(0, 0, 0, 0);
    }
    return bRet;
}

char* MenuColumn::getName() {
    return this->columnName;
}

int MenuColumn::getWidth(float scale, int mono, int leftPosition) {
    if (columnWidth == 0) {
        columnWidth = widthText(scale, mono, this->columnName);
        this->leftPosition = leftPosition;
    }  
    return columnWidth;
}

int MenuColumn::getWidth() {
    return columnWidth;
}

int MenuColumn::getLeftPosition() {
    return leftPosition;
}

int MenuColumn::addRow(char * rowName, PFNRowCallback callback) {
    MenuRow* pRow;
    pRow = new MenuRow(rowName, callback);
    rows.push_back(*pRow);
    delete pRow;
    return 0;
}

void MenuColumn::render(int yOffset, int menuHeight, int lineHeight) {
    int numberOfRows = rows.size();
    if (numberOfRows > 0) {
        glColor3f(0.9,0.9,0.9);
        glBegin(GL_QUADS); {
            glVertex2f(leftPosition,  yOffset + menuHeight);
            glVertex2f(leftPosition+100, yOffset + menuHeight);
            glVertex2f(leftPosition+100,  yOffset + menuHeight + numberOfRows*lineHeight);
            glVertex2f(leftPosition ,  yOffset + menuHeight + numberOfRows* lineHeight);
        }
        glEnd();
    }
    float scale = 0.10;
    int mono = 0;
    int y = menuHeight + lineHeight / 2 ;
    char* rowName;
    int columnWidth = 0;
    for (unsigned int i = 0; i < rows.size(); ++i) {
        rowName = rows[i].getName();
        columnWidth = rows[i].getWidth(scale, mono, 0);
        drawtext(leftPosition + SPACE_BEFORE_ROW_NAME, y+5 + yOffset, scale, 0, 1.0, mono, rowName, 0, 0, 0);
        y += lineHeight;
    }
    renderMouseOver(yOffset);
}

void MenuColumn::renderMouseOver(int yOffset) {
    if (leftMouseOver != 0 || topMouseOver != 0 || rightMouseOver != 0 ||& bottomMouseOver != 0) {
        glColor4f(0,0,0,0.1);
        glBegin(GL_QUADS); {
            glVertex2f(leftMouseOver,  yOffset + topMouseOver);
            glVertex2f(rightMouseOver, yOffset + topMouseOver);
            glVertex2f(rightMouseOver,  yOffset + bottomMouseOver);
            glVertex2f(leftMouseOver ,  yOffset + bottomMouseOver);
        }
        glEnd();
    }
}
