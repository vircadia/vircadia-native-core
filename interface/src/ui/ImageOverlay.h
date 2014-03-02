//
//  ImageOverlay.h
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__ImageOverlay__
#define __interface__ImageOverlay__

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRect>
#include <QScriptValue>
#include <QString>
#include <QUrl>

#include <SharedUtil.h>

#include "Overlay.h"
#include "Overlay2D.h"

class ImageOverlay : public Overlay2D {
    Q_OBJECT
    
public:
    ImageOverlay();
    ~ImageOverlay();
    virtual void render();

    // getters
    const QRect& getClipFromSource() const { return _fromImage; }
    const QUrl& getImageURL() const { return _imageURL; }

    // setters
    void setClipFromSource(const QRect& bounds) { _fromImage = bounds; _wantClipFromImage = true; }
    void setImageURL(const QUrl& url);
    virtual void setProperties(const QScriptValue& properties);

private slots:
    void replyFinished(QNetworkReply* reply); // we actually want to hide this...

private:

    QUrl _imageURL;
    QImage _textureImage;
    GLuint _textureID;
    QRect _fromImage; // where from in the image to sample
    bool _renderImage; // is there an image associated with this overlay, or is it just a colored rectangle
    bool _textureBound; // has the texture been bound
    bool _wantClipFromImage;
};

 
#endif /* defined(__interface__ImageOverlay__) */
