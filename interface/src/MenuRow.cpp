
#include <algorithm>
#include "InterfaceConfig.h"
#include "MenuRow.h"
#include "MenuColumn.h"
#include "Menu.h"
#include "Util.h"


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

