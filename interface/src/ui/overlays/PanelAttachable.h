//
//  PanelAttachable.h
//  interface/src/ui/overlays
//
//  Created by Zander Otavka on 7/1/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Base class for anything that can attach itself to an `OverlayPanel` as a child.
//  `PanelAttachable keeps an `std::shared_ptr` to it's parent panel, and sets its
//  transformations and visibility based on the parent.
//
//  When subclassing `PanelAttachable`, make sure `applyTransformTo`, `getProperty`, and
//  `setProperties are all called in the appropriate places.  Look through `Image3DOverlay` and
//  `Billboard3DOverlay` for examples.  Pay special attention to `applyTransformTo`; it should
//  be called in three places for `Overlay`s: `render`, `update`, and `findRayIntersection`.
//
//  When overriding `applyTransformTo`, make sure to wrap all of your code, including the call
//  to the superclass method, with the following `if` block.  Then call the superclass method
//  with force = true.
//
//      if (force || usecTimestampNow() > _transformExpiry) {
//          PanelAttachable::applyTransformTo(transform, true);
//          ...
//      }
//

#ifndef hifi_PanelAttachable_h
#define hifi_PanelAttachable_h

#define OVERLAY_PANELS 0

#include <memory>

#include <glm/glm.hpp>
#include <Transform.h>
#include <QScriptValue>
#include <QScriptEngine>
#include <QUuid>

class OverlayPanel;
class PanelAttachable {
public:
    // getters
#if OVERLAY_PANELS
    std::shared_ptr<OverlayPanel> getParentPanel() const { return _parentPanel; }
#endif
    glm::vec3 getOffsetPosition() const { return _offset.getTranslation(); }
    glm::quat getOffsetRotation() const { return _offset.getRotation(); }
    glm::vec3 getOffsetScale() const { return _offset.getScale(); }
    bool getParentVisible() const;

    // setters
#if OVERLAY_PANELS
    void setParentPanel(std::shared_ptr<OverlayPanel> panel) { _parentPanel = panel; }
#endif
    void setOffsetPosition(const glm::vec3& position) { _offset.setTranslation(position); }
    void setOffsetRotation(const glm::quat& rotation) { _offset.setRotation(rotation); }
    void setOffsetScale(float scale) { _offset.setScale(scale); }
    void setOffsetScale(const glm::vec3& scale) { _offset.setScale(scale); }

protected:
    void setProperties(const QVariantMap& properties);
    QVariant getProperty(const QString& property);

    /// set position, rotation and scale on transform based on offsets, and parent panel offsets
    /// if force is false, only apply transform if it hasn't been applied in the last .1 seconds
    virtual bool applyTransformTo(Transform& transform, bool force = false);
    quint64 _transformExpiry = 0;

private:
#if OVERLAY_PANELS
    std::shared_ptr<OverlayPanel> _parentPanel = nullptr;
#endif
    Transform _offset;
};

#endif // hifi_PanelAttachable_h
