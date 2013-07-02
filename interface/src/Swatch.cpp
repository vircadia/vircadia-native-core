#include "Swatch.h"
#include <iostream>

Swatch::Swatch(QAction* action) : Tool(action, 0, -1, -1),
                                  _selected(1),
                                  _textRenderer(MONO_FONT_FAMILY, 10, 100) {
}

void Swatch::reset() {
    _colors[0].setRgb(237, 175, 0);
    _colors[1].setRgb(161, 211, 72);
    _colors[2].setRgb(51, 204, 204);
    _colors[3].setRgb(63, 169, 245);
    _colors[4].setRgb(193, 99, 122);
    _colors[5].setRgb(255, 54, 69);
    _colors[6].setRgb(124, 36, 36);
    _colors[7].setRgb(63, 35, 19);
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
        QString name("R1");
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

    _colors[0].setRgb(settings->value("R1", 237).toInt(),
                      settings->value("G1", 175).toInt(),
                      settings->value("B1",   0).toInt());
    _colors[1].setRgb(settings->value("R2", 161).toInt(),
                      settings->value("G2", 211).toInt(),
                      settings->value("B2",  72).toInt());
    _colors[2].setRgb(settings->value("R3",  51).toInt(),
                      settings->value("G3", 204).toInt(),
                      settings->value("B3", 204).toInt());
    _colors[3].setRgb(settings->value("R4",  63).toInt(),
                      settings->value("G4", 169).toInt(),
                      settings->value("B4", 245).toInt());
    _colors[4].setRgb(settings->value("R5", 193).toInt(),
                      settings->value("G5",  99).toInt(),
                      settings->value("B5", 122).toInt());
    _colors[5].setRgb(settings->value("R6", 255).toInt(),
                      settings->value("G6",  54).toInt(),
                      settings->value("B6",  69).toInt());
    _colors[6].setRgb(settings->value("R7", 124).toInt(),
                      settings->value("G7",  36).toInt(),
                      settings->value("B7",  36).toInt());
    _colors[7].setRgb(settings->value("R8",  63).toInt(),
                      settings->value("G8",  35).toInt(),
                      settings->value("B8",  19).toInt());

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
    }
    else {
        _selected = next;
        QPixmap map(16, 16);
        map.fill(_colors[_selected - 1]);
        _action->setData(_colors[_selected - 1]) ;
        _action->setIcon(map);
    }
}

void Swatch::render(int width, int height) {
    char str[2];
    int margin = 0.10f*height;
    height = 0.75f*height;

    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(0, 8*(height - margin) + margin);
    glVertex2f(width, 8*(height - margin) + margin);
    glVertex2f(width, 0);
    glVertex2f(0, 0);
    glEnd();

    for (unsigned int i(0); i < SWATCH_SIZE; ++i) {
        glBegin(GL_QUADS);
        glColor3f(_colors[i].redF(),
                  _colors[i].greenF(),
                  _colors[i].blueF());
        glVertex2f(margin, (i + 1)*(height - margin));
        glVertex2f(width - margin, (i + 1)*(height - margin));
        glVertex2f(width - margin, i*(height - margin) + margin);
        glVertex2f(margin, i*(height - margin) + margin);
        glEnd();

        if (_colors[i].lightness() < 50) {
            glBegin(GL_LINES);
            glColor3f(1.0f, 1.0f, 1.0f);
            glVertex2f(margin, (i + 1)*(height - margin));
            glVertex2f(width - margin, (i + 1)*(height - margin));

            glVertex2f(width - margin, (i + 1)*(height - margin));
            glVertex2f(width - margin, i*(height - margin) + margin);

            glVertex2f(width - margin, i*(height - margin) + margin);
            glVertex2f(margin, i*(height - margin) + margin);

            glVertex2f(margin, i*(height - margin) + margin);
            glVertex2f(margin, (i + 1)*(height - margin));
            glEnd();


        }
        else {
            glColor3f(0.0f, 0.0f, 0.0f);
        }

        if (_selected == i + 1) {
            glBegin(GL_TRIANGLES);
            glVertex2f(margin, (i + 1)*(height - margin) - margin);
            glVertex2f(width/4 - margin, i*(height - margin) + height/2.0f);
            glVertex2f(margin, i*(height - margin) + margin + margin);
            glEnd();
        }

        sprintf(str, "%d", i + 1);
        _textRenderer.draw(3*width/4, (i + 1)*(height - margin) - 0.2f*height, str);
    }

    glTranslated(0, 8*(height - margin) + margin + 0.075f*height, 0);
}
