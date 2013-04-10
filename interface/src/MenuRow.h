
#include <vector>

#define MAX_COLUMN_NAME 50
#define SPACE_BETWEEN_COLUMNS 20
#define SPACE_BEFORE_ROW_NAME 10
typedef int(CALLBACK * PFNRowCallback)(int);

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

