//
//  ImageOverlay.h
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__ImageOverlay__
#define __interface__ImageOverlay__

#include <QGLWidget>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRect>
#include <QScriptValue>
#include <QString>
#include <QUrl>

#include <SharedUtil.h>

#include "InterfaceConfig.h"

const xColor DEFAULT_BACKGROUND_COLOR = { 255, 255, 255 };
const float DEFAULT_ALPHA = 0.7f;

class ImageOverlay : QObject {
    Q_OBJECT
    
public:
    ImageOverlay();
    ~ImageOverlay();
    void init(QGLWidget* parent);
    void render();

//public slots:
    // getters
    bool getVisible() const { return _visible; }
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
    void setVisible(bool visible) { _visible = visible; }
    void setX(int x) { _bounds.setX(x); }
    void setY(int y) { _bounds.setY(y);  }
    void setWidth(int width) { _bounds.setWidth(width); }
    void setHeight(int height) {  _bounds.setHeight(height); }
    void setBounds(const QRect& bounds) { _bounds = bounds; }
    void setClipFromSource(const QRect& bounds) { _fromImage = bounds; _wantClipFromImage = true; }
    void setBackgroundColor(const xColor& color) { _backgroundColor = color; }
    void setAlpha(float alpha) { _alpha = alpha; }
    void setImageURL(const QUrl& url);
    void setProperties(const QScriptValue& properties);

private slots:
    void replyFinished(QNetworkReply* reply); // we actually want to hide this...

private:

    QUrl _imageURL;
    QGLWidget* _parent;
    QImage _textureImage;
    GLuint _textureID;
    QRect _bounds; // where on the screen to draw
    QRect _fromImage; // where from in the image to sample
    float _alpha;
    xColor _backgroundColor;
    bool _visible; // should the overlay be drawn at all
    bool _renderImage; // is there an image associated with this overlay, or is it just a colored rectangle
    bool _textureBound; // has the texture been bound
    bool _wantClipFromImage;
};

 
#endif /* defined(__interface__ImageOverlay__) */
