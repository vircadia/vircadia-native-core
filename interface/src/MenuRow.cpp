//
//  MenuRow.cpp
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

MenuRow::MenuRow() :
	callback(NULL),
	stateNameCallback(NULL) {
}

MenuRow::MenuRow(const char * columnName, MenuRowCallback callback) :
	callback(callback),
	stateNameCallback(NULL) {
    int length = std::min(MAX_COLUMN_NAME - 5,(int) strlen(columnName));
    strncpy(this->rowName, columnName, length);
    memcpy(this->rowName + length, "    \0", 5);
    rowWidth = 0;
}

MenuRow::MenuRow(const char * columnName, MenuRowCallback callback, MenuStateNameCallback stateNameCallback) :
	callback(callback),
	stateNameCallback(stateNameCallback) {
    int length = std::min(MAX_COLUMN_NAME - 5,(int) strlen(columnName));
    strncpy(this->rowName, columnName, length);
    
    // note: it would be good to also include the initial state
    memcpy(this->rowName + length, "    \0", 5);
    rowWidth = 0;
}

MenuRow::~MenuRow() {
}

void MenuRow::call() {
    callback(MENU_ROW_PICKED);
}

char* MenuRow::getName() {
    int length = (int) strlen(this->rowName) - 4;
    int currentValue = callback(MENU_ROW_GET_VALUE);
    
    // If the MenuRow has a custom stateNameCallback function, then call it to get a string 
    // to display in the menu. Otherwise, use the default implementation.
    if (stateNameCallback != NULL) {
    	const char* stateName = stateNameCallback(currentValue);
    	int stateNameLength = strlen(stateName);
    	printf("MenuRow::getName() stateName=%s stateNameLength=%d\n",stateName,stateNameLength);
    
    	// XXXBHG - BUG!!! - only 5 characters?? someplace else hard coded? for some reason, we end up 
    	// with memory corruption down the line, if we allow these states to be longer than 5 characters 
    	// including the NULL termination.
		strncpy(this->rowName + length, stateName,4); // only 4 chars
		memcpy(this->rowName + length + 5, "\0", 0); // null terminate!!
		
    } else {
		if (currentValue == 0) {
			memcpy(this->rowName + length, " OFF\0", 5);
		} else if (currentValue == 1) {
			memcpy(this->rowName + length, " ON \0", 5);
		} else  {
			memcpy(this->rowName + length, "    \0", 5);
		}
	}
    return this->rowName;
}

int MenuRow::getWidth(float scale, int mono, int leftPosition) {
    if (rowWidth == 0) {
        rowWidth = widthText( scale, mono, this->rowName);
    }  
    return rowWidth;
}

int MenuRow::getWidth() {
    return rowWidth;
}

