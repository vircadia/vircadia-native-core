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
#include <SharedUtil.h> // for xColor
#include <render/Scene.h>

class QScriptEngine;
class QScriptValue;

class Overlay : public QObject {
    Q_OBJECT
    
public:
    enum Anchor {
        NO_ANCHOR,
        MY_AVATAR
    };

    typedef std::shared_ptr<Overlay> Pointer;
    typedef render::Payload<Overlay> Payload;
    typedef std::shared_ptr<render::Item::PayloadInterface> PayloadPointer;

    Overlay();
    Overlay(const Overlay* overlay);
    ~Overlay();
    void init(QScriptEngine* scriptEngine);
    virtual void update(float deltatime) {}
    virtual void render(RenderArgs* args) = 0;
    
    virtual AABox getBounds() const = 0;

    virtual bool addToScene(Overlay::Pointer overlay, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges);
    virtual void removeFromScene(Overlay::Pointer overlay, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges);

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
    virtual Overlay* createClone() const = 0;
    virtual QScriptValue getProperty(const QString& property);

    render::ItemID getRenderItemID() const { return _renderItemID; }
    void setRenderItemID(render::ItemID renderItemID) { _renderItemID = renderItemID; }

protected:
    float updatePulse();

    render::ItemID _renderItemID;

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

namespace render {
   template <> const ItemKey payloadGetKey(const Overlay::Pointer& overlay); 
   template <> const Item::Bound payloadGetBound(const Overlay::Pointer& overlay);
   template <> int payloadGetLayer(const Overlay::Pointer& overlay);
   template <> void payloadRender(const Overlay::Pointer& overlay, RenderArgs* args);
}

 
#endif // hifi_Overlay_h
