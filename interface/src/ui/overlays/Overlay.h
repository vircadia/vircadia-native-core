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

#include <render/Scene.h>

class OverlayID : public QUuid {
public:
    OverlayID() : QUuid() {}
    OverlayID(QString v) : QUuid(v) {}
    OverlayID(QUuid v) : QUuid(v) {}
};

class Overlay : public QObject {
    Q_OBJECT

public:
    typedef std::shared_ptr<Overlay> Pointer;
    typedef render::Payload<Overlay> Payload;
    typedef std::shared_ptr<render::Item::PayloadInterface> PayloadPointer;

    Overlay();
    Overlay(const Overlay* overlay);
    ~Overlay();

    virtual OverlayID getOverlayID() const { return _overlayID; }
    virtual void setOverlayID(OverlayID overlayID) { _overlayID = overlayID; }

    virtual void update(float deltatime) {}
    virtual void render(RenderArgs* args) = 0;

    virtual render::ItemKey getKey();
    virtual AABox getBounds() const = 0;
    virtual bool supportsGetProperty() const { return true; }

    virtual bool addToScene(Overlay::Pointer overlay, const render::ScenePointer& scene, render::Transaction& transaction);
    virtual void removeFromScene(Overlay::Pointer overlay, const render::ScenePointer& scene, render::Transaction& transaction);

    virtual const render::ShapeKey getShapeKey() { return render::ShapeKey::Builder::ownPipeline(); }

    virtual uint32_t fetchMetaSubItems(render::ItemIDs& subItems) const { return 0; }

    // getters
    virtual QString getType() const = 0;
    virtual bool is3D() const = 0;
    bool isLoaded() { return _isLoaded; }
    bool getVisible() const { return _visible; }
    virtual bool isTransparent() { return getAlphaPulse() != 0.0f || getAlpha() != 1.0f; };
    virtual bool getIsVisibleInSecondaryCamera() const { return false; }

    glm::u8vec3 getColor();
    float getAlpha();

    float getPulseMax() const { return _pulseMax; }
    float getPulseMin() const { return _pulseMin; }
    float getPulsePeriod() const { return _pulsePeriod; }
    float getPulseDirection() const { return _pulseDirection; }

    float getColorPulse() const { return _colorPulse; }
    float getAlphaPulse() const { return _alphaPulse; }

    // setters
    virtual void setVisible(bool visible) { _visible = visible; }
    void setDrawHUDLayer(bool drawHUDLayer);
    void setColor(const glm::u8vec3& color) { _color = color; }
    void setAlpha(float alpha) { _alpha = alpha; }

    void setPulseMax(float value) { _pulseMax = value; }
    void setPulseMin(float value) { _pulseMin = value; }
    void setPulsePeriod(float value) { _pulsePeriod = value; }
    void setPulseDirection(float value) { _pulseDirection = value; }

    void setColorPulse(float value) { _colorPulse = value; }
    void setAlphaPulse(float value) { _alphaPulse = value; }

    Q_INVOKABLE virtual void setProperties(const QVariantMap& properties);
    Q_INVOKABLE virtual Overlay* createClone() const = 0;
    Q_INVOKABLE virtual QVariant getProperty(const QString& property);

    render::ItemID getRenderItemID() const { return _renderItemID; }
    void setRenderItemID(render::ItemID renderItemID) { _renderItemID = renderItemID; }

    unsigned int getStackOrder() const { return _stackOrder; }
    void setStackOrder(unsigned int stackOrder) { _stackOrder = stackOrder; }

    virtual void addMaterial(graphics::MaterialLayer material, const std::string& parentMaterialName);
    virtual void removeMaterial(graphics::MaterialPointer material, const std::string& parentMaterialName);

protected:
    float updatePulse();

    render::ItemID _renderItemID{ render::Item::INVALID_ITEM_ID };

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

    glm::u8vec3 _color;
    bool _visible; // should the overlay be drawn at all

    unsigned int _stackOrder { 0 };

    static const glm::u8vec3 DEFAULT_OVERLAY_COLOR;
    static const float DEFAULT_ALPHA;

    std::unordered_map<std::string, graphics::MultiMaterial> _materials;
    std::mutex _materialsLock;

private:
    OverlayID _overlayID; // only used for non-3d overlays
};

namespace render {
   template <> const ItemKey payloadGetKey(const Overlay::Pointer& overlay);
   template <> const Item::Bound payloadGetBound(const Overlay::Pointer& overlay);
   template <> void payloadRender(const Overlay::Pointer& overlay, RenderArgs* args);
   template <> const ShapeKey shapeGetShapeKey(const Overlay::Pointer& overlay);
   template <> uint32_t metaFetchMetaSubItems(const Overlay::Pointer& overlay, ItemIDs& subItems);
}

Q_DECLARE_METATYPE(OverlayID);
Q_DECLARE_METATYPE(QVector<OverlayID>);
QScriptValue OverlayIDtoScriptValue(QScriptEngine* engine, const OverlayID& id);
void OverlayIDfromScriptValue(const QScriptValue& object, OverlayID& id);
QVector<OverlayID> qVectorOverlayIDFromScriptValue(const QScriptValue& array);

#endif // hifi_Overlay_h
