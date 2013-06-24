#include "Swatch.h"
#include <iostream>

Swatch::Swatch(QAction* action) : Tool(action, 0, -1, -1),
                                  _selected(1),
                                  _margin(4),
                                  _textRenderer(SANS_FONT_FAMILY, -1, 100) {
    _width = 62;
    _height = 30;

    _colors[0].setRgb(128, 128, 128);
    _colors[1].setRgb(255, 0, 0);
    _colors[2].setRgb(0, 255, 0);
    _colors[3].setRgb(0, 0, 255);
    _colors[4].setRgb(255, 0, 255);
    _colors[5].setRgb(255, 255, 0);
    _colors[6].setRgb(0, 255, 255);
    _colors[7].setRgb(0, 0, 0);

    QPixmap map(16, 16);
    map.fill(_colors[0]);
    _action->setData(_colors[_selected - 1]) ;
    _action->setIcon(map);
}

void Swatch::handleEvent(int key, bool getColor) {
    switch (key) {
        case Qt::Key_1:
            _selected = 1;
            break;
        case Qt::Key_2:
            _selected = 2;
            break;
        case Qt::Key_3:
            _selected = 3;
            break;
        case Qt::Key_4:
            _selected = 4;
            break;
        case Qt::Key_5:
            _selected = 5;
            break;
        case Qt::Key_6:
            _selected = 6;
            break;
        case Qt::Key_7:
            _selected = 7;
            break;
        case Qt::Key_8:
            _selected = 8;
            break;
        default:
            break;
    }

    if (getColor) {
        _colors[_selected - 1] = _action->data().value<QColor>();
    }
    else {
        QPixmap map(16, 16);
        map.fill(_colors[_selected - 1]);
        _action->setData(_colors[_selected - 1]) ;
        _action->setIcon(map);
    }
}

void Swatch::render(int screenWidth, int screenHeight) {
    char str[2];

    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(0, 8*(_height - _margin) + _margin);
    glVertex2f(_width, 8*(_height - _margin) + _margin);
    glVertex2f(_width, 0);
    glVertex2f(0, 0);
    glEnd();

    for (unsigned int i(0); i < SWATCH_SIZE; ++i) {
        glBegin(GL_QUADS);
        glColor3f(_colors[i].redF(),
                  _colors[i].greenF(),
                  _colors[i].blueF());
        glVertex2f(_margin, (i + 1)*(_height - _margin));
        glVertex2f(_width - _margin, (i + 1)*(_height - _margin));
        glVertex2f(_width - _margin, i*(_height - _margin) + _margin);
        glVertex2f(_margin, i*(_height - _margin) + _margin);
        glEnd();

        if (_colors[i].lightness() < 100) {
            glBegin(GL_LINES);
            glColor3f(1.0f, 1.0f, 1.0f);
            glVertex2f(_margin, (i + 1)*(_height - _margin));
            glVertex2f(_width - _margin, (i + 1)*(_height - _margin));

            glVertex2f(_width - _margin, (i + 1)*(_height - _margin));
            glVertex2f(_width - _margin, i*(_height - _margin) + _margin);

            glVertex2f(_width - _margin, i*(_height - _margin) + _margin);
            glVertex2f(_margin, i*(_height - _margin) + _margin);

            glVertex2f(_margin, i*(_height - _margin) + _margin);
            glVertex2f(_margin, (i + 1)*(_height - _margin));
            glEnd();


        }
        else {
            glColor3f(0.0f, 0.0f, 0.0f);
        }

        if (_selected == i + 1) {
            glBegin(GL_TRIANGLES);
            glVertex2f(_margin, (i + 1)*(_height - _margin) - _margin);
            glVertex2f(_width/4 - _margin, i*(_height - _margin) + _height/2.0f);
            glVertex2f(_margin, i*(_height - _margin) + _margin + _margin);
            glEnd();
        }

        sprintf(str, "%d", i + 1);
        _textRenderer.draw(3*_width/4, (i + 1)*(_height - _margin) - 6, str);
    }

    glTranslated(0, 8*(_height - _margin) + _margin + 5, 0);

}
