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

#include <QString>

#include <SharedUtil.h>

#include "Overlay2D.h"

const xColor DEFAULT_BACKGROUND_COLOR = { 0, 0, 0 };
const float DEFAULT_BACKGROUND_ALPHA = 0.7f;
const int DEFAULT_MARGIN = 10;
const int DEFAULT_FONTSIZE = 12;
const int DEFAULT_FONT_WEIGHT = 50;

class TextOverlayElement;

class TextOverlay : public Overlay2D {
    Q_OBJECT
    
public:
    TextOverlay();
    TextOverlay(const TextOverlay* textOverlay);
    ~TextOverlay();
    virtual void render(RenderArgs* args);

    // getters
    const QString& getText() const { return _text; }
    int getLeftMargin() const { return _leftMargin; }
    int getTopMargin() const { return _topMargin; }
    xColor getBackgroundColor();
    float getBackgroundAlpha() const { return _backgroundAlpha; }

    // setters
    void setText(const QString& text);
    void setLeftMargin(int margin);
    void setTopMargin(int margin);
    void setFontSize(int fontSize);

    virtual void setProperties(const QScriptValue& properties);
    virtual TextOverlay* createClone() const;
    virtual QScriptValue getProperty(const QString& property);

    QSizeF textSize(const QString& text) const;  // Pixels

private:
    TextOverlayElement* _qmlElement{ nullptr };
    QString _text;
    xColor _backgroundColor;
    float _backgroundAlpha;
    int _leftMargin;
    int _topMargin;
    int _fontSize;
};

 
#endif // hifi_TextOverlay_h
