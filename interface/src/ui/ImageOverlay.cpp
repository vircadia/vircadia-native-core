#include "ImageOverlay.h"

#include <QSvgRenderer>
#include <QPainter>
#include <QGLWidget>
#include <SharedUtil.h>

ImageOverlay::ImageOverlay() :
    _parent(NULL),
    _textureID(0)
{
}    

void ImageOverlay::init(QGLWidget* parent, const QString& filename, const QRect& drawAt, const QRect& fromImage) {
    _parent = parent;
    qDebug() << "ImageOverlay::init()... filename=" << filename;
    _bounds = drawAt;
    _fromImage = fromImage;
    _textureImage = QImage(filename);
    _textureID = _parent->bindTexture(_textureImage);
}


ImageOverlay::~ImageOverlay() {
    if (_parent && _textureID) {
        // do we need to call this?
        //_parent->deleteTexture(_textureID);
    }
}


void ImageOverlay::render() {
qDebug() << "ImageOverlay::render _textureID=" << _textureID << "_bounds=" << _bounds;


    bool renderImage = false;
    if (renderImage) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, _textureID);
    }
    glColor4f(1.0f, 0.0f, 0.0f, 0.7f); // ???

    float imageWidth = _textureImage.width();
    float imageHeight = _textureImage.height();
    float x = _fromImage.x() / imageWidth;
    float y = _fromImage.y() / imageHeight;
    float w = _fromImage.width() / imageWidth; // ?? is this what we want? not sure
    float h = _fromImage.height() / imageHeight;

    qDebug() << "ImageOverlay::render x=" << x << "y=" << y << "w="<<w << "h="<<h << "(1.0f - y)=" << (1.0f - y);
    
    glBegin(GL_QUADS);
        //glTexCoord2f(x, 1.0f - y);
        glVertex2f(_bounds.left(), _bounds.top());

        //glTexCoord2f(x + w, 1.0f - y);
        glVertex2f(_bounds.right(), _bounds.top());

        //glTexCoord2f(x + w, 1.0f - (y + h));
        glVertex2f(_bounds.right(), _bounds.bottom());

        //glTexCoord2f(x, 1.0f - (y + h));
        glVertex2f(_bounds.left(), _bounds.bottom());
    glEnd();
    if (renderImage) {
        glDisable(GL_TEXTURE_2D);
    }
}

