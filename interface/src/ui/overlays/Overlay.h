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

#include <RegisteredMetaTypes.h>
#include <SharedUtil.h> // for xColor
#include <RenderArgs.h>

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
    void init(QGLWidget* parent, QScriptEngine* scriptEngine);
    virtual void update(float deltatime) {}
    virtual void render(RenderArgs* args) = 0;

    // getters
    virtual bool is3D() const = 0;
    bool isLoaded() { return _isLoaded; }
    bool getVisible() const { return _visible; }
    xColor getColor();
    float getAlpha();
    float getGlowLevel();
    Anchor getAnchor() const { return _anchor; }


    float getPulseMax() const { return _pulseMax; }
    float getPulseMin() const { return _pulseMin; }
    float getPulsePeriod() const { return _pulsePeriod; }
    float getPulseDirection() const { return _pulseDirection; }

    float getGlowLevelPulse() const { return _glowLevelPulse; }
    float getColorPulse() const { return _colorPulse; }
    float getAlphaPulse() const { return _alphaPulse; }

    // setters
    void setVisible(bool visible) { _visible = visible; }
    void setColor(const xColor& color) { _color = color; }
    void setAlpha(float alpha) { _alpha = alpha; }
    void setGlowLevel(float value) { _glowLevel = value; }
    void setAnchor(Anchor anchor) { _anchor = anchor; }

    void setPulseMax(float value) { _pulseMax = value; }
    void setPulseMin(float value) { _pulseMin = value; }
    void setPulsePeriod(float value) { _pulsePeriod = value; }
    void setPulseDirection(float value) { _pulseDirection = value; }


    void setGlowLevelPulse(float value) { _glowLevelPulse = value; }
    void setColorPulse(float value) { _colorPulse = value; }
    void setAlphaPulse(float value) { _alphaPulse = value; }

    virtual void setProperties(const QScriptValue& properties);
    virtual QScriptValue getProperty(const QString& property);

protected:
    float updatePulse();

    QGLWidget* _parent;
    bool _isLoaded;
    float _alpha;
    float _glowLevel;

    float _pulse;
    float _pulseMax;
    float _pulseMin;
    float _pulsePeriod;
    float _pulseDirection;
    quint64 _lastPulseUpdate;

    float _glowLevelPulse; // ratio of the pulse to the glow level
    float _alphaPulse; // ratio of the pulse to the alpha
    float _colorPulse; // ratio of the pulse to the color

    xColor _color;
    bool _visible; // should the overlay be drawn at all
    Anchor _anchor;

    QScriptEngine* _scriptEngine;
};

 
#endif // hifi_Overlay_h
