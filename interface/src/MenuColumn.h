#ifndef __hifi__MenuColumn__
#define __hifi__MenuColumn__
#include <vector>

class MenuColumn  {
public:
    MenuColumn();
    MenuColumn(char * columnName);
    ~MenuColumn();
    void mouseClickRow(int iColumnIndex);
    bool mouseClick(int x, int y, int xLeft, int menuHeight, int lineHeight);
    void setMouseOver(int xLeft, int xRight, int yTop, int yBottom);
    bool mouseOver(int x, int y, int xLeft, int menuHeight, int lineHeight);
    char* getName();
    int getWidth(float scale, int mono, int leftPosition);
    int getWidth();
    int getLeftPosition();
    void render(int yOffset, int menuHeight, int lineHeight);
    void MenuColumn::renderMouseOver(int yOffset);
    int addRow(char * rowName, PFNRowCallback callback);
private:
    char columnName[MAX_COLUMN_NAME];
    int columnWidth;
    int leftPosition;
    std::vector<MenuRow> rows;
    int leftMouseOver;
    int rightMouseOver;
    int topMouseOver;
    int bottomMouseOver;
};

#endif /* defined(__hifi__MenuColumn__) */

