//
//  Overlay2D.h
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Overlay2D_h
#define hifi_Overlay2D_h

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
    
    virtual bool is3D() const { return false; }

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
    virtual void writeToClone(Overlay2D* clone);
    QRect _bounds; // where on the screen to draw
};

 
#endif // hifi_Overlay2D_h
