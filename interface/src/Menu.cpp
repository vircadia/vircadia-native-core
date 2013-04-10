
#include <algorithm>
#include "InterfaceConfig.h"
#include "Menu.h"
#include "Util.h"

int menuHeight = 30;
int lineHeight = 30;
int yOffset = 8;    // under windows we have 8 vertical pixels offset. In 2D an object with y=8, the object is displayed at y=0
                    // change the value in the other platforms (if required).

MenuRow::MenuRow()
{
}
MenuRow::MenuRow(char * columnName, PFNRowCallback callback)
{
    int length = std::min(MAX_COLUMN_NAME - 5,(int) strlen(columnName));
    strncpy(this->rowName, columnName, length);
    memcpy(this->rowName + length, "    \0", 5);
    this->callback = callback;
    rowWidth = 0;
}

MenuRow::~MenuRow()
{
}

void MenuRow::call() {
    callback(-2);
}

char * MenuRow::getName() {
    int length = (int) strlen(this->rowName) - 4;
    int currentValue = callback(-1);
    if (currentValue == 0) {
        memcpy(this->rowName + length, " OFF\0", 5);
    } else if (currentValue == 1) {
        memcpy(this->rowName + length, " ON \0", 5);
    } else  {
        memcpy(this->rowName + length, "    \0", 5);
    }
    return this->rowName;
}

int MenuRow::getWidth(float scale, int mono, int leftPosition){
    if (rowWidth == 0) {
        rowWidth = widthText( scale, mono, this->rowName);
    }  
    return rowWidth;
}

int MenuRow::getWidth(){
    return rowWidth;
}


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

bool MenuColumn::mouseClick(int x, int y, int xLeft) {
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

bool MenuColumn::mouseOver(int x, int y, int xLeft) {
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

void MenuColumn::render() {
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
    renderMouseOver();
}

void MenuColumn::renderMouseOver() {
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
             bRet = columns[i].mouseClick(x, y, xLeft);
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
             columns[i].mouseOver(x, y, xLeft);
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
            columns[i].render();
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