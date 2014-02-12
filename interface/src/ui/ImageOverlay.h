//
//  ImageOverlay.h
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__ImageOverlay__
#define __interface__ImageOverlay__

#include <QImage>
#include <QGLWidget>
#include <QRect>
#include <QString>
#include <QUrl>

#include <SharedUtil.h>

#include "InterfaceConfig.h"
//#include "Util.h"


class ImageOverlay : QObject {
    Q_OBJECT
    Q_PROPERTY(int x READ getX WRITE setX)
    Q_PROPERTY(int y READ getY WRITE setY)
    Q_PROPERTY(int width READ getWidth WRITE setWidth)
    Q_PROPERTY(int height READ getHeight WRITE setHeight)
    Q_PROPERTY(QRect bounds READ getBounds WRITE setBounds)
    Q_PROPERTY(QRect subImage READ getClipFromSource WRITE setClipFromSource)
    Q_PROPERTY(xColor backgroundColor READ getBackgroundColor WRITE setBackgroundColor)
    Q_PROPERTY(float alpha READ getAlpha WRITE setAlpha)
    Q_PROPERTY(QUrl imageURL READ getImageURL WRITE setImageURL)
public:
    ImageOverlay();
    ~ImageOverlay();
    void init(QGLWidget* parent, const QString& filename, const QRect& drawAt, const QRect& fromImage);
    void render();

    // getters
    int getX() const { return _bounds.x(); }
    int getY() const { return _bounds.y(); }
    int getWidth() const { return _bounds.width(); }
    int getHeight() const { return _bounds.height(); }
    const QRect& getBounds() const { return _bounds; }
    const QRect& getClipFromSource() const { return _fromImage; }
    const xColor& getBackgroundColor() const { return _backgroundColor; }
    float getAlpha() const { return _alpha; }
    const QUrl& getImageURL() const { return _imageURL; }

    // setters
    void setX(int x) { }
    void setY(int y) { }
    void setWidth(int width) { }
    void setHeight(int height) { }
    void setBounds(const QRect& bounds) { }
    void setClipFromSource(const QRect& bounds) { }
    void setBackgroundColor(const xColor& color) { }
    void setAlpha(float) { }
    void setImageURL(const QUrl& ) { }

private:
    QUrl _imageURL;
    QGLWidget* _parent;
    QImage _textureImage;
    GLuint _textureID;
    QRect _bounds; // where on the screen to draw
    QRect _fromImage; // where from in the image to sample
    float _alpha;
    xColor _backgroundColor;
};

#endif /* defined(__interface__ImageOverlay__) */
