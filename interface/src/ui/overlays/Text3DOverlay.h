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

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QString>

#include <RenderArgs.h>
#include "Planar3DOverlay.h"

class Text3DOverlay : public Planar3DOverlay {
    Q_OBJECT
    
public:
    Text3DOverlay();
    ~Text3DOverlay();
    virtual void render(RenderArgs* args);

    // getters
    const QString& getText() const { return _text; }
    float getLineHeight() const { return _lineHeight; }
    float getLeftMargin() const { return _leftMargin; }
    float getTopMargin() const { return _topMargin; }
    float getRightMargin() const { return _rightMargin; }
    float getBottomMargin() const { return _bottomMargin; }
    bool getIsFacingAvatar() const { return _isFacingAvatar; }
    xColor getBackgroundColor();

    // setters
    void setText(const QString& text) { _text = text; }
    void setLineHeight(float value) { _lineHeight = value; }
    void setLeftMargin(float margin) { _leftMargin = margin; }
    void setTopMargin(float margin) { _topMargin = margin; }
    void setRightMargin(float margin) { _rightMargin = margin; }
    void setBottomMargin(float margin) { _bottomMargin = margin; }
    void setIsFacingAvatar(bool isFacingAvatar) { _isFacingAvatar = isFacingAvatar; }

    virtual void setProperties(const QScriptValue& properties);

    virtual Text3DOverlay* createClone();

private:
    virtual void writeToClone(Text3DOverlay* clone);
    void enableClipPlane(GLenum plane, float x, float y, float z, float w);

    QString _text;
    xColor _backgroundColor;
    float _lineHeight;
    float _leftMargin;
    float _topMargin;
    float _rightMargin;
    float _bottomMargin;
    bool _isFacingAvatar;
};

 
#endif // hifi_Text3DOverlay_h
