#include "Swatch.h"
#include <iostream>

Swatch::Swatch(QAction* action) :
    Tool(action, 0, -1, -1),
    _selected(1),
    _textRenderer(MONO_FONT_FAMILY, 10, 100) {
}

void Swatch::reset() {
    for (int i = 0; i < 8; ++i) {
        _colors[i].setRgb(colorBase[i][0],
                          colorBase[i][1],
                          colorBase[i][2]);
    }
}

QColor Swatch::getColor() {
    return _colors[_selected - 1];
}

void Swatch::checkColor() {
    if (_action->data().value<QColor>() == _colors[_selected - 1]) {
        return;
    }

    QPixmap map(16, 16);
    map.fill(_colors[_selected - 1]);
    _action->setData(_colors[_selected - 1]) ;
    _action->setIcon(map);
}

void Swatch::saveData(QSettings* settings) {
    settings->beginGroup("Swatch");
    
    for (int i(0); i < SWATCH_SIZE; ++i) {
        QString rx("R1"), gx("G1"), bx("B1");
        rx[1] = '1' + i;
        gx[1] = rx[1];
        bx[1] = rx[1];
        settings->setValue(rx, _colors[i].red());
        settings->setValue(gx, _colors[i].green());
        settings->setValue(bx, _colors[i].blue());
    }
    
    settings->endGroup();
}

void Swatch::loadData(QSettings* settings) {
    settings->beginGroup("Swatch");

    for (int i = 0; i < SWATCH_SIZE; ++i) {
        QString rx("R1"), gx("G1"), bx("B1");
        rx[1] = '1' + i;
        gx[1] = rx[1];
        bx[1] = rx[1];
        _colors[i].setRgb(settings->value(rx, colorBase[i][0]).toInt(),
                          settings->value(gx, colorBase[i][1]).toInt(),
                          settings->value(bx, colorBase[i][2]).toInt());
    }

    settings->endGroup();

    checkColor();
}

void Swatch::handleEvent(int key, bool getColor) {
    int next(0);
    
    switch (key) {
        case Qt::Key_1:
            next = 1;
            break;
        case Qt::Key_2:
            next = 2;
            break;
        case Qt::Key_3:
            next = 3;
            break;
        case Qt::Key_4:
            next = 4;
            break;
        case Qt::Key_5:
            next = 5;
            break;
        case Qt::Key_6:
            next = 6;
            break;
        case Qt::Key_7:
            next = 7;
            break;
        case Qt::Key_8:
            next = 8;
            break;
        default:
            break;
    }

    if (getColor) {
        if (_action->data().value<QColor>() != _colors[_selected - 1]) {
            _selected = next;
            _colors[_selected - 1] = _action->data().value<QColor>();
        }
    } else {
        _selected = next;
        QPixmap map(16, 16);
        map.fill(_colors[_selected - 1]);
        _action->setData(_colors[_selected - 1]) ;
        _action->setIcon(map);
    }
}

void Swatch::render(int width, int height) {
    char str[2];
    int margin = 0.10f * height;
    height = 0.75f * height;

    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(0, 8 * (height - margin) + margin);
    glVertex2f(width, 8 * (height - margin) + margin);
    glVertex2f(width, 0);
    glVertex2f(0, 0);
    glEnd();

    for (unsigned int i = 0; i < SWATCH_SIZE; ++i) {
        glBegin(GL_QUADS);
        glColor3f(_colors[i].redF(),
                  _colors[i].greenF(),
                  _colors[i].blueF());
        glVertex2f(margin        , (i + 1) * (height - margin));
        glVertex2f(width - margin, (i + 1) * (height - margin));
        glVertex2f(width - margin,  i      * (height - margin) + margin);
        glVertex2f(margin        ,  i      * (height - margin) + margin);
        glEnd();

        if (_colors[i].lightness() < 50) {
            glBegin(GL_LINES);
            glColor3f(1.0f, 1.0f, 1.0f);
            glVertex2f(margin        , (i + 1) * (height - margin));
            glVertex2f(width - margin, (i + 1) * (height - margin));

            glVertex2f(width - margin, (i + 1) * (height - margin));
            glVertex2f(width - margin,  i      * (height - margin) + margin);

            glVertex2f(width - margin,  i      * (height - margin) + margin);
            glVertex2f(margin        ,  i      * (height - margin) + margin);

            glVertex2f(margin        ,  i      * (height - margin) + margin);
            glVertex2f(margin        , (i + 1) * (height - margin));
            glEnd();
        } else {
            glColor3f(0.0f, 0.0f, 0.0f);
        }

        if (_selected == i + 1) {
            glBegin(GL_TRIANGLES);
            glVertex2f(margin          , (i + 1) * (height - margin) - margin);
            glVertex2f(width/4 - margin,  i      * (height - margin) + height / 2.0f);
            glVertex2f(margin          ,  i      * (height - margin) + margin + margin);
            glEnd();
        }

        sprintf(str, "%d", i + 1);
        _textRenderer.draw(3 * width/4, (i + 1) * (height - margin) - 0.2f * height, str);
    }

    glTranslated(0, 8 * (height - margin) + margin + 0.075f * height, 0);
}
