//
//  ImageOverlay.h
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ImageOverlay_h
#define hifi_ImageOverlay_h

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <QImage>
#include <QNetworkReply>
#include <QRect>
#include <QScriptValue>
#include <QString>
#include <QUrl>

#include <NetworkAccessManager.h>
#include <SharedUtil.h>

#include "Overlay.h"
#include "Overlay2D.h"

class ImageOverlay : public Overlay2D {
    Q_OBJECT
    
public:
    ImageOverlay();
    ~ImageOverlay();
    virtual void render(RenderArgs* args);

    // getters
    const QRect& getClipFromSource() const { return _fromImage; }
    const QUrl& getImageURL() const { return _imageURL; }

    // setters
    void setClipFromSource(const QRect& bounds) { _fromImage = bounds; _wantClipFromImage = true; }
    void setImageURL(const QUrl& url);
    virtual void setProperties(const QScriptValue& properties);

    virtual ImageOverlay* createClone();

private slots:
    void replyFinished(); // we actually want to hide this...

private:
    virtual void writeToClone(ImageOverlay* clone);

    QUrl _imageURL;
    QImage _textureImage;

    GLuint _textureID;
    QRect _fromImage; // where from in the image to sample
    bool _renderImage; // is there an image associated with this overlay, or is it just a colored rectangle
    bool _textureBound; // has the texture been bound
    bool _wantClipFromImage;
};

 
#endif // hifi_ImageOverlay_h
