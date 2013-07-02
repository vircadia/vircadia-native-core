//
//  Swatch.h
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Swatch__
#define __interface__Swatch__

#define SWATCH_SIZE 8

#include "Tool.h"
#include "ui/TextRenderer.h"

class Swatch : public Tool {
public:
    Swatch(QAction* action);


    QColor getColor();
    void checkColor();
    void saveData(QSettings* settings);
    void loadData(QSettings* settings);
    void reset();

    void render(int width, int height);
    void handleEvent(int key, bool getColor);

private:
    TextRenderer _textRenderer;
    QColor _colors[SWATCH_SIZE];
    int _selected;
};

#endif /* defined(__interface__Swatch__) */
