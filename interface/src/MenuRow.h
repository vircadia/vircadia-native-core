#ifndef __hifi__MenuRow__ 
#define __hifi__MenuRow__

const int MAX_COLUMN_NAME = 50;
const int SPACE_BETWEEN_COLUMNS = 20;
const int SPACE_BEFORE_ROW_NAME = 10;

typedef int(*MenuRowCallback)(int);

class MenuRow  {
public:
    MenuRow();
    MenuRow(const char * rowName, MenuRowCallback callback);
    ~MenuRow();
    void call();
    char * getName();
    int getWidth(float scale, int mono, int leftPosition);
    int getWidth();
private:
    char rowName[MAX_COLUMN_NAME];
    int rowWidth;
    MenuRowCallback callback;
};

#endif /* defined(__hifi__MenuRow__) */
