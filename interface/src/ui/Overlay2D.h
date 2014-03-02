//
//  Overlay2D.h
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Overlay2D__
#define __interface__Overlay2D__

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <QRect>
#include <QScriptValue>
#include <QString>

#include <SharedUtil.h> // for xColor

#include "Overlay.h"

class Overlay2D : public Overlay {
    Q_OBJECT
    
public:
    Overlay2D();
    ~Overlay2D();

    // getters
    int getX() const { return _bounds.x(); }
    int getY() const { return _bounds.y(); }
    int getWidth() const { return _bounds.width(); }
    int getHeight() const { return _bounds.height(); }
    const QRect& getBounds() const { return _bounds; }

    // setters
    void setX(int x) { _bounds.setX(x); }
    void setY(int y) { _bounds.setY(y);  }
    void setWidth(int width) { _bounds.setWidth(width); }
    void setHeight(int height) {  _bounds.setHeight(height); }
    void setBounds(const QRect& bounds) { _bounds = bounds; }

    virtual void setProperties(const QScriptValue& properties);

protected:
    QRect _bounds; // where on the screen to draw
};

 
#endif /* defined(__interface__Overlay2D__) */
