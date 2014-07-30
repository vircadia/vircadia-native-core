//
//  Overlay.h
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Overlay_h
#define hifi_Overlay_h

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <QRect>
#include <QScriptValue>
#include <QString>

#include <SharedUtil.h> // for xColor

const xColor DEFAULT_OVERLAY_COLOR = { 255, 255, 255 };
const float DEFAULT_ALPHA = 0.7f;

class Overlay : public QObject {
    Q_OBJECT
    
public:
    enum Anchor {
        NO_ANCHOR,
        MY_AVATAR
    };
    
    Overlay();
    ~Overlay();
    void init(QGLWidget* parent);
    virtual void update(float deltatime) {}
    virtual void render() = 0;

    // getters
    bool isLoaded() { return _isLoaded; }
    bool getVisible() const { return _visible; }
    const xColor& getColor() const { return _color; }
    float getAlpha() const { return _alpha; }
    Anchor getAnchor() const { return _anchor; }

    // setters
    void setVisible(bool visible) { _visible = visible; }
    void setColor(const xColor& color) { _color = color; }
    void setAlpha(float alpha) { _alpha = alpha; }
    void setAnchor(Anchor anchor) { _anchor = anchor; }

    virtual void setProperties(const QScriptValue& properties);

protected:
    QGLWidget* _parent;
    bool _isLoaded;
    float _alpha;
    xColor _color;
    bool _visible; // should the overlay be drawn at all
    Anchor _anchor;
};

 
#endif // hifi_Overlay_h
