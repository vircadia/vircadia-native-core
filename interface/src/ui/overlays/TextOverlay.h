//
//  TextOverlay.h
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TextOverlay_h
#define hifi_TextOverlay_h

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <QImage>
#include <QNetworkReply>
#include <QRect>
#include <QScriptValue>
#include <QString>
#include <QUrl>

#include <SharedUtil.h>

#include "Overlay.h"
#include "Overlay2D.h"

const xColor DEFAULT_BACKGROUND_COLOR = { 0, 0, 0 };
const int DEFAULT_MARGIN = 10;
const int DEFAULT_FONTSIZE = 11;
const int DEFAULT_FONT_WEIGHT = 50;

class TextOverlay : public Overlay2D {
    Q_OBJECT
    
public:
    TextOverlay();
    TextOverlay(TextOverlay* textOverlay);
    ~TextOverlay();
    virtual void render(RenderArgs* args);

    // getters
    const QString& getText() const { return _text; }
    int getLeftMargin() const { return _leftMargin; }
    int getTopMargin() const { return _topMargin; }
    xColor getBackgroundColor();

    // setters
    void setText(const QString& text) { _text = text; }
    void setLeftMargin(int margin) { _leftMargin = margin; }
    void setTopMargin(int margin) { _topMargin = margin; }
    void setFontSize(int fontSize) { _fontSize = fontSize; }

    virtual void setProperties(const QScriptValue& properties);
    virtual TextOverlay* createClone();
    virtual QScriptValue getProperty(const QString& property);

    float textWidth(const QString& text) const;  // Pixels

private:
    QString _text;
    xColor _backgroundColor;
    int _leftMargin;
    int _topMargin;
    int _fontSize;
};

 
#endif // hifi_TextOverlay_h
