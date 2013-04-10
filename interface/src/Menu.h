#ifndef __hifi__Menu__ 
#define __hifi__Menu__

#include <vector>


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
    MenuColumn* addColumn(const char *columnName);
private:
    std::vector<MenuColumn> columns;
    int currentColumn;
    int leftMouseOver;
    int rightMouseOver;
    int topMouseOver;
    int bottomMouseOver;
};
#endif /* defined(__hifi__Menu__) */
