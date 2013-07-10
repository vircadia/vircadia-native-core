#include "ToolsPalette.h"

#include <QSvgRenderer>
#include <QPainter>
#include <QGLWidget>
#include <SharedUtil.h>

void ToolsPalette::init(int screenWidth, int screenHeight) {
    _width = (PAL_SCREEN_RATIO * screenWidth < WIDTH_MIN) ? WIDTH_MIN : PAL_SCREEN_RATIO * screenWidth;
    _height = TOOLS_RATIO * _width;

    _left = screenWidth / 150;
    _top = (screenHeight - SWATCHS_TOOLS_COUNT * _height) / 2;

    // Load SVG
    switchToResourcesParentIfRequired();
    QSvgRenderer renderer(QString("./resources/images/hifi-interface-tools.svg"));

    // Prepare a QImage with desired characteritisc
    QImage image(NUM_TOOLS_COLS * _width, NUM_TOOLS_ROWS * _height, QImage::Format_ARGB32);

    // Get QPainter that paints to the image
    QPainter painter(&image);
    renderer.render(&painter);

    //get the OpenGL-friendly image
    _textureImage = QGLWidget::convertToGLFormat(image);

    glGenTextures(1, &_textureID);
    glBindTexture(GL_TEXTURE_2D, _textureID);

    //generate the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                  _textureImage.width(),
                  _textureImage.height(),
                  0, GL_RGBA, GL_UNSIGNED_BYTE,
                  _textureImage.bits());

    //texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}


void ToolsPalette::addAction(QAction* action, int x, int y) {
    Tool* tmp = new Tool(action, _textureID, x, y);
    _tools.push_back(tmp);
}

void ToolsPalette::addTool(Tool* tool) {
    _tools.push_back(tool);
}

void ToolsPalette::render(int screenWidth, int screenHeight) {
    _width = (PAL_SCREEN_RATIO * screenWidth < WIDTH_MIN) ? WIDTH_MIN : PAL_SCREEN_RATIO * screenWidth;
    _height = TOOLS_RATIO * _width;

    _left = screenWidth / 150;
    _top = (screenHeight - SWATCHS_TOOLS_COUNT * _height) / 2;

    glPushMatrix();
    glTranslated(_left, _top, 0);

    bool show = false;
    for (unsigned int i = 0; i < _tools.size(); ++i) {
        if (_tools[i]->isActive()) {
            show = true;
            break;
        }
    }

    if (show) {
        for (unsigned int i = 0; i < _tools.size(); ++i) {
            _tools[i]->render(_width, _height);
        }
    }

    glPopMatrix();
}
