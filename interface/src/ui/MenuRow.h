//
//  MenuRow.h
//  hifi
//
//  Created by Dominque Vincent on 4/10/13.
//
//

#ifndef __hifi__MenuRow__
#define __hifi__MenuRow__

const int MAX_COLUMN_NAME = 50;
const int SPACE_BETWEEN_COLUMNS = 20;
const int SPACE_BEFORE_ROW_NAME = 10;
const int MENU_ROW_PICKED = -2;
const int MENU_ROW_GET_VALUE = -1;

typedef int(*MenuRowCallback)(int);
typedef const char*(*MenuStateNameCallback)(int);

class MenuRow  {
public:
    MenuRow();
    MenuRow(const char* rowName, MenuRowCallback callback);
    MenuRow(const char* rowName, MenuRowCallback callback, MenuStateNameCallback stateNameCallback);
    ~MenuRow();
    void call();
    char * getName();
	const char* getStateName();
    int getWidth(float scale, int mono, int leftPosition);
    int getWidth();
private:
	int nameLength;
    char rowName[MAX_COLUMN_NAME];
    int rowWidth;
    MenuRowCallback callback;
    MenuStateNameCallback stateNameCallback;
};

#endif /* defined(__hifi__MenuRow__) */
