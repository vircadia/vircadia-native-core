//
//  Overlay.h
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Overlay__
#define __interface__Overlay__

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <QRect>
#include <QScriptValue>
#include <QString>

#include <SharedUtil.h> // for xColor

const xColor DEFAULT_BACKGROUND_COLOR = { 255, 255, 255 };
const float DEFAULT_ALPHA = 0.7f;

class Overlay : public QObject {
    Q_OBJECT
    
public:
    Overlay();
    ~Overlay();
    void init(QGLWidget* parent);
    virtual void render() = 0;

    // getters
    bool getVisible() const { return _visible; }
    int getX() const { return _bounds.x(); }
    int getY() const { return _bounds.y(); }
    int getWidth() const { return _bounds.width(); }
    int getHeight() const { return _bounds.height(); }
    const QRect& getBounds() const { return _bounds; }
    const xColor& getBackgroundColor() const { return _backgroundColor; }
    float getAlpha() const { return _alpha; }

    // setters
    void setVisible(bool visible) { _visible = visible; }
    void setX(int x) { _bounds.setX(x); }
    void setY(int y) { _bounds.setY(y);  }
    void setWidth(int width) { _bounds.setWidth(width); }
    void setHeight(int height) {  _bounds.setHeight(height); }
    void setBounds(const QRect& bounds) { _bounds = bounds; }
    void setBackgroundColor(const xColor& color) { _backgroundColor = color; }
    void setAlpha(float alpha) { _alpha = alpha; }

    virtual void setProperties(const QScriptValue& properties);

protected:
    QGLWidget* _parent;
    QRect _bounds; // where on the screen to draw
    float _alpha;
    xColor _backgroundColor;
    bool _visible; // should the overlay be drawn at all
};

 
#endif /* defined(__interface__Overlay__) */
