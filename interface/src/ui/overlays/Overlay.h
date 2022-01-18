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

class Overlay : public QObject {
    Q_OBJECT

public:
    typedef std::shared_ptr<Overlay> Pointer;
    typedef render::Payload<Overlay> Payload;
    typedef std::shared_ptr<render::Item::PayloadInterface> PayloadPointer;

    Overlay();
    Overlay(const Overlay* overlay);

    virtual QUuid getID() const { return _id; }
    virtual void setID(const QUuid& id) { _id = id; }

    virtual void render(RenderArgs* args) = 0;

    virtual render::ItemKey getKey();
    virtual AABox getBounds() const = 0;

    virtual bool addToScene(Overlay::Pointer overlay, const render::ScenePointer& scene, render::Transaction& transaction);
    virtual void removeFromScene(Overlay::Pointer overlay, const render::ScenePointer& scene, render::Transaction& transaction);

    virtual const render::ShapeKey getShapeKey() { return render::ShapeKey::Builder::ownPipeline(); }

    virtual uint32_t fetchMetaSubItems(render::ItemIDs& subItems) const { return 0; }

    // getters
    virtual QString getType() const = 0;
    bool getVisible() const { return _visible; }

    // setters
    void setVisible(bool visible) { _visible = visible; }
    unsigned int getStackOrder() const { return _stackOrder; }
    void setStackOrder(unsigned int stackOrder) { _stackOrder = stackOrder; }

    Q_INVOKABLE virtual void setProperties(const QVariantMap& properties) = 0;
    Q_INVOKABLE virtual Overlay* createClone() const = 0;

    render::ItemID getRenderItemID() const { return _renderItemID; }
    void setRenderItemID(render::ItemID renderItemID) { _renderItemID = renderItemID; }

protected:
    render::ItemID _renderItemID { render::Item::INVALID_ITEM_ID };

    bool _visible { true };
    unsigned int _stackOrder { 0 };

private:
    QUuid _id;
};

namespace render {
   template <> const ItemKey payloadGetKey(const Overlay::Pointer& overlay);
   template <> const Item::Bound payloadGetBound(const Overlay::Pointer& overlay, RenderArgs* args);
   template <> void payloadRender(const Overlay::Pointer& overlay, RenderArgs* args);
   template <> const ShapeKey shapeGetShapeKey(const Overlay::Pointer& overlay);
   template <> uint32_t metaFetchMetaSubItems(const Overlay::Pointer& overlay, ItemIDs& subItems);
}

#endif // hifi_Overlay_h
