//
//  MenuColumn.cpp
//  hifi
//
//  Created by Dominque Vincent on 4/10/13.
//
//

#include <algorithm>
#include <cstring>

#include "InterfaceConfig.h"
#include "Util.h"

#include "MenuRow.h"
#include "MenuColumn.h"
#include "Menu.h"


MenuColumn::MenuColumn() {
}

MenuColumn::MenuColumn(const char* columnName) {
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
    int rightPosition = leftPosition + 200; // XXXBHG - this looks like a hack?
    int topPosition = menuHeight;
    int bottomPosition = menuHeight;
    int columnWidth = 0;
    bool menuFound = false;
    for (unsigned int i = 0; i < rows.size(); ++i) {
        columnWidth = rows[i].getWidth();
        topPosition = bottomPosition + lineHeight;
        if (x > leftPosition && x < rightPosition && y > bottomPosition && y < topPosition) {
            mouseClickRow(i);
            menuFound = true;
            break;
        }
        bottomPosition = topPosition;
    }
    return menuFound;
}

void MenuColumn::setMouseOver(int leftPosition, int rightPosition, int topPosition, int bottomPosition) {
   leftMouseOver = leftPosition;
   rightMouseOver = rightPosition;
   topMouseOver = topPosition;
   bottomMouseOver = bottomPosition;
}

bool MenuColumn::mouseOver(int x, int y, int leftPosition, int menuHeight, int lineHeight) {

	int maxColumnWidth = this->getMaxRowWidth();

    int rightPosition = leftPosition + maxColumnWidth;
    int topPosition = menuHeight;
    int bottomPosition = menuHeight;
    bool overMenu = false;
    for (unsigned int i = 0; i < rows.size(); ++i) {
        topPosition = bottomPosition + lineHeight ;
        if (x > leftPosition && x < rightPosition && y > bottomPosition && y < topPosition) {
            setMouseOver(leftPosition, rightPosition, bottomPosition, topPosition);
            overMenu = true;
            break;
        }
        bottomPosition = topPosition;
    }
    if (!overMenu) {
        setMouseOver(0, 0, 0, 0);
    }
    return overMenu;
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

int MenuColumn::addRow(const char* rowName, MenuRowCallback callback) {
    MenuRow* row;
    row = new MenuRow(rowName, callback);
    rows.push_back(*row);
    delete row;
    return 0;
}

int MenuColumn::getMaxRowWidth() {
    float scale = 0.09;
    int mono = 0;
	int maxColumnWidth = 100 - (SPACE_BEFORE_ROW_NAME*2); // the minimum size we want
	for (unsigned int i = 0; i < rows.size(); ++i) {
		maxColumnWidth = std::max(maxColumnWidth,rows[i].getWidth(scale, mono, 0));
	}
	
	maxColumnWidth += SPACE_BEFORE_ROW_NAME*2; // space before and after!!
	return maxColumnWidth;
}

void MenuColumn::render(int yOffset, int menuHeight, int lineHeight) {
    float scale = 0.09;
    int mono = 0;
    int numberOfRows = rows.size();
    if (numberOfRows > 0) {

		int maxColumnWidth = this->getMaxRowWidth();
    
        glColor3f(0.9,0.9,0.9);
        glBegin(GL_QUADS); {
            glVertex2f(leftPosition,  yOffset + menuHeight);
            glVertex2f(leftPosition+maxColumnWidth, yOffset + menuHeight);
            glVertex2f(leftPosition+maxColumnWidth,  yOffset + menuHeight + numberOfRows*lineHeight);
            glVertex2f(leftPosition ,  yOffset + menuHeight + numberOfRows* lineHeight);
        }
        glEnd();
    }
    int y = menuHeight + lineHeight / 2 ;
    char* rowName;
    int columnWidth = 0;
    for (unsigned int i = 0; i < rows.size(); ++i) {
        rowName = rows[i].getName();
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
