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
	this->nameLength = strlen(columnName);
    strncpy(this->rowName, columnName, MAX_COLUMN_NAME); // copy the base name
    strncpy(this->rowName, this->getName(), MAX_COLUMN_NAME); // now add in state
    rowWidth = 0;
}

MenuRow::MenuRow(const char * columnName, MenuRowCallback callback, MenuStateNameCallback stateNameCallback) :
	callback(callback),
	stateNameCallback(stateNameCallback) {
	this->nameLength = strlen(columnName);
    strncpy(this->rowName, columnName, MAX_COLUMN_NAME);
    strncpy(this->rowName, this->getName(), MAX_COLUMN_NAME); // now add in state
    rowWidth = 0;
}

MenuRow::~MenuRow() {
}

void MenuRow::call() {
    callback(MENU_ROW_PICKED);
}

const char* MenuRow::getStateName() {
    int currentValue = callback(MENU_ROW_GET_VALUE);
    const char* stateName;
    // If the MenuRow has a custom stateNameCallback function, then call it to get a string 
    // to display in the menu. Otherwise, use the default implementation.
    if (stateNameCallback != NULL) {
    	stateName = stateNameCallback(currentValue);
    } else {
		if (currentValue == 0) {
			stateName = " OFF ";
		} else if (currentValue == 1) {
			stateName = " ON ";
		} else  {
			stateName = " ";
		}
    }
    return stateName;
}

char* MenuRow::getName() {
	strcpy(this->rowName + nameLength, getStateName());
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

