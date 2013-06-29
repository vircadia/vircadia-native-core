#include "ToolsPalette.h"

#include <QSvgRenderer>
#include <QPainter>
#include <QGLWidget>

ToolsPalette::ToolsPalette() {
    // Load SVG
    QSvgRenderer renderer(QString("/Users/graysonstebbins/Documents/hifi/interface/resources/images/hifi-interface-tools.svg"));

    // Prepare a QImage with desired characteritisc
    QImage image(124, 400, QImage::Format_ARGB32);

    // Get QPainter that paints to the image
    QPainter painter(&image);
    renderer.render(&painter);

    //get the OpenGL-friendly image
    _textureImage = QGLWidget::convertToGLFormat(image);
}

void ToolsPalette::init(int top, int left) {
    _top = top;
    _left = left;

    glGenTextures(1, &_textureID);
    glBindTexture(GL_TEXTURE_2D, _textureID);

    //generate the texture
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA,
                  _textureImage.width(),
                  _textureImage.height(),
                  0, GL_RGBA, GL_UNSIGNED_BYTE,
                  _textureImage.bits() );

    //texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}


void ToolsPalette::addAction(QAction* action, int x, int y) {
    Tool* tmp = new Tool(action, _textureID, x, y);
    _tools.push_back(tmp);
}

void ToolsPalette::addTool(Tool *tool) {
    _tools.push_back(tool);
}

void ToolsPalette::render(int screenWidth, int screenHeight) {
    glPushMatrix();
    glTranslated(_left, _top, 0);

    for (unsigned int i(0); i < _tools.size(); ++i) {
        _tools[i]->render(screenWidth, screenHeight);
    }

    glPopMatrix();
}
