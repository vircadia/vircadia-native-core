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
    const xColor& getColor() const { return _color; }
    float getAlpha() const { return _alpha; }

    // setters
    void setVisible(bool visible) { _visible = visible; }
    void setColor(const xColor& color) { _color = color; }
    void setAlpha(float alpha) { _alpha = alpha; }

    virtual void setProperties(const QScriptValue& properties);

protected:
    QGLWidget* _parent;
    float _alpha;
    xColor _color;
    bool _visible; // should the overlay be drawn at all
};

 
#endif /* defined(__interface__Overlay__) */
