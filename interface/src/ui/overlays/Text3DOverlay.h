//
//  Text3DOverlay.h
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Text3DOverlay_h
#define hifi_Text3DOverlay_h

#include <QString>

#include "Billboard3DOverlay.h"

class TextRenderer3D;

class Text3DOverlay : public Billboard3DOverlay {
    Q_OBJECT
    
public:
    static QString const TYPE;
    virtual QString getType() const { return TYPE; }

    Text3DOverlay();
    Text3DOverlay(const Text3DOverlay* text3DOverlay);
    ~Text3DOverlay();
    virtual void render(RenderArgs* args);

    virtual void update(float deltatime);

    // getters
    const QString& getText() const { return _text; }
    float getLineHeight() const { return _lineHeight; }
    float getLeftMargin() const { return _leftMargin; }
    float getTopMargin() const { return _topMargin; }
    float getRightMargin() const { return _rightMargin; }
    float getBottomMargin() const { return _bottomMargin; }
    xColor getBackgroundColor();
    float getBackgroundAlpha() const { return _backgroundAlpha; }

    // setters
    void setText(const QString& text) { _text = text; }
    void setLineHeight(float value) { _lineHeight = value; }
    void setLeftMargin(float margin) { _leftMargin = margin; }
    void setTopMargin(float margin) { _topMargin = margin; }
    void setRightMargin(float margin) { _rightMargin = margin; }
    void setBottomMargin(float margin) { _bottomMargin = margin; }

    virtual void setProperties(const QScriptValue& properties);
    virtual QScriptValue getProperty(const QString& property);

    QSizeF textSize(const QString& test) const;  // Meters

    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance, 
                                        BoxFace& face, glm::vec3& surfaceNormal);

    virtual Text3DOverlay* createClone() const;

private:
    TextRenderer3D* _textRenderer = nullptr;
    
    QString _text;
    xColor _backgroundColor;
    float _backgroundAlpha;
    float _lineHeight;
    float _leftMargin;
    float _topMargin;
    float _rightMargin;
    float _bottomMargin;
};

 
#endif // hifi_Text3DOverlay_h
