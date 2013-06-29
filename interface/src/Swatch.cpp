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
    map.fill(_colors[_selected - 1]);
    _action->setData(_colors[_selected - 1]) ;
    _action->setIcon(map);
}

QColor Swatch::getColor() {
    return _colors[_selected - 1];
}

void Swatch::saveData(QSettings* settings) {
    settings->beginGroup("Swatch");
    
    for (int i(0); i < SWATCH_SIZE; ++i) {
        QString name("R0");
        name[1] = '1' + i;
        settings->setValue(name, _colors[i].red());
        name[0] = 'G';
        settings->setValue(name, _colors[i].green());
        name[0] = 'B';
        settings->setValue(name, _colors[i].blue());
    }
    
    settings->endGroup();
}

void Swatch::loadData(QSettings* settings) {
    settings->beginGroup("Swatch");
    
    _colors[0].setRgb(settings->value("R1", 128).toInt(),
                      settings->value("G1", 128).toInt(),
                      settings->value("B1", 128).toInt());
    _colors[1].setRgb(settings->value("R2", 128).toInt(),
                      settings->value("G2", 128).toInt(),
                      settings->value("B2", 128).toInt());
    _colors[2].setRgb(settings->value("R3", 128).toInt(),
                      settings->value("G3", 128).toInt(),
                      settings->value("B3", 128).toInt());
    _colors[3].setRgb(settings->value("R4", 128).toInt(),
                      settings->value("G4", 128).toInt(),
                      settings->value("B4", 128).toInt());
    _colors[4].setRgb(settings->value("R5", 128).toInt(),
                      settings->value("G5", 128).toInt(),
                      settings->value("B5", 128).toInt());
    _colors[5].setRgb(settings->value("R6", 128).toInt(),
                      settings->value("G6", 128).toInt(),
                      settings->value("B6", 128).toInt());
    _colors[6].setRgb(settings->value("R7", 128).toInt(),
                      settings->value("G7", 128).toInt(),
                      settings->value("B7", 128).toInt());
    _colors[7].setRgb(settings->value("R8", 128).toInt(),
                      settings->value("G8", 128).toInt(),
                      settings->value("B8", 128).toInt());
    
    settings->endGroup();
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
    }
    else {
        _selected = next;
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

        if (_colors[i].lightness() < 50) {
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
