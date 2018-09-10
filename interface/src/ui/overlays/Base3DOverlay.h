//
//  Base3DOverlay.h
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Base3DOverlay_h
#define hifi_Base3DOverlay_h

#include <Transform.h>
#include <SpatiallyNestable.h>
#include <graphics-scripting/Forward.h>
#include "Overlay.h"

class Base3DOverlay : public Overlay, public SpatiallyNestable, public scriptable::ModelProvider {
    Q_OBJECT
    using Parent = Overlay;

public:
    Base3DOverlay();
    Base3DOverlay(const Base3DOverlay* base3DOverlay);

    void setVisible(bool visible) override;

    virtual OverlayID getOverlayID() const override { return OverlayID(getID().toString()); }
    void setOverlayID(OverlayID overlayID) override { setID(overlayID); }

    virtual QString getName() const override;
    void setName(QString name);

    // getters
    virtual bool is3D() const override { return true; }

    virtual render::ItemKey getKey() override;
    virtual uint32_t fetchMetaSubItems(render::ItemIDs& subItems) const override { subItems.push_back(getRenderItemID()); return (uint32_t) subItems.size(); }
    virtual scriptable::ScriptableModelBase getScriptableModel() override { return scriptable::ScriptableModelBase(); }

    // TODO: consider implementing registration points in this class
    glm::vec3 getCenter() const { return getWorldPosition(); }

    bool getIsSolid() const { return _isSolid; }
    bool getIsDashedLine() const { return _isDashedLine; }
    bool getIsSolidLine() const { return !_isDashedLine; }
    bool getIgnorePickIntersection() const { return _ignorePickIntersection; }
    bool getDrawInFront() const { return _drawInFront; }
    bool getDrawHUDLayer() const { return _drawHUDLayer; }
    bool getIsGrabbable() const { return _isGrabbable; }
    virtual bool getIsVisibleInSecondaryCamera() const override { return _isVisibleInSecondaryCamera; }

    void setIsSolid(bool isSolid) { _isSolid = isSolid; }
    void setIsDashedLine(bool isDashedLine) { _isDashedLine = isDashedLine; }
    void setIgnorePickIntersection(bool value) { _ignorePickIntersection = value; }
    virtual void setDrawInFront(bool value) { _drawInFront = value; }
    virtual void setDrawHUDLayer(bool value) { _drawHUDLayer = value; }
    void setIsGrabbable(bool value) { _isGrabbable = value; }
    virtual void setIsVisibleInSecondaryCamera(bool value) { _isVisibleInSecondaryCamera = value; }

    virtual AABox getBounds() const override = 0;

    void update(float deltatime) override;

    void notifyRenderVariableChange() const;

    virtual void setProperties(const QVariantMap& properties) override;
    virtual QVariant getProperty(const QString& property) override;

    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance,
                                     BoxFace& face, glm::vec3& surfaceNormal, bool precisionPicking = false) { return false; }

    virtual bool findRayIntersectionExtraInfo(const glm::vec3& origin, const glm::vec3& direction,
                                              float& distance, BoxFace& face, glm::vec3& surfaceNormal, QVariantMap& extraInfo, bool precisionPicking = false) {
        return findRayIntersection(origin, direction, distance, face, surfaceNormal, precisionPicking);
    }

    virtual bool findParabolaIntersection(const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration, float& parabolicDistance,
                                          BoxFace& face, glm::vec3& surfaceNormal, bool precisionPicking = false) { return false; }

    virtual bool findParabolaIntersectionExtraInfo(const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration,
                                                   float& parabolicDistance, BoxFace& face, glm::vec3& surfaceNormal, QVariantMap& extraInfo, bool precisionPicking = false) {
        return findParabolaIntersection(origin, velocity, acceleration, parabolicDistance, face, surfaceNormal, precisionPicking);
    }

    virtual SpatialParentTree* getParentTree() const override;

protected:
    virtual void locationChanged(bool tellPhysics = true) override;
    virtual void parentDeleted() override;

    mutable Transform _renderTransform;
    mutable bool _renderVisible;
    virtual Transform evalRenderTransform();
    virtual void setRenderTransform(const Transform& transform);
    void setRenderVisible(bool visible);
    const Transform& getRenderTransform() const { return _renderTransform; }

    bool _isSolid;
    bool _isDashedLine;
    bool _ignorePickIntersection;
    bool _drawInFront;
    bool _drawHUDLayer;
    bool _isGrabbable { false };
    bool _isVisibleInSecondaryCamera { false };
    mutable bool _renderVariableDirty { true };

    QString _name;
    mutable ReadWriteLockable _nameLock;
};

#endif // hifi_Base3DOverlay_h
