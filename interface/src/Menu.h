
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


class MenuColumn  {
    public:
        MenuColumn();
        MenuColumn(char * columnName);
        ~MenuColumn();
        void mouseClickRow(int iColumnIndex);
        bool mouseClick(int x, int y, int xLeft);
        void setMouseOver(int xLeft, int xRight, int yTop, int yBottom);
        bool mouseOver(int x, int y, int xLeft);
        char * getName();
        int getWidth(float scale, int mono, int leftPosition);
        int getWidth();
        int getLeftPosition();
        void render();
        void MenuColumn::renderMouseOver();
        int addRow(char * rowName, PFNRowCallback callback);
private:
    char columnName[MAX_COLUMN_NAME];
    int columnWidth;
    int leftPosition;
    std::vector<MenuRow> rows;
    int xLeftMouseOver;
    int xRightMouseOver;
    int yTopMouseOver;
    int yBottomMouseOver;
};

class Menu  {
    public:
        Menu();
        ~Menu();
        void mouseClickColumn(int iColumnIndex);
        void setMouseOver(int xLeft, int xRight, int yTop, int yBottom);
        void renderMouseOver();
        bool mouseClick(int x, int y);
        bool mouseOver(int x, int y);
        void render(int screenwidth, int screenheight);
        void renderColumn(int i);
        MenuColumn* addColumn(char *columnName);
private:
    std::vector<MenuColumn> columns;
    int iCurrentColumn;
    int xLeftMouseOver;
    int xRightMouseOver;
    int yTopMouseOver;
    int yBottomMouseOver;
};