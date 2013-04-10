#ifndef __hifi__MenuRow__ 
#define __hifi__MenuRow__

const int MAX_COLUMN_NAME = 50;
const int SPACE_BETWEEN_COLUMNS = 20;
const int SPACE_BEFORE_ROW_NAME = 10;

#define MenuCallBack CALLBACK

typedef int(MenuCallBack * PFNRowCallback)(int);

class MenuRow  {
public:
    MenuRow();
    MenuRow(char * rowName, PFNRowCallback);
    ~MenuRow();
    void call();
    char * getName();
    int getWidth(float scale, int mono, int leftPosition);
    int getWidth();
private:
    char rowName[MAX_COLUMN_NAME];
    int rowWidth;
    PFNRowCallback callback;
};

#endif /* defined(__hifi__MenuRow__) */
