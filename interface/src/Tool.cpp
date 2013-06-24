#include "Tool.h"

#include <QSvgRenderer>
#include <QPainter>
#include <QGLWidget>

Tool::Tool(QAction *action, GLuint texture, int x, int y) : _texture(texture),
                                                            _action(action),
                                                            _x(x),
                                                            _y(y),
                                                            _width(62),
                                                            _height(40) {
}

void Tool::render(int screenWidth, int screenHeight) {
    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, _texture);


    if (_action == 0 || _action->isChecked()) {
        glColor3f(1.0f, 1.0f, 1.0f); // reset gl color
    }
    else {
        glColor3f(0.3f, 0.3f, 0.3f);
    }

    glBegin(GL_QUADS);
    glTexCoord2f(_x/TOOLS_COLS, 1.0f - (_y + 1)/TOOLS_ROWS);
    glVertex2f(0, _height);

    glTexCoord2f((_x + 1)/TOOLS_COLS, 1.0f - (_y + 1)/TOOLS_ROWS);
    glVertex2f(_width, _height);

    glTexCoord2f((_x + 1)/TOOLS_COLS, 1.0f - _y/TOOLS_ROWS);
    glVertex2f(_width, 0);

    glTexCoord2f(_x/TOOLS_COLS, 1.0f - _y/TOOLS_ROWS);
    glVertex2f(0, 0);
    glEnd();

    glDisable(GL_TEXTURE_2D);

    glTranslated(0, _height + 5, 0);
}
