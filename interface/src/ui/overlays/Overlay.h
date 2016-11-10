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

    unsigned int getOverlayID() { return _overlayID; }
    void setOverlayID(unsigned int overlayID) { _overlayID = overlayID; }

    virtual void update(float deltatime) {}
    virtual void render(RenderArgs* args) = 0;

    virtual AABox getBounds() const = 0;
    virtual bool supportsGetProperty() const { return true; }

    virtual bool addToScene(Overlay::Pointer overlay, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges);
    virtual void removeFromScene(Overlay::Pointer overlay, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges);

    virtual const render::ShapeKey getShapeKey() { return render::ShapeKey::Builder::ownPipeline(); }

    // getters
    virtual QString getType() const = 0;
    virtual bool is3D() const = 0;
    bool isLoaded() { return _isLoaded; }
    bool getVisible() const { return _visible; }
    xColor getColor();
    float getAlpha();
    Anchor getAnchor() const { return _anchor; }

    float getPulseMax() const { return _pulseMax; }
    float getPulseMin() const { return _pulseMin; }
    float getPulsePeriod() const { return _pulsePeriod; }
    float getPulseDirection() const { return _pulseDirection; }

    float getColorPulse() const { return _colorPulse; }
    float getAlphaPulse() const { return _alphaPulse; }

    // setters
    void setVisible(bool visible) { _visible = visible; }
    void setColor(const xColor& color) { _color = color; }
    void setAlpha(float alpha) { _alpha = alpha; }
    void setAnchor(Anchor anchor) { _anchor = anchor; }

    void setPulseMax(float value) { _pulseMax = value; }
    void setPulseMin(float value) { _pulseMin = value; }
    void setPulsePeriod(float value) { _pulsePeriod = value; }
    void setPulseDirection(float value) { _pulseDirection = value; }

    void setColorPulse(float value) { _colorPulse = value; }
    void setAlphaPulse(float value) { _alphaPulse = value; }

    virtual void setProperties(const QVariantMap& properties);
    virtual Overlay* createClone() const = 0;
    virtual QVariant getProperty(const QString& property);

    render::ItemID getRenderItemID() const { return _renderItemID; }
    void setRenderItemID(render::ItemID renderItemID) { _renderItemID = renderItemID; }

protected:
    float updatePulse();

    render::ItemID _renderItemID{ render::Item::INVALID_ITEM_ID };

    unsigned int _overlayID; // what Overlays.cpp knows this instance as

    bool _isLoaded;
    float _alpha;

    float _pulse;
    float _pulseMax;
    float _pulseMin;
    float _pulsePeriod;
    float _pulseDirection;
    quint64 _lastPulseUpdate;

    float _alphaPulse; // ratio of the pulse to the alpha
    float _colorPulse; // ratio of the pulse to the color

    xColor _color;
    bool _visible; // should the overlay be drawn at all
    Anchor _anchor;
};

namespace render {
   template <> const ItemKey payloadGetKey(const Overlay::Pointer& overlay); 
   template <> const Item::Bound payloadGetBound(const Overlay::Pointer& overlay);
   template <> int payloadGetLayer(const Overlay::Pointer& overlay);
   template <> void payloadRender(const Overlay::Pointer& overlay, RenderArgs* args);
   template <> const ShapeKey shapeGetShapeKey(const Overlay::Pointer& overlay);
}

 
#endif // hifi_Overlay_h
