//
//  Swatch.h
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Swatch__
#define __interface__Swatch__

#include "Tool.h"
#include "ui/TextRenderer.h"

static const int SWATCH_SIZE = 8;
static const int colorBase[8][3] = {{237, 175, 0},
                                    {61, 211, 72},
                                    {51, 204, 204},
                                    {63, 169, 245},
                                    {193, 99, 122},
                                    {255, 54, 69},
                                    {124, 36, 36},
                                    {63, 35, 19}};

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
