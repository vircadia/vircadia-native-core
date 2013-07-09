#include "Tool.h"

#include <QSvgRenderer>
#include <QPainter>
#include <QGLWidget>

Tool::Tool(QAction *action, GLuint texture, int x, int y) :
    _texture(texture),
    _action(action),
    _x(x),
    _y(y) {
}

void Tool::setAction(QAction* action) {
    _action = action;
}

bool Tool::isActive() {
    return _action->isChecked();
}

void Tool::render(int width, int height) {
    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, _texture);


    if (_action == 0 || _action->isChecked()) {
        glColor3f(1.0f, 1.0f, 1.0f); // reset gl color
    } else {
        glColor3f(0.3f, 0.3f, 0.3f);
    }

    glBegin(GL_QUADS);
    glTexCoord2f( _x      / NUM_TOOLS_COLS, 1.0f - (_y + 1) / NUM_TOOLS_ROWS);
    glVertex2f(0    , height);

    glTexCoord2f((_x + 1) / NUM_TOOLS_COLS, 1.0f - (_y + 1) / NUM_TOOLS_ROWS);
    glVertex2f(width, height);

    glTexCoord2f((_x + 1) / NUM_TOOLS_COLS, 1.0f -  _y      / NUM_TOOLS_ROWS);
    glVertex2f(width,      0);

    glTexCoord2f( _x      / NUM_TOOLS_COLS, 1.0f -  _y      / NUM_TOOLS_ROWS);
    glVertex2f(0    ,      0);
    glEnd();

    glDisable(GL_TEXTURE_2D);

    glTranslated(0, 1.10f * height, 0);
}
